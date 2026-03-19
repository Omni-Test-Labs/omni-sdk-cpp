/**
 * @file test_config.cpp
 * @brief Additional tests for configuration system to improve coverage.
 */

#include <gtest/gtest.h>
#include "../include/omni_sdk/config.h"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>

namespace omni_sdk {

// ============================================================================
// Additional ConfigLoader Tests for Coverage
// ============================================================================

TEST(ConfigLoaderCoverage, LoadSuccess) {
    // Create a temporary config file
    std::string temp_file = "/tmp/test_config_temp.toml";
    std::ofstream file(temp_file);
    ASSERT_TRUE(file.is_open());
    file << "# Test config\n";
    file << "[devices]\n";
    file.close();

    auto result = ConfigLoader::load(temp_file);
    EXPECT_TRUE(result.isOk());

    // Cleanup
    unlink(temp_file.c_str());
}

TEST(ConfigLoaderCoverage, LoadWithValidData) {
    auto result = ConfigLoader::load("/etc/hosts");  // Use existing file
    // File exists, should succeed with empty map (stub implementation)
    EXPECT_TRUE(result.isOk());
}

TEST(ConfigLoaderCoverage, LoadWithNonExistentPath) {
    auto result = ConfigLoader::load("/nonexistent/path/to/file.toml");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_NOT_FOUND_ERROR);
    EXPECT_TRUE(result.error().message.find("Config file not found") != std::string::npos);
}

TEST(ConfigLoaderCoverage, LoadAndValidateSuccess) {
    auto result = ConfigLoader::loadAndValidate("/etc/hosts");
    EXPECT_TRUE(result.isOk());

    auto config = result.unwrap();
    EXPECT_EQ(config.devices.size(), 0);
}

TEST(ConfigLoaderCoverage, LoadAndValidateFailure) {
    auto result = ConfigLoader::loadAndValidate("/nonexistent/file.toml");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_NOT_FOUND_ERROR);
}

TEST(ConfigLoaderCoverage, GetClientConfigWithClients) {
    DeviceConfig device_config;
    device_config.id = "test-device";
    device_config.name = "Test Device";
    device_config.clients = {
        {"ssh", {{"host", "192.168.1.1"}, {"port", "22"}}},
        {"serial", {{"port", "/dev/ttyUSB0"}, {"baud_rate", "9600"}}}
    };

    auto result = ConfigLoader::getClientConfig(device_config, "ssh");
    EXPECT_TRUE(result.isOk());

    auto client_config = result.unwrap();
    EXPECT_EQ(client_config.at("host"), "192.168.1.1");
    EXPECT_EQ(client_config.at("port"), "22");
}

TEST(ConfigLoaderCoverage, GetClientConfigNotFound) {
    DeviceConfig device_config;
    device_config.id = "test-device";
    device_config.name = "Test Device";
    device_config.clients = {
        {"ssh", {{"host", "192.168.1.1"}}}
    };

    auto result = ConfigLoader::getClientConfig(device_config, "serial");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_ERROR);
    EXPECT_TRUE(result.error().message.find("not found") != std::string::npos);
}

TEST(ConfigLoaderCoverage, GetClientConfigEmptyClients) {
    DeviceConfig device_config;
    device_config.id = "test-device";
    device_config.name = "Test Device";
    device_config.clients = {};

    auto result = ConfigLoader::getClientConfig(device_config, "ssh");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_ERROR);
    EXPECT_TRUE(result.error().message.find("No clients configured") != std::string::npos);
}

TEST(ConfigLoaderCoverage, ValidateSuccess) {
    std::map<std::string, std::string> data;
    data["global"] = "";
    data["devices"] = "";

    auto result = ConfigLoader::validate(data);
    EXPECT_TRUE(result.isOk());

    auto config = result.unwrap();
    EXPECT_EQ(config.devices.size(), 0);
    EXPECT_EQ(config.global_config.log_level, "info");
}

TEST(ConfigLoaderCoverage, ValidateWithGlobalConfig) {
    std::map<std::string, std::string> data;
    data["log_level"] = "debug";
    data["devices"] = "";

    auto result = ConfigLoader::validate(data);
    EXPECT_TRUE(result.isOk());
}

// ============================================================================
// Additional Config Model Tests
// ============================================================================

TEST(ConfigModelsCoverage, SshConfigWithKeyFile) {
    SshConfig config;
    config.host = "192.168.1.1";
    config.port = 22;
    config.username = "admin";
    config.key_file = "/home/user/.ssh/id_rsa";
    config.timeout_ms = 10000;

    EXPECT_TRUE(config.isValid());
    EXPECT_EQ(config.key_file, "/home/user/.ssh/id_rsa");
}

TEST(ConfigModelsCoverage, SshConfigWithPasswordFile) {
    SshConfig config;
    config.host = "192.168.1.1";
    config.port = 22;
    config.username = "admin";
    // password_file alone is not valid for isValid(), but it's a valid field
    config.password_file = "/path/to/password.txt";
    config.password = "password"; // Need this for isValid()

    EXPECT_TRUE(config.isValid());
    EXPECT_EQ(config.password_file, "/path/to/password.txt");
}

TEST(ConfigModelsCoverage, SshConfigEdgeCases) {
    SshConfig config;

    // Minimum valid config
    config.host = "1.1.1.1";
    config.port = 1;
    config.username = "a";
    config.password = "x";
    EXPECT_TRUE(config.isValid());

    // Maximum valid port
    config.port = 65535;
    EXPECT_TRUE(config.isValid());

    // Invalid: port too high
    config.port = 65536;
    EXPECT_FALSE(config.isValid());

    // Invalid: port zero
    config.port = 0;
    EXPECT_FALSE(config.isValid());

    // Invalid: port negative
    config.port = -1;
    EXPECT_FALSE(config.isValid());

    // Empty credentials
    config.port = 22;
    config.password = "";
    config.key_file = "";
    EXPECT_FALSE(config.isValid());
}

TEST(ConfigModelsCoverage, SerialConfigAllParameters) {
    SerialConfig config;
    config.port = "/dev/ttyUSB0";
    config.baud_rate = 115200;
    config.data_bits = 8;
    config.stop_bits = 1;
    config.parity = "none";
    config.flow_control = "none";
    config.timeout_ms = 5000;

    EXPECT_TRUE(config.isValid());
}

TEST(ConfigModelsCoverage, SerialConfigEdgeCases) {
    SerialConfig config;

    // Test all valid baud rates
    std::vector<int> valid_rates = {9600, 19200, 38400, 57600, 115200, 230400};
    for (int rate : valid_rates) {
        config.baud_rate = rate;
        config.port = "/dev/ttyUSB0";
        EXPECT_TRUE(config.isValid()) << "Baud rate " << rate << " should be valid";
    }

    // Invalid baud rate near valid values
    config.baud_rate = 9599;
    EXPECT_FALSE(config.isValid());

    config.baud_rate = 230401;
    EXPECT_FALSE(config.isValid());

    // Invalid data bits
    config.baud_rate = 9600;
    config.data_bits = 5;
    // isValid only checks baud rate and port, so this is OK
    EXPECT_TRUE(config.isValid());
}

TEST(ConfigModelsCoverage, SerialConfigEmptyPort) {
    SerialConfig config;
    config.baud_rate = 9600;
    // Don't set port

    EXPECT_FALSE(config.isValid());
}

TEST(ConfigModelsCoverage, HttpConfigValid) {
    HttpConfig config;
    config.base_url = "https://api.example.com";
    config.headers = {{"Authorization", "Bearer token"}, {"Content-Type", "application/json"}};

    EXPECT_TRUE(config.isValid());
    EXPECT_EQ(config.base_url, "https://api.example.com");
}

TEST(ConfigModelsCoverage, HttpConfigEmptyBaseUrl) {
    HttpConfig config;
    config.headers = {{"Authorization", "Bearer token"}};

    EXPECT_FALSE(config.isValid());
}

TEST(ConfigModelsCoverage, GlobalConfigDefaults) {
    GlobalConfig config;

    EXPECT_EQ(config.log_level, "info");
    EXPECT_EQ(config.default_timeout_ms, 5000);
    EXPECT_EQ(config.retry_attempts, 3);
}

TEST(ConfigModelsCoverage, DeviceConfigFull) {
    DeviceConfig config;
    config.id = "device-001";
    config.name = "Test Device";
    config.metadata = {{"location", "Lab"}, {"owner", "John Doe"}};
    config.clients = {
        {"ssh", {{"host", "192.168.1.1"}}},
        {"serial", {{"port", "/dev/ttyUSB0"}}}
    };

    EXPECT_EQ(config.id, "device-001");
    EXPECT_EQ(config.name, "Test Device");
    EXPECT_EQ(config.clients.size(), 2);
    EXPECT_EQ(config.clients["ssh"]["host"], "192.168.1.1");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace omni_sdk
