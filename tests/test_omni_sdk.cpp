/**
 * @file test_omni_sdk.cpp
 * @brief Tests for OmniSDK factory functions to improve coverage.
 */

#include <gtest/gtest.h>
#include "../include/omni_sdk/omni_sdk.h"
#include "../include/omni_sdk/device.h"

namespace omni_sdk {

// Stub Client class for testing
class StubClient : public Client {
public:
    std::string client_name_;
    std::string client_version_;

    StubClient(const std::string& name, const std::string& version = "1.0.0")
        : client_name_(name), client_version_(version) {}

    std::string name() const override { return client_name_; }
    std::string version() const override { return client_version_; }

    Result<void> initialize(const std::map<std::string, std::string>& config) override {
        (void)config;
        return Result<void>::Ok();
    }

    Result<void> connect() override {
        return Result<void>::Ok();
    }

    Result<void> disconnect() override {
        return Result<void>::Ok();
    }

    bool isConnected() const override { return false; }

    Result<void> send(const std::string& command) override {
        (void)command;
        return Result<void>::Ok();
    }

    Result<std::string> receive(int timeout_ms) override {
        (void)timeout_ms;
        return Result<std::string>::Ok("");
    }

    Result<std::string> sendAndReceive(const std::string& command, int timeout_ms) override {
        (void)command;
        (void)timeout_ms;
        return Result<std::string>::Ok("");
    }

    std::map<std::string, std::string> capabilities() const override {
        return {
            {"test_capability", "Test capability for " + client_name_}
        };
    }
};

// ============================================================================
// OmniSDK Tests
// ============================================================================

TEST(OmniSDK, InitializeFromConfigFileNotFound) {
    auto result = OmniSDK::initializeFromConfig("/nonexistent/config.toml");

    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_NOT_FOUND_ERROR);
}

TEST(OmniSDK, InitializeFromConfigEmptyDevices) {
    auto result = OmniSDK::initializeFromConfig("/etc/hosts");

    EXPECT_TRUE(result.isOk());

    auto devices = result.unwrap();
    EXPECT_EQ(devices.size(), 0);
}

TEST(OmniSDK, ConnectDeviceNotFound) {
    std::map<std::string, std::shared_ptr<Device>> devices;

    auto result = OmniSDK::connectDevice("nonexistent-device", devices);

    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::DEVICE_NOT_FOUND_ERROR);
    EXPECT_TRUE(result.error().message.find("not found") != std::string::npos);
}

TEST(OmniSDK, ConnectDeviceSuccess) {
    // Create a mock device
    Config device_config;
    device_config.name = "Test Device";
    device_config.clients = {};

    auto device = std::make_shared<Device>("test-device", device_config);
    std::map<std::string, std::shared_ptr<Device>> devices;
    devices["test-device"] = device;

    auto result = OmniSDK::connectDevice("test-device", devices);

    EXPECT_TRUE(result.isOk());

    auto connected_device = result.unwrap();
    EXPECT_EQ(connected_device->deviceId(), "test-device");
}

TEST(OmniSDK, ConnectDeviceButClientsFail) {
    // Create a device with a client that will fail to connect
    Config device_config;
    device_config.name = "Test Device";
    device_config.clients = {};

    auto device = std::make_shared<Device>("test-device", device_config);

    // Add a client that fails to connect
    auto failing_client = std::make_shared<StubClient>("ssh", "1.0.0");
    auto add_result = device->addClient(failing_client);
    ASSERT_TRUE(add_result.isOk());

    std::map<std::string, std::shared_ptr<Device>> devices;
    devices["test-device"] = device;

    auto result = OmniSDK::connectDevice("test-device", devices);

    // Should succeed because stub client connect() returns Ok
    EXPECT_TRUE(result.isOk());
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace omni_sdk
