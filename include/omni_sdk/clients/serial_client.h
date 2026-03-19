/**
 * @file serial_client.h
 * @brief SerialClient: Serial port communication client.
 *
 * Dependencies:
 *   - Boost.Asio (for serial port communication)
 *
 * Implementation notes:
 *   1. Use boost::asio::serial_port for port management
 *   2. Use boost::asio::serial_port_base::baud_rate for configuration
 *   3. Implement all 9 Client interface methods
 *   4. See Python serial_client.py for complete implementation
 *
 * To build:
 *   - Set CMake option: OMNI_SDK_ENABLE_SERIAL=ON
 *   - Requires Boost.Asio (system component)
 */

#pragma once

#include "omni_sdk/result.h"
#include <string>
#include <map>
#include <memory>

namespace omni_sdk {

class SerialClient {
public:
    SerialClient();
    ~SerialClient();

    std::string name() const { return std::string("serial"); }
    std::string version() const { return std::string("1.0.0"); }

    Result<void> initialize(const std::map<std::string, std::string>& config);
    Result<void> connect();
    Result<void> disconnect();
    bool isConnected() const;

    Result<void> send(const std::string& command);
    Result<std::string> receive(int timeout_ms = 5000);
    Result<std::string> sendAndReceive(const std::string& command, int timeout_ms = 5000);

    std::map<std::string, std::string> capabilities() const;

    Result<void> configure(const std::map<std::string, std::string>& params);

    Result<std::map<std::string, std::string>> getStatus();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace omni_sdk
