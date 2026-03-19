/**
 * @file config.h
 * @brief Configuration system using TOML files and validation.
 *
 * Defines schemas for device and client configuration.
 * Matches Python config.py functionality.
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include "result.h"

namespace omni_sdk {

/**
 * @brief Client configuration base class.
 */
struct ClientConfig {
    int timeout_ms = 5000;
};

/**
 * @brief SSH client configuration.
 */
struct SshConfig : ClientConfig {
    std::string host;
    int port = 22;
    std::string username;
    std::string password;  // Optional
    std::string password_file;  // Optional
    std::string key_file;  // Optional

    bool isValid() const {
        if (host.empty()) return false;
        if (port < 1 || port > 65535) return false;
        if (username.empty()) return false;
        if (password.empty() && key_file.empty()) return false;
        return true;
    }
};

/**
 * @brief Serial client configuration.
 */
struct SerialConfig : ClientConfig {
    std::string port;
    int baud_rate = 9600;
    int data_bits = 8;
    int stop_bits = 1;
    std::string parity = "none";
    std::string flow_control = "none";

    bool isValid() const {
        if (port.empty()) return false;
        std::vector<int> valid_rates = {9600, 19200, 38400, 57600, 115200, 230400};
        if (std::find(valid_rates.begin(), valid_rates.end(), baud_rate) == valid_rates.end()) {
            return false;
        }
        return true;
    }
};

/**
 * @brief HTTP client configuration.
 */
struct HttpConfig : ClientConfig {
    std::string base_url;
    std::map<std::string, std::string> headers;

    bool isValid() const {
        if (base_url.empty()) return false;
        return true;
    }
};

/**
 * @brief Device configuration.
 */
struct DeviceConfig {
    std::string id;
    std::string name;
    std::map<std::string, std::string> metadata;
    std::map<std::string, std::map<std::string, std::string>> clients;
};

/**
 * @brief Global SDK configuration.
 */
struct GlobalConfig {
    std::string log_level = "info";
    int default_timeout_ms = 5000;
    int retry_attempts = 3;
};

/**
 * @brief Top-level SDK configuration.
 */
struct SdkConfig {
    GlobalConfig global_config;
    std::vector<DeviceConfig> devices;
};

/**
 * @brief Configuration loader.
 *
 * Loads and parses TOML configuration files.
 * Returns Result<T> for all operations, aligning with SDK error handling.
 */
class ConfigLoader {
public:
    /**
     * @brief Load TOML file from path.
     * @param path Path to TOML configuration file
     * @return Result.ok(config_dict) on success, Result.err on failure
     *
     * Note: Currently returns a simple map of string to string.
     * TODO: Integrate TOML++ for proper parsing.
     */
    static Result<std::map<std::string, std::string>> load(const std::string& path);

    /**
     * @brief Validate configuration data and convert to SdkConfig.
     * @param data Configuration map
     * @return Result.ok(SdkConfig) on success, Result.err on validation failure
     */
    static Result<SdkConfig> validate(const std::map<std::string, std::string>& data);

    /**
     * @brief Load and validate configuration from file in one step.
     * @param path Path to TOML configuration file
     * @return Result.ok(SdkConfig) on success, Result.err on failure
     */
    static Result<SdkConfig> loadAndValidate(const std::string& path);

    /**
     * @brief Get client configuration for a specific device.
     * @param device_config Device configuration
     * @param client_name Client name (e.g., "ssh", "serial")
     * @return Result.ok(config_map) on success, Result.err if client not found
     */
    static Result<std::map<std::string, std::string>>
    getClientConfig(const DeviceConfig& device_config, const std::string& client_name);
};

} // namespace omni_sdk
