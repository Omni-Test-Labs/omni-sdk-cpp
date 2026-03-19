/**
 * @file ssh_client.cpp
 * @brief SshClient implementation using libssh2.
 */

#include "omni_sdk/clients/ssh_client.h"

#ifdef OMNI_SDK_ENABLE_SSH
#include <libssh2.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <fstream>

namespace omni_sdk {

struct SshClient::Impl {
    LIBSSH2_SESSION* session;
    LIBSSH2_CHANNEL* channel;

    std::map<std::string, std::string> config;
    bool connected;
    int sock;

    Impl()
        : session(nullptr),
          channel(nullptr),
          connected(false),
          sock(-1) {}

    ~Impl() {
        cleanup();
    }

    void cleanup() {
        if (channel) {
            libssh2_channel_free(channel);
            channel = nullptr;
        }
        if (session) {
            libssh2_session_disconnect(session, "Normal shutdown");
            libssh2_session_free(session);
            session = nullptr;
        }
        if (sock >= 0) {
            close(sock);
            sock = -1;
        }
        connected = false;
    }
};

SshClient::SshClient() : impl_(std::make_unique<Impl>()) {}

SshClient::~SshClient() {
    impl_->cleanup();
}

Result<void> SshClient::initialize(const std::map<std::string, std::string>& config) {
    std::vector<std::string> required_fields = {"host", "port", "username"};
    for (const auto& field : required_fields) {
        if (config.find(field) == config.end()) {
            std::ostringstream oss;
            oss << "Missing required SSH config field: " << field;
            return Result<void>::Err(createError(
                ErrorKinds::CONFIG_ERROR,
                oss.str(),
                {{"required_fields", "host, port, username"}}
            ));
        }
    }

    impl_->config = config;

    // Load password from file if specified
    if (config.find("password_file") != config.end()) {
        std::string password_file_path = config.at("password_file");
        std::ifstream file(password_file_path);
        if (!file.is_open()) {
            return Result<void>::Err(createError(
                ErrorKinds::CONFIG_ERROR,
                "Failed to read password file",
                {{"password_file", password_file_path}}
            ));
        }
        std::string password;
        std::getline(file, password);
        file.close();
        impl_->config["password"] = password;
    }

    // Validate authentication credentials
    if (impl_->config.find("password") == impl_->config.end() &&
        impl_->config.find("key_file") == impl_->config.end()) {
        return Result<void>::Err(createError(
            ErrorKinds::AUTHENTICATION_ERROR,
            "SSH authentication requires either password or key_file",
            {{"config_keys", "host, port, username"}}
        ));
    }

    return Result<void>::Ok();
}

Result<void> SshClient::connect() {
    if (impl_->connected) {
        return Result<void>::Ok();
    }

    std::string host = impl_->config["host"];
    int port = std::stoi(impl_->config["port"]);
    std::string username = impl_->config["username"];
    std::string password = impl_->config.count("password") ? impl_->config["password"] : "";
    std::string key_file = impl_->config.count("key_file") ? impl_->config["key_file"] : "";
    int timeout_ms = impl_->config.count("timeout_ms") ? std::stoi(impl_->config["timeout_ms"]) : 5000;

    try {
        // Create socket
        impl_->sock = socket(AF_INET, SOCK_STREAM, 0);
        if (impl_->sock < 0) {
            return Result<void>::Err(createError(
                ErrorKinds::CONNECTION_ERROR,
                "Failed to create socket",
                {{"host", host}, {"port", std::to_string(port)}}
            ));
        }

        // Set socket timeout
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(impl_->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(impl_->sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        // Connect to server
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        if (::connect(impl_->sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            return Result<void>::Err(createError(
                ErrorKinds::CONNECTION_ERROR,
                "Failed to connect to SSH server",
                {{"host", host}, {"port", std::to_string(port)}, {"error", std::string(strerror(errno))}}
            ));
        }

        // Create libssh2 session
        impl_->session = libssh2_session_init();
        if (!impl_->session) {
            return Result<void>::Err(createError(
                ErrorKinds::SSH_ERROR,
                "Failed to create SSH session"
            ));
        }

        // Perform SSH handshake
        if (libssh2_session_handshake(impl_->session, impl_->sock) != 0) {
            return Result<void>::Err(createError(
                ErrorKinds::SSH_ERROR,
                "SSH handshake failed",
                {{"host", host}, {"port", std::to_string(port)}}
            ));
        }

        // Set timeout for session operations
        libssh2_session_set_timeout(impl_->session, timeout_ms);

        // Authenticate
        const char* auth_list = libssh2_userauth_list(
            impl_->session, username.c_str(), username.length());

        bool auth_success = false;
        std::string auth_error;

        // Try public key authentication first
        if (auth_list && strstr(auth_list, "publickey") && !key_file.empty()) {
            if (libssh2_userauth_publickey_fromfile(
                    impl_->session,
                    username.c_str(),
                    nullptr,
                    key_file.c_str(),
                    nullptr) == 0) {
                auth_success = true;
            } else {
                char* err_msg = nullptr;
                libssh2_session_last_error(impl_->session, &err_msg, nullptr, 0);
                auth_error = "Public key authentication failed: " + std::string(err_msg ? err_msg : "unknown");
            }
        }

        // Try password authentication
        if (!auth_success && auth_list && strstr(auth_list, "password") && !password.empty()) {
            if (libssh2_userauth_password(
                    impl_->session,
                    username.c_str(),
                    password.c_str()) == 0) {
                auth_success = true;
            } else {
                char* err_msg = nullptr;
                libssh2_session_last_error(impl_->session, &err_msg, nullptr, 0);
                auth_error = "Password authentication failed: " + std::string(err_msg ? err_msg : "unknown");
            }
        }

        if (!auth_success) {
            return Result<void>::Err(createError(
                ErrorKinds::AUTHENTICATION_ERROR,
                "SSH authentication failed",
                {{"host", host}, {"username", username}, {"error", auth_error}}
            ));
        }

        impl_->connected = true;
        return Result<void>::Ok();

    } catch (const std::exception& e) {
        return Result<void>::Err(createError(
            ErrorKinds::CONNECTION_ERROR,
            std::string("Connection error: ") + e.what(),
            {{"exception_type", typeid(e).name()}}
        ));
    }
}

Result<void> SshClient::disconnect() {
    if (impl_->connected) {
        impl_->cleanup();
        impl_->connected = false;
    }
    return Result<void>::Ok();
}

bool SshClient::isConnected() const {
    if (!impl_->connected || !impl_->session) {
        return false;
    }
    return libssh2_session_block_directions(impl_->session) == 0;
}

Result<void> SshClient::send(const std::string& command) {
    if (!impl_->connected || !impl_->session) {
        return Result<void>::Err(createError(
            ErrorKinds::DEVICE_NOT_CONNECTED,
            "SSH client is not connected"
        ));
    }

    impl_->channel = libssh2_channel_open_session(impl_->session);
    if (!impl_->channel) {
        return Result<void>::Err(createError(
            ErrorKinds::SSH_ERROR,
            "Failed to open SSH channel"
        ));
    }

    if (libssh2_channel_exec(impl_->channel, command.c_str()) != 0) {
        libssh2_channel_free(impl_->channel);
        impl_->channel = nullptr;
        return Result<void>::Err(createError(
            ErrorKinds::SSH_ERROR,
            "Failed to execute command",
            {{"command", command}}
        ));
    }

    return Result<void>::Ok();
}

Result<std::string> SshClient::receive(int timeout_ms) {
    if (!impl_->connected || !impl_->session) {
        return Result<std::string>::Err(createError(
            ErrorKinds::DEVICE_NOT_CONNECTED,
            "SSH client is not connected"
        ));
    }

    if (!impl_->channel) {
        return Result<std::string>::Err(createError(
            ErrorKinds::SSH_ERROR,
            "SSH channel not available"
        ));
    }

    std::ostringstream output;
    char buffer[4096];
    int rc;

    while ((rc = libssh2_channel_read(impl_->channel, buffer, sizeof(buffer))) > 0) {
        output.write(buffer, rc);
    }

    libssh2_channel_free(impl_->channel);
    impl_->channel = nullptr;

    return Result<std::string>::Ok(output.str());
}

Result<std::string> SshClient::sendAndReceive(const std::string& command, int timeout_ms) {
    return execute(command, timeout_ms);
}

std::map<std::string, std::string> SshClient::capabilities() const {
    return {
        {"execute", "Execute shell command and return output"},
        {"file_transfer", "Transfer files using SCP"},
        {"get_status", "Get SSH connection status"}
    };
}

Result<void> SshClient::configure(const std::map<std::string, std::string>& params) {
    for (const auto& [key, value] : params) {
        if (key == "timeout_ms") {
            impl_->config["timeout_ms"] = value;
            if (impl_->session) {
                int timeout_ms = std::stoi(value);
                libssh2_session_set_timeout(impl_->session, timeout_ms);
            }
        } else {
            impl_->config[key] = value;
        }
    }
    return Result<void>::Ok();
}

Result<std::map<std::string, std::string>> SshClient::getStatus() {
    std::map<std::string, std::string> status = {
        {"connected", impl_->connected ? "true" : "false"},
        {"client_name", name()},
        {"version", version()},
        {"host", impl_->config.count("host") ? impl_->config["host"] : ""},
        {"port", impl_->config.count("port") ? impl_->config["port"] : ""},
        {"username", impl_->config.count("username") ? impl_->config["username"] : ""}
    };

    return Result<std::map<std::string, std::string>>::Ok(status);
}

Result<std::string> SshClient::execute(const std::string& command, int timeout_ms) {
    if (!impl_->connected || !impl_->session) {
        return Result<std::string>::Err(createError(
            ErrorKinds::DEVICE_NOT_CONNECTED,
            "SSH client is not connected"
        ));
    }

    LIBSSH2_CHANNEL* channel = libssh2_channel_open_session(impl_->session);
    if (!channel) {
        char* err_msg = nullptr;
        int err_code = libssh2_session_last_error(impl_->session, &err_msg, nullptr, 0);
        return Result<std::string>::Err(createError(
            ErrorKinds::SSH_ERROR,
            "Failed to open SSH channel",
            {{"libssh2_error_code", std::to_string(err_code)}, {"error", err_msg ? err_msg : "unknown"}}
        ));
    }

    if (libssh2_channel_exec(channel, command.c_str()) != 0) {
        char* err_msg = nullptr;
        int err_code = libssh2_session_last_error(impl_->session, &err_msg, nullptr, 0);
        libssh2_channel_free(channel);
        return Result<std::string>::Err(createError(
            ErrorKinds::SSH_ERROR,
            "Failed to execute command",
            {{"command", command}, {"libssh2_error_code", std::to_string(err_code)}, {"error", err_msg ? err_msg : "unknown"}}
        ));
    }

    std::ostringstream output;
    std::ostringstream error_output;
    char buffer[4096];
    int rc;

    while ((rc = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
        output.write(buffer, rc);
    }

    char* err_msg = nullptr;
    int exit_code = libssh2_channel_get_exit_status(channel);

    libssh2_channel_free(channel);

    if (exit_code != 0) {
        return Result<std::string>::Err(createError(
            ErrorKinds::SSH_ERROR,
            "Command failed with exit code " + std::to_string(exit_code),
            {{"command", command}, {"exit_code", std::to_string(exit_code)}, {"output", output.str()}}
        ));
    }

    return Result<std::string>::Ok(output.str());
}

Result<void> SshClient::fileTransfer(const std::string& local_path,
                                      const std::string& remote_path) {
    if (!impl_->connected || !impl_->session) {
        return Result<void>::Err(createError(
            ErrorKinds::DEVICE_NOT_CONNECTED,
            "SSH client is not connected"
        ));
    }

    LIBSSH2_CHANNEL* channel = libssh2_scp_send(
        impl_->session,
        remote_path.c_str(),
        0644,
        0);

    if (!channel) {
        char* err_msg = nullptr;
        libssh2_session_last_error(impl_->session, &err_msg, nullptr, 0);
        return Result<void>::Err(createError(
            ErrorKinds::SSH_ERROR,
            "Failed to open SCP channel",
            {{"remote_path", remote_path}, {"error", err_msg ? err_msg : "unknown"}}
        ));
    }

    std::ifstream file(local_path, std::ios::binary);
    if (!file.is_open()) {
        libssh2_channel_free(channel);
        return Result<void>::Err(createError(
            ErrorKinds::SSH_ERROR,
            "Failed to open local file",
            {{"local_path", local_path}}
        ));
    }

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) {
        libssh2_channel_write(channel, buffer, file.gcount());
    }

    if (file.gcount() > 0) {
        libssh2_channel_write(channel, buffer, file.gcount());
    }

    file.close();
    libssh2_channel_send_eof(channel);
    libssh2_channel_wait_eof(channel);
    libssh2_channel_wait_closed(channel);
    libssh2_channel_free(channel);

    return Result<void>::Ok();
}

} // namespace omni_sdk

#else // !OMNI_SDK_ENABLE_SSH

namespace omni_sdk {

struct SshClient::Impl {};

SshClient::SshClient() : impl_(std::make_unique<Impl>()) {}
SshClient::~SshClient() = default;

Result<void> SshClient::initialize(const std::map<std::string, std::string>&) {
    return Result<void>::Err(createError(
        ErrorKinds::SSH_ERROR,
        "SSH client support not enabled. Build with OMNI_SDK_ENABLE_SSH=ON"
    ));
}

Result<void> SshClient::connect() {
    return Result<void>::Err(createError(
        ErrorKinds::SSH_ERROR,
        "SSH client support not enabled. Build with OMNI_SDK_ENABLE_SSH=ON"
    ));
}

Result<void> SshClient::disconnect() { return Result<void>::Ok(); }
bool SshClient::isConnected() const { return false; }

Result<void> SshClient::send(const std::string&) {
    return Result<void>::Err(createError(
        ErrorKinds::SSH_ERROR,
        "SSH client support not enabled. Build with OMNI_SDK_ENABLE_SSH=ON"
    ));
}

Result<std::string> SshClient::receive(int) {
    return Result<std::string>::Err(createError(
        ErrorKinds::SSH_ERROR,
        "SSH client support not enabled. Build with OMNI_SDK_ENABLE_SSH=ON"
    ));
}

Result<std::string> SshClient::sendAndReceive(const std::string&, int) {
    return Result<std::string>::Err(createError(
        ErrorKinds::SSH_ERROR,
        "SSH client support not enabled. Build with OMNI_SDK_ENABLE_SSH=ON"
    ));
}

std::map<std::string, std::string> SshClient::capabilities() const { return {}; }

Result<void> SshClient::configure(const std::map<std::string, std::string>&) {
    return Result<void>::Err(createError(
        ErrorKinds::SSH_ERROR,
        "SSH client support not enabled. Build with OMNI_SDK_ENABLE_SSH=ON"
    ));
}

Result<std::map<std::string, std::string>> SshClient::getStatus() {
    std::map<std::string, std::string> status = {
        {"connected", "false"},
        {"client_name", name()},
        {"version", version()}
    };
    return Result<std::map<std::string, std::string>>::Ok(status);
}

Result<std::string> SshClient::execute(const std::string&, int) {
    return Result<std::string>::Err(createError(
        ErrorKinds::SSH_ERROR,
        "SSH client support not enabled. Build with OMNI_SDK_ENABLE_SSH=ON"
    ));
}

Result<void> SshClient::fileTransfer(const std::string&, const std::string&) {
    return Result<void>::Err(createError(
        ErrorKinds::SSH_ERROR,
        "SSH client support not enabled. Build with OMNI_SDK_ENABLE_SSH=ON"
    ));
}

} // namespace omni_sdk

#endif // OMNI_SDK_ENABLE_SSH
