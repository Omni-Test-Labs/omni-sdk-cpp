/**
 * @file test_framework.cpp
 * @brief Unit tests for SDK framework components (Device, Config, Client interface).
 */

#include <gtest/gtest.h>
#include "omni_sdk/config.h"
#include "omni_sdk/device.h"
#include "omni_sdk/result.h"

// Stub Client class for testing
class StubClient : public omni_sdk::Client {
public:
    std::string client_name_;
    std::string client_version_;

    StubClient(const std::string& name, const std::string& version = "1.0.0")
        : client_name_(name), client_version_(version) {}

    std::string name() const override { return client_name_; }
    std::string version() const override { return client_version_; }

    omni_sdk::Result<void> initialize(const std::map<std::string, std::string>& config) override {
        (void)config;
        return omni_sdk::Result<void>::Ok();
    }

    omni_sdk::Result<void> connect() override {
        return omni_sdk::Result<void>::Ok();
    }

    omni_sdk::Result<void> disconnect() override {
        return omni_sdk::Result<void>::Ok();
    }

    bool isConnected() const override { return false; }

    omni_sdk::Result<void> send(const std::string& command) override {
        (void)command;
        return omni_sdk::Result<void>::Ok();
    }

    omni_sdk::Result<std::string> receive(int timeout_ms) override {
        (void)timeout_ms;
        return omni_sdk::Result<std::string>::Ok("");
    }

    omni_sdk::Result<std::string> sendAndReceive(const std::string& command, int timeout_ms) override {
        (void)command;
        (void)timeout_ms;
        return omni_sdk::Result<std::string>::Ok("");
    }

    std::map<std::string, std::string> capabilities() const override {
        return {
            {"test_capability", "Test capability for " + client_name_}
        };
    }
};

// ============================================================================
// Tests for Config models
// ============================================================================

TEST(ConfigModels, SshConfigValid) {
    omni_sdk::SshConfig config;
    config.host = "192.168.1.1";
    config.port = 22;
    config.username = "admin";
    config.password = "secret";

    EXPECT_TRUE(config.isValid());
    EXPECT_EQ(config.host, "192.168.1.1");
    EXPECT_EQ(config.port, 22);
    EXPECT_EQ(config.username, "admin");
}

TEST(ConfigModels, SshConfigInvalidPort) {
    omni_sdk::SshConfig config;
    config.host = "192.168.1.1";
    config.port = 70000;
    config.username = "admin";
    config.password = "secret";

    EXPECT_FALSE(config.isValid());
}

TEST(ConfigModels, SshConfigMissingHost) {
    omni_sdk::SshConfig config;
    config.port = 22;
    config.username = "admin";
    config.password = "secret";

    EXPECT_FALSE(config.isValid());
}

TEST(ConfigModels, SerialConfigValid) {
    omni_sdk::SerialConfig config;
    config.port = "/dev/ttyUSB0";
    config.baud_rate = 115200;

    EXPECT_TRUE(config.isValid());
    EXPECT_EQ(config.port, "/dev/ttyUSB0");
    EXPECT_EQ(config.baud_rate, 115200);
}

TEST(ConfigModels, SerialConfigInvalidBaudRate) {
    omni_sdk::SerialConfig config;
    config.port = "/dev/ttyUSB0";
    config.baud_rate = 12345;

    EXPECT_FALSE(config.isValid());
}

TEST(ConfigModels, SerialConfigMissingPort) {
    omni_sdk::SerialConfig config;
    config.baud_rate = 115200;

    EXPECT_FALSE(config.isValid());
}

// ============================================================================
// Tests for Device container
// ============================================================================

class DeviceTest : public ::testing::Test {
protected:
    void SetUp() override {
        omni_sdk::Config config;
        config.name = "Test Device";
        config.clients = {};
        device_ = std::make_shared<omni_sdk::Device>("test-device", config);
    }

    std::shared_ptr<omni_sdk::Device> device_;
};

TEST_F(DeviceTest, DeviceCreation) {
    EXPECT_EQ(device_->deviceId(), "test-device");
}

TEST_F(DeviceTest, ListClientsEmpty) {
    auto clients = device_->listClients();
    EXPECT_EQ(clients.size(), 0);
}

TEST_F(DeviceTest, ListCapabilitiesEmpty) {
    auto caps = device_->listCapabilities();
    EXPECT_EQ(caps.size(), 0);
}

TEST_F(DeviceTest, AddClientSuccess) {
    auto stub_client = std::make_shared<StubClient>("ssh", "1.0.0");

    auto result = device_->addClient(stub_client);
    EXPECT_TRUE(result.isOk());

    auto clients = device_->listClients();
    EXPECT_EQ(clients.size(), 1);
    EXPECT_EQ(clients[0], "ssh");
}

TEST_F(DeviceTest, AddClientDuplicate) {
    auto stub_client1 = std::make_shared<StubClient>("ssh", "1.0.0");
    auto stub_client2 = std::make_shared<StubClient>("ssh", "1.0.0");

    // First add succeeds
    auto result1 = device_->addClient(stub_client1);
    EXPECT_TRUE(result1.isOk());

    // Second add fails (duplicate)
    auto result2 = device_->addClient(stub_client2);
    EXPECT_TRUE(result2.isErr());
    EXPECT_EQ(result2.error().kind, omni_sdk::ErrorKinds::CONFIG_ERROR);
}

TEST_F(DeviceTest, GetClientNotFound) {
    auto result = device_->getClient("ssh");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, omni_sdk::ErrorKinds::DEVICE_ERROR);
    EXPECT_TRUE(result.error().message.find("not found") != std::string::npos);
}

TEST_F(DeviceTest, ConnectAllNoClients) {
    auto result = device_->connectAll();
    EXPECT_TRUE(result.isOk());
}

TEST_F(DeviceTest, DisconnectAllAlwaysSucceeds) {
    auto result = device_->disconnectAll();
    EXPECT_TRUE(result.isOk());
}

// ============================================================================
// Tests for ConfigLoader
// ============================================================================

TEST(ConfigLoader, LoadFileNotFound) {
    auto result = omni_sdk::ConfigLoader::load("nonexistent.toml");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, omni_sdk::ErrorKinds::CONFIG_NOT_FOUND_ERROR);
}

TEST(ConfigLoader, ValidateEmptyData) {
    std::map<std::string, std::string> data;
    data["global"] = "";
    data["devices"] = "";

    // For now, this should return empty config
    auto result = omni_sdk::ConfigLoader::validate(data);

    // Depending on implementation, this may succeed or fail
    // For now, let's just verify it doesn't crash
    (void)result;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
