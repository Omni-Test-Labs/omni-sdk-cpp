/**
 * @file omni_sdk.h
 * @brief Omni SDK public API - factory functions and utilities.
 */

#pragma once

#include <string>
#include <map>
#include <memory>
#include "device.h"
#include "config.h"
#include "result.h"

namespace omni_sdk {

/**
 * @brief Omni SDK factory class.
 *
 * Provides factory functions for initializing the SDK from configuration.
 */
class OmniSDK {
public:
    /**
     * @brief Initialize SDK from configuration file.
     * @param config_path Path to TOML configuration file
     * @return Result.ok(device_map) on success, Result.err on failure
     *
     * Example:
     *     auto result = OmniSDK::initializeFromConfig("devices.toml");
     *     if (result.isOk()) {
     *         auto devices = result.value();
     *         // Use devices...
     *     }
     */
    static Result<std::map<std::string, std::shared_ptr<Device>>>
    initializeFromConfig(const std::string& config_path);

    /**
     * @brief Connect to a specific device.
     * @param device_id Device identifier
     * @param devices Device map from initializeFromConfig
     * @return Result.ok(device) on success, Result.err on failure
     */
    static Result<std::shared_ptr<Device>>
    connectDevice(const std::string& device_id,
                   const std::map<std::string, std::shared_ptr<Device>>& devices);
};

} // namespace omni_sdk
