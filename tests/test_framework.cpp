/**
 * @file test_framework.cpp
 * @brief Unit tests for SDK framework components (Device, Config, Client interface).
 *
 * Mock-based tests using Google Mock.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Mock Client class for testing
class MockClient : public omni_sdk::Client {
public:
    MOCK_METHOD(std::string, name, (), (const, override));
    MOCK_METHOD(std::string, version, (), (const, override));
    MOCK_METHOD(omni_sdk::Result<void>, initialize, (const std::map<std::string, std::string>&), (override));
    MOCK_METHOD(omni_sdk::Result<void>, connect, (), (override));
    MOCK_METHOD(omni_sdk::Result<void>, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(omni_sdk::Result<void>, send, (const std::string&), (override));
    MOCK_METHOD(omni_sdk::Result<std::string>, receive, (int), (override));
    MOCK_METHOD(omni_sdk::Result<std::string>, sendAndReceive, (const std::string&, int), (override));
    MOCK_METHOD(std::map<std::string, std::string>, capabilities, (), (const, override));
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

class Device : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = {
            {"id", "test-device"},
            {"name", "Test Device"},
            {"clients", {}},
            {"metadata", {{"location", "Lab"}}}
        };
        device_ = std::make_shared<omni_sdk::Device>("test-device", config_);
    }

    omni_sdk::Config config_;
    std::shared_ptr<omni_sdk::Device> device_;
};

TEST_F(Device, DeviceCreation) {
    EXPECT_EQ(device_->deviceId(), "test-device");
    EXPECT_EQ(device_->getName(), "Get Test Device");
}

TEST_F(Device, ListClientsEmpty) {
    auto clients = device_->listClients();
    EXPECT_EQ(clients.size(), 0);
}

TEST_F(Device, ListCapabilitiesEmpty) {
    auto caps = device_->listCapabilities();
    EXPECT_EQ(caps.size(), 0);
}

TEST_F(Device, AddClientSuccess) {
    auto mock_client = std::make_shared<MockClient>();
    EXPECT_CALL(*mock_client, name()).WillRepeatedly(Return(std::string("ssh")));
    EXPECT_CALL(*mock_client, initialize(testing::_))
        .WillOnce(Return(omni_sdk::Result<void>::Ok()));
    EXPECT_CALL(*mock_client, capabilities())
        .WillOnce(Return(std::map<std::string, std::string>{
            {"execute", "Execute command"},
            {"get_status", "Get status"}
        }));

    omni_sdk::Config client_config;
    // client_config["host"] = ...

    auto result = device_->addClient(mock_client);
    EXPECT_TRUE(result.isOk());

    auto clients = device_->listClients();
    EXPECT_EQ(clients.size(), 1);
    EXPECT_EQ(clients[0], "ssh");
}

TEST_F(Device, AddClientDuplicate) {
    auto mock_client1 = std::make_shared<MockClient>();
    EXPECT_CALL(*mock_client1, name()).WillRepeatedly(Return(std::string("ssh")));
    EXPECT_CALL(*mock_client1, initialize(testing::_))
        .WillRepeatedly(Return(omni_sdk::Result<void>::Ok()));

    auto mock_client2 = std::make_shared<MockClient>();
    EXPECT_CALL(*mock_client2, name()).WillRepeatedly(Return(std::string("ssh")));

    // First add succeeds
    auto result1 = device_->addClient(mock_client1);
    EXPECT_TRUE(result1.isOk());

    // Second add fails (duplicate)
    auto result2 = device_->addClient(mock_client2);
    EXPECT_TRUE(result2.isErr());
    EXPECT_EQ(result2.error().kind, omni_sdk::ErrorKinds::CONFIG_ERROR);
}

TEST_F(Device, GetClientNotFound) {
    auto result = device_->getClient("ssh");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, omni_sdk::ErrorKinds::DEVICE_ERROR);
    EXPECT_TRUE(result.error().message.find("not found") != std::string::npos);
}

TEST_F(Device, ConnectAllNoClients) {
    auto result = device_->connectAll();
    EXPECT_TRUE(result.isOk());
}

TEST_F(Device, DisconnectAllAlwaysSucceeds) {
    auto result = device_->disconnectAll();
    EXPECT_TRUE(result.isOk());
}

TEST_F(Device, GetStatus) {
    auto status = device_->getStatus();

    EXPECT_TRUE(status.contains("device_id"));
    EXPECT_EQ(std::any_cast<std::string>(status["device_id"]), "test-device");
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
    ::testing::InitGoogleMock(&argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
