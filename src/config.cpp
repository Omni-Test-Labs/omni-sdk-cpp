/**
 * @file config.cpp
 * @brief Configuration loader implementation.
 */

#include "omni_sdk/config.h"
#include "omni_sdk/result.h"
#include <fstream>
#include <sstream>
#include <map>

namespace omni_sdk {

Result<std::map<std::string, std::string>> ConfigLoader::load(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return Result<std::map<std::string, std::string>>::Err(Error{
                ErrorKinds::CONFIG_NOT_FOUND_ERROR,
                "Config file not found: " + path
            });
        }

        // For now, just read all lines and return an empty map
        // TODO: Parse TOML file properly
        return Result<std::map<std::string, std::string>>::Ok({});

    } catch (const std::exception& e) {
        return Result<std::map<std::string, std::string>>::Err(Error{
            ErrorKinds::CONFIG_ERROR,
            "Failed to load config: " + std::string(e.what())
        });
    }
}

Result<SdkConfig> ConfigLoader::validate(const std::map<std::string, std::string>& data) {
    try {
        SdkConfig config;

        // TODO: Parse configuration from data map
        // For now, return empty config

        return Result<SdkConfig>::Ok(config);
    } catch (const std::exception& e) {
        return Result<SdkConfig>::Err(Error{
            ErrorKinds::CONFIG_VALIDATION_ERROR,
            "Config validation failed: " + std::string(e.what())
        });
    }
}

Result<SdkConfig> ConfigLoader::loadAndValidate(const std::string& path) {
    auto loadResult = load(path);
    if (loadResult.isErr()) {
        return Result<SdkConfig>::Err(loadResult.error());
    }

    return validate(loadResult.value());
}

Result<std::map<std::string, std::string>>
ConfigLoader::getClientConfig(const DeviceConfig& device_config, const std::string& client_name) {
    (void)device_config;
    (void)client_name;

    // TODO: Implement client config lookup
    // For now, return error if clients empty
    if (device_config.clients.empty()) {
        return Result<std::map<std::string, std::string>>::Err(Error{
            ErrorKinds::CONFIG_ERROR,
            "No clients configured for device"
        });
    }

    auto it = device_config.clients.find(client_name);
    if (it == device_config.clients.end()) {
        return Result<std::map<std::string, std::string>>::Err(Error{
            ErrorKinds::CONFIG_ERROR,
            "Client '" + client_name + "' not found in device config"
        });
    }

    return Result<std::map<std::string, std::string>>::Ok(it->second);
}

} // namespace omni_sdk
