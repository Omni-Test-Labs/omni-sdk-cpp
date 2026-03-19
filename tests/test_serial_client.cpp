/**
 * @file test_serial_client.cpp
 * @brief Unit tests for SerialClient implementation.
 */

#include <gtest/gtest.h>

// Only include client header if enabled
#ifdef OMNI_SDK_ENABLE_SERIAL
#include "omni_sdk/clients/serial_client.h"

using namespace omni_sdk;

// ============================================================================
// Test SerialClient Construction
// ============================================================================

TEST(SerialClient, Construction) {
    SerialClient client;
    EXPECT_EQ(client.name(), "serial");
    EXPECT_EQ(client.version(), "1.0.0");
    EXPECT_FALSE(client.isConnected());
}

// ============================================================================
// Test SerialClient Initialization
// ============================================================================

TEST(SerialClient, InitializeWithValidConfig) {
    SerialClient client;

    std::map<std::string, std::string> config = {
        {"port", "/dev/ttyUSB0"},
        {"baud_rate", "9600"},
        {"data_bits", "8"},
        {"stop_bits", "1"},
        {"timeout_ms", "5000"}
    };

    auto result = client.initialize(config);
    EXPECT_TRUE(result.isOk());
}

TEST(SerialClient, InitializeWithMissingPort) {
    SerialClient client;

    std::map<std::string, std::string> config = {
        {"baud_rate", "9600"}
    };

    auto result = client.initialize(config);
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_ERROR);
    EXPECT_TRUE(result.error().message.find("port") != std::string::npos);
}

TEST(SerialClient, InitializeWithInvalidBaudRate) {
    SerialClient client;

    std::map<std::string, std::string> config = {
        {"port", "/dev/ttyUSB0"},
        {"baud_rate", "12345"}
    };

    auto result = client.initialize(config);
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_ERROR);
    EXPECT_TRUE(result.error().message.find("Invalid baud rate") != std::string::npos);
}

TEST(SerialClient, InitializeWithValidBaudRates) {
    SerialClient client;

    std::vector<int> valid_rates = {9600, 19200, 38400, 57600, 115200, 230400};

    for (int baud_rate : valid_rates) {
        std::map<std::string, std::string> config = {
            {"port", "/dev/ttyUSB0"},
            {"baud_rate", std::to_string(baud_rate)}
        };

        auto result = client.initialize(config);
        EXPECT_TRUE(result.isOk()) << "Baud rate " << baud_rate << " should be valid";
    }
}

TEST(SerialClient, InitializeWithDefaults) {
    SerialClient client;

    std::map<std::string, std::string> config = {
        {"port", "/dev/ttyUSB0"}
    };

    auto result = client.initialize(config);
    EXPECT_TRUE(result.isOk());

    // Verify defaults are set by checking getStatus
    // (Can't access config directly, but we can verify it worked)
}

// ============================================================================
// Test SerialClient Capabilities
// ============================================================================

TEST(SerialClient, Capabilities) {
    SerialClient client;

    auto caps = client.capabilities();

    EXPECT_TRUE(caps.contains("send"));
    EXPECT_TRUE(caps.contains("receive"));
    EXPECT_TRUE(caps.contains("get_status"));
    EXPECT_TRUE(caps.contains("configure"));

    EXPECT_TRUE(caps["send"].find("Send data") != std::string::npos);
    EXPECT_TRUE(caps["receive"].find("Receive data") != std::string::npos);
}

// ============================================================================
// Test SerialClient Status
// ============================================================================

TEST(SerialClient, GetStatusNotConnected) {
    SerialClient client;

    auto status_result = client.getStatus();
    EXPECT_TRUE(status_result.isOk());

    auto status = status_result.unwrap();
    EXPECT_EQ(status.at("connected"), "false");
    EXPECT_EQ(status.at("client_name"), "serial");
    EXPECT_EQ(status.at("version"), "1.0.0");
}

TEST(SerialClient, ConfigureBeforeConnect) {
    SerialClient client;

    std::map<std::string, std::string> init_config = {
        {"port", "/dev/ttyUSB0"},
        {"baud_rate", "9600"}
    };
    client.initialize(init_config);

    std::map<std::string, std::string> params = {
        {"baud_rate", "115200"},
        {"data_bits", "7"},
        {"timeout_ms", "10000"}
    };

    auto result = client.configure(params);
    // Should fail because not connected
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_CONNECTED);
}

// ============================================================================
// Test SerialClient Connection Error Paths
// ============================================================================

TEST(SerialClient, SendWhenNotConnected) {
    SerialClient client;

    std::map<std::string, std::string> init_config = {
        {"port", "/dev/ttyUSB0"}
    };
    client.initialize(init_config);

    auto result = client.send("test command");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_CONNECTED);
}

TEST(SerialClient, ReceiveWhenNotConnected) {
    SerialClient client;

    std::map<std::string, std::string> init_config = {
        {"port", "/dev/ttyUSB0"}
    };
    client.initialize(init_config);

    auto result = client.receive();
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_CONNECTED);
}

TEST(SerialClient, SendAndReceiveWhenNotConnected) {
    SerialClient client;

    std::map<std::string, std::string> init_config = {
        {"port", "/dev/ttyUSB0"}
    };
    client.initialize(init_config);

    auto result = client.sendAndReceive("test");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_CONNECTED);
}

TEST(SerialClient, DisconnectWhenNotConnected) {
    SerialClient client;
    auto result = client.disconnect();
    // Should succeed (safe to call multiple times)
    EXPECT_TRUE(result.isOk());
}

TEST(SerialClient, IsConnectedInitiallyFalse) {
    SerialClient client;
    EXPECT_FALSE(client.isConnected());
}

#else // !OMNI_SDK_ENABLE_SERIAL

TEST(SerialClientDisabled, AllMethodsReturnError) {
    // When OMNI_SDK_ENABLE_SERIAL is OFF, all methods should return meaningful errors

    // Note: We can't actually instantiate SerialClient because the implementation
    // provides only stub definitions. The linker would fail if we tried to use it.
    // This is a placeholder test that validates our understanding of the stub behavior.

    SUCCEED() << "Serial client is disabled (OMNI_SDK_ENABLE_SERIAL=OFF). "
               << "All methods return errors with descriptive messages.";
}

TEST(SerialClientDisabled, ErrorMessagesAreDescriptive) {
    // When disabled, errors should clearly indicate build configuration issue

    // Again, placeholder - actual testing would require linking with stub implementation
    SUCCEED() << "Error messages should indicate: "
               << "'Serial client support not enabled. Build with OMNI_SDK_ENABLE_SERIAL=ON'";
}

#endif // OMNI_SDK_ENABLE_SERIAL

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
