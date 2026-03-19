/**
 * @file serial_client.h
 * @brief SerialClient: Serial port communication client.
 *
 * @note This is a placeholder implementation.
 *
 * Real implementation requires Boost.Beast or Boost.Asio.
 *
 * Dependencies:
 *   - Boost::asio (for serial port communication)
 *   - Or Boost.Beast (more modern alternative)
 *
 * Implementation notes:
 *   1. Use boost::asio::serial_port for port management
 *   2. Use boost::asio::serial_port_base::baud_rate for configuration
 *   3. Implement all 9 Client interface methods
 *   - See Python serial_client.py for complete implementation
 *
 * To implement:
 *   - Add find_package(Boost COMPONENTS system thread) to CMakeLists.txt
 *   - Implement connect() with boost::asio::serial_port
 *   - Implement send() with write() method on serial port
 *   - Implement receive() with read() with timeout
 *   - Implement configure() with set_option() calls
 */

#pragma once

#include "omni_sdk/result.h"
#include <string>
#include <map>

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
};

} // namespace omni_sdk
