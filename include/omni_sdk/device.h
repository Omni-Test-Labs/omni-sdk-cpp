/**
 * @file device.h
 * @brief Device container: Manages multiple clients and their capabilities.
 *
 * A device can have multiple clients (e.g., SSH + HTTP, Serial + SNMP).
 * The Device class manages these clients and auto-discovers their capabilities.
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include "client.h"
#include "result.h"

namespace omni_sdk {

/**
 * @brief Capability information storage.
 */
struct CapabilityInfo {
    std::string client_name;
    std::string description;
};

/**
 * @brief Device configuration placeholder.
 * TODO: Replace with actual Config class from config.h when implemented.
 */
struct Config {
    std::string name;
    std::map<std::string, std::map<std::string, std::string>> clients;

    std::string getString(const std::string& key,
                          const std::string& defaultValue = "") const {
        // Placeholder implementation
        return defaultValue;
    }

    std::map<std::string, std::string> getClientConfig(const std::string& clientName) const {
        auto it = clients.find(clientName);
        if (it != clients.end()) {
            return it->second;
        }
        return {};
    }
};

/**
 * @brief Device container - holds multiple clients and capabilities.
 *
 * Usage:
 *     auto device = std::make_shared<Device>("device-001", config);
 *     device->addClient(std::make_shared<SshClient>());   // Add SSH capability
 *     device->addClient(std::make_shared<HttpClient>()); // Add HTTP capability
 *
 *     // Auto-discovered from client capabilities
 *     auto caps = device->listCapabilities();
 *
 *     // Execute capability
 *     auto result = device->execute("execute", "show version", 5000);
 */
class Device {
public:
    /**
     * @brief Initialize device.
     * @param device_id Unique device identifier
     * @param config Device configuration from devices.toml
     */
    Device(const std::string& device_id, const Config& config)
        : device_id_(device_id), config_(config) {
        name_ = config_.getString("name", device_id);
    }

    /**
     * @brief Add client to device - framework discovers capabilities from client.
     * @param client Client instance to add
     * @return Result.ok() on success, Result.err() on failure
     */
    Result<void> addClient(std::shared_ptr<Client> client) {
        if (clients_.find(client->name()) != clients_.end()) {
            return Result<void>::Err(Error{
                ErrorKinds::CONFIG_ERROR,
                "Client " + client->name() + " already exists in device " + device_id_,
                {{"device_id", device_id_}, {"client_name", client->name()}}
            });
        }

        // Initialize client with config
        auto client_config = config_.getClientConfig(client->name());
        auto init_result = client->initialize(client_config);

        if (init_result.isErr()) {
            return init_result;
        }

        clients_[client->name()] = client;

        // Auto-register capabilities from client
        for (const auto& [cap_name, cap_desc] : client->capabilities()) {
            capabilities_[cap_name] = CapabilityInfo{
                client->name(), cap_desc
            };
        }

        return Result<void>::Ok();
    }

    /**
     * @brief Get client by name.
     * @param name Client name (e.g., "ssh", "serial")
     * @return Result.ok(client) on success, Result.err() on failure
     */
    Result<std::shared_ptr<Client>> getClient(const std::string& name) {
        auto it = clients_.find(name);
        if (it == clients_.end()) {
            std::vector<std::string> available;
            for (const auto& [n, _] : clients_) {
                available.push_back(n);
            }
            return Result<std::shared_ptr<Client>>::Err(Error{
                ErrorKinds::DEVICE_ERROR,
                "Client '" + name + "' not found in device " + device_id_,
                {{"device_id", device_id_}, {"available_clients", joinStrings(available, ", ")}}
            });
        }
        return Result<std::shared_ptr<Client>>::Ok(it->second);
    }

    /**
     * @brief Connect all clients.
     * @return Result.ok() on success, Result.err() on first failure
     */
    Result<void> connectAll() {
        for (auto& [_, client] : clients_) {
            auto result = client->connect();
            if (result.isErr()) {
                return result;
            }
        }
        return Result<void>::Ok();
    }

    /**
     * @brief Disconnect all clients.
     * @return Result.ok() (always succeeds for disconnectAll)
     */
    Result<void> disconnectAll() {
        for (auto& [_, client] : clients_) {
            auto result = client->disconnect();
            // Log but continue disconnecting others
        }
        return Result<void>::Ok();
    }

    /**
     * @brief List all available capabilities across all clients.
     * @return List of capability names
     */
    std::vector<std::string> listCapabilities() const {
        std::vector<std::string> caps;
        for (const auto& [name, _] : capabilities_) {
            caps.push_back(name);
        }
        return caps;
    }

    /**
     * @brief List all client names.
     * @return List of client names
     */
    std::vector<std::string> listClients() const {
        std::vector<std::string> clients;
        for (const auto& [name, _] : clients_) {
            clients.push_back(name);
        }
        return clients;
    }

    /**
     * @brief Get device ID.
     */
    std::string deviceId() const { return device_id_; }

    /**
     * @brief Get device name.
     */
    std::string getName() const { return name_; }

private:
    std::string device_id_;
    std::string name_;
    std::map<std::string, std::shared_ptr<Client>> clients_;
    std::map<std::string, CapabilityInfo> capabilities_;
    Config config_;

    static std::string joinStrings(const std::vector<std::string>& strs, const std::string& delim) {
        if (strs.empty()) return "";
        std::string result = strs[0];
        for (size_t i = 1; i < strs.size(); ++i) {
            result += delim + strs[i];
        }
        return result;
    }
};

} // namespace omni_sdk
