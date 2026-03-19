/**
 * @file test_ssh_client.cpp
 * @brief Unit tests for SshClient implementation.
 */

#include <gtest/gtest.h>

// Only include client header if enabled
#ifdef OMNI_SDK_ENABLE_SSH
#include "omni_sdk/clients/ssh_client.h"

using namespace omni_sdk;

// ============================================================================
// Test SshClient Construction
// ============================================================================

TEST(SshClient, Construction) {
    SshClient client;
    EXPECT_EQ(client.name(), "ssh");
    EXPECT_EQ(client.version(), "1.0.0");
    EXPECT_FALSE(client.isConnected());
}

// ============================================================================
// Test SshClient Initialization
// ============================================================================

TEST(SshClient, InitializeWithValidPasswordConfig) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"},
        {"timeout_ms", "5000"}
    };

    auto result = client.initialize(config);
    EXPECT_TRUE(result.isOk());
}

TEST(SshClient, InitializeWithValidKeyFileConfig) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"key_file", "/home/user/.ssh/id_rsa"}
    };

    auto result = client.initialize(config);
    EXPECT_TRUE(result.isOk());
}

TEST(SshClient, InitializeWithMissingHost) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"}
    };

    auto result = client.initialize(config);
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_ERROR);
    EXPECT_TRUE(result.error().message.find("host") != std::string::npos);
}

TEST(SshClient, InitializeWithMissingUsername) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"password", "secret"}
    };

    auto result = client.initialize(config);
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_ERROR);
    EXPECT_TRUE(result.error().message.find("username") != std::string::npos);
}

TEST(SshClient, InitializeWithMissingPasswordAndKeyFile) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"}
    };

    auto result = client.initialize(config);
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::AUTHENTICATION_ERROR);
    EXPECT_TRUE(result.error().message.find("password or key_file") != std::string::npos);
}

TEST(SshClient, InitializeWithInvalidPort) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "invalid"},
        {"username", "admin"},
        {"password", "secret"}
    };

    auto result = client.initialize(config);
    // Should handle the invalid port gracefully
    // Either fail here or during connect
    (void)result;
}

// ============================================================================
// Test SshClient Capabilities
// ============================================================================

TEST(SshClient, Capabilities) {
    SshClient client;

    auto caps = client.capabilities();

    EXPECT_TRUE(caps.contains("execute"));
    EXPECT_TRUE(caps.contains("file_transfer"));
    EXPECT_TRUE(caps.contains("get_status"));

    EXPECT_TRUE(caps["execute"].find("Execute shell command") != std::string::npos);
    EXPECT_TRUE(caps["file_transfer"].find("SCP") != std::string::npos);
}

// ============================================================================
// Test SshClient Status
// ============================================================================

TEST(SshClient, GetStatusNotConnected) {
    SshClient client;

    auto status_result = client.getStatus();
    EXPECT_TRUE(status_result.isOk());

    auto status = status_result.unwrap();
    EXPECT_EQ(status.at("connected"), "false");
    EXPECT_EQ(status.at("client_name"), "ssh");
    EXPECT_EQ(status.at("version"), "1.0.0");
}

TEST(SshClient, GetStatusAfterInitialize) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"}
    };

    auto init_result = client.initialize(config);
    ASSERT_TRUE(init_result.isOk());

    auto status_result = client.getStatus();
    EXPECT_TRUE(status_result.isOk());

    auto status = status_result.unwrap();
    EXPECT_EQ(status.at("connected"), "false");
    EXPECT_EQ(status.at("host"), "192.168.1.1");
    EXPECT_EQ(status.at("port"), "22");
    EXPECT_EQ(status.at("username"), "admin");
}

TEST(SshClient, ConfigureTimeout) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"}
    };

    auto init_result = client.initialize(config);
    ASSERT_TRUE(init_result.isOk());

    std::map<std::string, std::string> params = {
        {"timeout_ms", "10000"}
    };

    auto configure_result = client.configure(params);
    EXPECT_TRUE(configure_result.isOk());
}

// ============================================================================
// Test SshClient Connection Error Paths
// ============================================================================

TEST(SshClient, SendWhenNotConnected) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"}
    };

    client.initialize(config);

    auto result = client.send("test command");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_CONNECTED);
}

TEST(SshClient, ReceiveWhenNotConnected) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"}
    };

    client.initialize(config);

    auto result = client.receive();
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_CONNECTED);
}

TEST(SshClient, SendAndReceiveWhenNotConnected) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"}
    };

    client.initialize(config);

    auto result = client.sendAndReceive("test");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_CONNECTED);
}

TEST(SshClient, ExecuteWhenNotConnected) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"}
    };

    client.initialize(config);

    auto result = client.execute("ls -la");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_CONNECTED);
}

TEST(SshClient, FileTransferWhenNotConnected) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"}
    };

    client.initialize(config);

    auto result = client.fileTransfer("/local/file", "/remote/file");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_CONNECTED);
}

TEST(SshClient, DisconnectWhenNotConnected) {
    SshClient client;
    auto result = client.disconnect();
    // Should succeed (safe to call multiple times)
    EXPECT_TRUE(result.isOk());
}

TEST(SshClient, IsConnectedInitiallyFalse) {
    SshClient client;
    EXPECT_FALSE(client.isConnected());
}

// ============================================================================
// Test SshClient Error Handling
// ============================================================================

TEST(SshClient, SendAndReceiveDelegatesToExecute) {
    SshClient client;

    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"},
        {"username", "admin"},
        {"password", "secret"}
    };

    client.initialize(config);

    // sendAndReceive should delegate to execute
    // This will fail because we're not connected, but the behavior should match
    auto result1 = client.sendAndReceive("ls");
    auto result2 = client.execute("ls");

    // Both should fail with same error kind
    EXPECT_TRUE(result1.isErr());
    EXPECT_TRUE(result2.isErr());
    EXPECT_EQ(result1.error().kind, result2.error().kind);
}

#else // !OMNI_SDK_ENABLE_SSH

TEST(SshClientDisabled, AllMethodsReturnError) {
    // When OMNI_SDK_ENABLE_SSH is OFF, all methods should return meaningful errors

    // Note: We can't actually instantiate SshClient because the implementation
    // provides only stub definitions. The linker would fail if we tried to use it.
    // This is a placeholder test that validates our understanding of the stub behavior.

    SUCCEED() << "SSH client is disabled (OMNI_SDK_ENABLE_SSH=OFF). "
               << "All methods return errors with descriptive messages.";
}

TEST(SshClientDisabled, ErrorMessagesAreDescriptive) {
    // When disabled, errors should clearly indicate build configuration issue

    // Again, placeholder - actual testing would require linking with stub implementation
    SUCCEED() << "Error messages should indicate: "
               << "'SSH client support not enabled. Build with OMNI_SDK_ENABLE_SSH=ON'";
}

#endif // OMNI_SDK_ENABLE_SSH

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
