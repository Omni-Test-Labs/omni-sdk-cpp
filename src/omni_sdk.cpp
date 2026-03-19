/**
 * @file omni_sdk.cpp
 * @brief Omni SDK factory functions.
 */

#include "omni_sdk/omni_sdk.h"
#include "omni_sdk/device.h"
#include "omni_sdk/config.h"

namespace omni_sdk {

    Result<std::map<std::string, std::shared_ptr<Device>>>
    OmniSDK::initializeFromConfig(const std::string& config_path) {
        // Load config
        auto load_result = ConfigLoader::load(config_path);
        if (load_result.isErr()) {
            return Result<std::map<std::string, std::shared_ptr<Device>>>::Err(
                load_result.error()
            );
        }

        auto config_data = load_result.value();

        // Validate
        auto validate_result = ConfigLoader::validate(config_data);
        if (validate_result.isErr()) {
            return Result<std::map<std::string, std::shared_ptr<Device>>>::Err(
                validate_result.error()
            );
        }

        auto config = validate_result.value();

        // Create devices
        std::map<std::string, std::shared_ptr<Device>> devices;

        for (const auto& device_config : config.devices) {
            auto device = std::make_shared<Device>(device_config.id, device_config);

            // TODO: Add clients to device
            // auto client = std::make_shared<ClientType>();
            // device->addClient(client);

            devices[device_config.id] = device;
        }

        return Result<std::map<std::string, std::shared_ptr<Device>>>::Ok(devices);
    }

    Result<std::shared_ptr<Device>>
    OmniSDK::connectDevice(const std::string& device_id,
                           const std::map<std::string, std::shared_ptr<Device>>& devices) {
        auto it = devices.find(device_id);
        if (it == devices.end()) {
            return Result<std::shared_ptr<Device>>::Err(Error{
                ErrorKinds::DEVICE_NOT_FOUND_ERROR,
                "Device '" + device_id + "' not found"
            });
        }

        auto device = it->second;
        auto connect_result = device->connectAll();
        if (connect_result.isErr()) {
            return Result<std::shared_ptr<Device>>::Err(connect_result.error());
        }

        return Result<std::shared_ptr<Device>>::Ok(device);
    }

} // namespace omni_sdk
