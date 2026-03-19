/**
 * @file ssh_client.h
 * @brief SshClient: SSH protocol client implementation.
 *
 * @note This is a placeholder implementation.
 *
 * Real implementation requires libssh2.
 *
 * Dependencies:
 *   - libssh2 (for SSH protocol communication)
 *
 * Implementation notes:
 *   1. Use libssh2_session for SSH session management
 *   2. Use libssh2_channel_exec for command execution
 *   3. Implement all 9 Client interface methods
 *   4. See Python ssh_client.py for complete implementation
 *
 * To implement:
 *   - Add find_package(libssh2) to CMakeLists.txt
 *   - Implement connect() with libssh2_session_handshake()
 *   - Implement sendAndReceive() with libssh2_channel_exec()
 *   - Implement authentication (password or key file)
 *   - Add execute capability using libssh2_channel_exec()
 */

#pragma once

#include "omni_sdk/result.h"
#include <string>
#include <map>

namespace omni_sdk {

class SshClient {
public:
    SshClient();
    ~SshClient();

    std::string name() const { return std::string("ssh"); }
    std::string version() const { return std::string("1.0.0"); }

    Result<void> initialize(const std::map<std::string, std::string>& config);
    Result<void> connect();
    Result<void> disconnect();
    bool isConnected() const;

    Result<void> send(const std::string& command);
    Result<std::string> receive(int timeout_ms = 5000);
    Result<std::string> sendAndReceive(const std::string& command, int timeout_ms = 5000);

    std::map<std::string, std::string> capabilities() const;

    Result<void> configure(const std::map<std::string, std::string>& params);

    Result<std::map<std::string, std::string>> getStatus();

    /**
     * @brief Execute shell command and return output.
     * @param command Command to execute
     * @param timeout_ms Timeout in milliseconds
     * @return Result.ok(output) on success, Result.err on failure
     */
    Result<std::string> execute(const std::string& command, int timeout_ms = 5000);

    /**
     * @brief Transfer file to remote host using SFTP.
     * @param local_path Local file path
     * @param remote_path Remote file path
     * @return Result.ok() on success, Result.err on failure
     */
    Result<void> fileTransfer(const std::string& local_path,
                              const std::string& remote_path);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace omni_sdk
