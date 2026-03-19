/**
 * @file client.h
 * @brief Client interface: Unified interface for all communication clients.
 *
 * All client implementations (SshClient, SerialClient, etc.) must implement this interface.
 * Provides a flat, maintainable structure without deep inheritance.
 *
 * Standard capability names:
 * - execute: Execute command/request (e.g., "show version")
 * - send_message: Send message (e.g., HTTP POST)
 * - get_status: Get client status/health
 */

#pragma once

#include <string>
#include <map>
#include <memory>
#include "result.h"

namespace omni_sdk {

/**
 * @brief Base interface for all communication clients.
 *
 * All client implementations must implement these 9 pure virtual methods.
 */
class Client {
public:
    virtual ~Client() = default;

    /**
     * @brief Get client name.
     * @return Client name string (e.g., "ssh", "serial", "http")
     */
    virtual std::string name() const = 0;

    /**
     * @brief Get client version string.
     * @return Version string (e.g., "1.0.0")
     */
    virtual std::string version() const = 0;

    /**
     * @brief Initialize client with configuration.
     * @param config Client configuration from devices.toml
     * @return Result.ok() on success, Result.err() on failure
     *
     * Example Ssh config:
     *   {
     *     {"host", "192.168.1.1"},
     *     {"port", "22"},
     *     {"username", "admin"},
     *     {"password", "secret"},
     *     {"timeout_ms", "5000"}
     *   }
     */
    virtual Result<void> initialize(const std::map<std::string, std::string>& config) = 0;

    /**
     * @brief Establish connection to endpoint.
     * Must be called before send/receive.
     * @return Result.ok() on success, Result.err() on failure
     */
    virtual Result<void> connect() = 0;

    /**
     * @brief Close connection.
     * Safe to call multiple times.
     * @return Result.ok() on success, Result.err() on failure
     */
    virtual Result<void> disconnect() = 0;

    /**
     * @brief Check if client is currently connected.
     * @return True if connected, False otherwise
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Send command/request to endpoint.
     * @param command Command string to send
     * @return Result.ok() on success, Result.err() on failure
     */
    virtual Result<void> send(const std::string& command) = 0;

    /**
     * @brief Receive response from endpoint.
     * @param timeout_ms Timeout in milliseconds (default: 5000)
     * @return Result.ok(response_string) on success, Result.err() on failure
     */
    virtual Result<std::string> receive(int timeout_ms = 5000) = 0;

    /**
     * @brief Convenience: send command and receive response.
     * @param command Command string to send
     * @param timeout_ms Timeout in milliseconds (default: 5000)
     * @return Result.ok(response_string) on success, Result.err() on failure
     */
    virtual Result<std::string> sendAndReceive(const std::string& command,
                                                int timeout_ms = 5000) = 0;

    /**
     * @brief Return map of capability_name -> description.
     *
     * Device framework auto-discovers capabilities from clients.
     * Each capability is a method on the client object.
     *
     * @return Map mapping capability names to descriptions
     *
     * Example SshClient capabilities:
     *   {
     *     {"execute", "Execute shell command and return output"},
     *     {"file_transfer", "Transfer files using SCP"},
     *     {"get_status", "Get SSH connection status"}
     *   }
     */
    virtual std::map<std::string, std::string> capabilities() const = 0;
};

} // namespace omni_sdk
