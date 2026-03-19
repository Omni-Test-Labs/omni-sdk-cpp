/**
 * @file serial_client.cpp
 * @brief SerialClient implementation using Boost.Asio.
 */

#include "omni_sdk/clients/serial_client.h"

#ifdef OMNI_SDK_ENABLE_SERIAL
#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>

namespace omni_sdk {

struct SerialClient::Impl {
    std::shared_ptr<boost::asio::io_context> io_context;
    std::shared_ptr<boost::asio::serial_port> serial_port;
    std::shared_ptr<boost::asio::deadline_timer> deadline;

    std::map<std::string, std::string> config;
    bool connected;

    Impl()
        : io_context(std::make_shared<boost::asio::io_context>()),
          serial_port(nullptr),
          deadline(std::make_shared<boost::asio::deadline_timer>(*io_context)),
          connected(false) {}
};

SerialClient::SerialClient() : impl_(std::make_unique<Impl>()) {}

SerialClient::~SerialClient() {
    if (impl_->connected) {
        disconnect();
    }
}

Result<void> SerialClient::initialize(const std::map<std::string, std::string>& config) {
    if (config.find("port") == config.end()) {
        return Result<void>::Err(createError(
            ErrorKinds::CONFIG_ERROR,
            "Missing required serial config field: port",
            {{"required_fields", "port"}, {"provided", "config keys available"}}
        ));
    }

    impl_->config = config;

    // Validate baud rate
    int baud_rate = 9600;
    if (config.find("baud_rate") != config.end()) {
        try {
            baud_rate = std::stoi(config.at("baud_rate"));
        } catch (...) {
            return Result<void>::Err(createError(
                ErrorKinds::CONFIG_ERROR,
                "Invalid baud_rate format",
                {{"baud_rate", config.at("baud_rate")}}
            ));
        }
    }

    std::vector<int> valid_rates = {9600, 19200, 38400, 57600, 115200, 230400};
    if (std::find(valid_rates.begin(), valid_rates.end(), baud_rate) == valid_rates.end()) {
        std::ostringstream oss;
        oss << "Invalid baud rate " << baud_rate << ". Valid rates: ";
        for (size_t i = 0; i < valid_rates.size(); ++i) {
            oss << valid_rates[i];
            if (i < valid_rates.size() - 1) oss << ", ";
        }
        return Result<void>::Err(createError(
            ErrorKinds::CONFIG_ERROR,
            oss.str(),
            {{"baud_rate", std::to_string(baud_rate)}}
        ));
    }
    impl_->config["baud_rate"] = std::to_string(baud_rate);

    // Set defaults
    if (impl_->config.find("data_bits") == impl_->config.end()) {
        impl_->config["data_bits"] = "8";
    }
    if (impl_->config.find("stop_bits") == impl_->config.end()) {
        impl_->config["stop_bits"] = "1";
    }
    if (impl_->config.find("timeout_ms") == impl_->config.end()) {
        impl_->config["timeout_ms"] = "5000";
    }

    return Result<void>::Ok();
}

Result<void> SerialClient::connect() {
    if (impl_->connected) {
        return Result<void>::Ok();
    }

    try {
        std::string port = impl_->config.at("port");
        int baud_rate = std::stoi(impl_->config.at("baud_rate"));
        int data_bits = std::stoi(impl_->config.at("data_bits"));
        int stop_bits = std::stoi(impl_->config.at("stop_bits"));
        double timeout_sec = std::stod(impl_->config.at("timeout_ms")) / 1000.0;

        impl_->serial_port = std::make_shared<boost::asio::serial_port>(
            *impl_->io_context, port);

        // Configure serial port
        using namespace boost::asio;
        serial_port_base::baud_rate baud(baud_rate);
        serial_port_base::character_size char_size(data_bits);
        serial_port_base::stop_bits stop;
        if (stop_bits == 1) stop = serial_port_base::stop_bits(serial_port_base::stop_bits::one);
        else if (stop_bits == 2) stop = serial_port_base::stop_bits(serial_port_base::stop_bits::two);
        else stop = serial_port_base::stop_bits(serial_port_base::stop_bits::one);

        serial_port_base::parity parity(serial_port_base::parity::none);
        serial_port_base::flow_control flow(serial_port_base::flow_control::none);

        impl_->serial_port->set_option(baud);
        impl_->serial_port->set_option(char_size);
        impl_->serial_port->set_option(stop);
        impl_->serial_port->set_option(parity);
        impl_->serial_port->set_option(flow);

        impl_->connected = true;
        return Result<void>::Ok();

    } catch (const boost::system::system_error& e) {
        return Result<void>::Err(createError(
            ErrorKinds::SERIAL_ERROR,
            std::string("Serial connection failed: ") + e.what(),
            {{"port", impl_->config.at("port")}, {"error_code", std::to_string(e.code().value())}}
        ));
    } catch (const std::exception& e) {
        return Result<void>::Err(createError(
            ErrorKinds::DEVICE_ERROR,
            std::string("Serial error: ") + e.what(),
            {{"exception_type", typeid(e).name()}}
        ));
    }
}

Result<void> SerialClient::disconnect() {
    if (impl_->connected && impl_->serial_port) {
        try {
            boost::system::error_code ec;
            impl_->serial_port->close(ec);
        } catch (...) {
        }
        impl_->connected = false;
        impl_->serial_port.reset();
    }
    return Result<void>::Ok();
}

bool SerialClient::isConnected() const {
    if (!impl_->connected || !impl_->serial_port) {
        return false;
    }
    return impl_->serial_port->is_open();
}

Result<void> SerialClient::send(const std::string& command) {
    if (!impl_->connected || !impl_->serial_port) {
        return Result<void>::Err(createError(
            ErrorKinds::DEVICE_NOT_CONNECTED,
            "Serial client is not connected"
        ));
    }

    try {
        size_t bytes_written = boost::asio::write(
            *impl_->serial_port, boost::asio::buffer(command));
        (void)bytes_written;
        return Result<void>::Ok();
    } catch (const boost::system::system_error& e) {
        return Result<void>::Err(createError(
            ErrorKinds::SERIAL_ERROR,
            std::string("Serial send failed: ") + e.what(),
            {{"command", command}}
        ));
    } catch (const std::exception& e) {
        return Result<void>::Err(createError(
            ErrorKinds::RUNTIME_ERROR,
            std::string("Send error: ") + e.what()
        ));
    }
}

Result<std::string> SerialClient::receive(int timeout_ms) {
    if (!impl_->connected || !impl_->serial_port) {
        return Result<std::string>::Err(createError(
            ErrorKinds::DEVICE_NOT_CONNECTED,
            "Serial client is not connected"
        ));
    }

    try {
        double timeout_sec = timeout_ms / 1000.0;
        impl_->serial_port->set_option(boost::asio::serial_port_base::timeout(boost::posix_time::milliseconds(timeout_ms)));

        std::vector<char> buffer(4096);
        size_t bytes_read = boost::asio::read(*impl_->serial_port, boost::asio::buffer(buffer));

        if (bytes_read == 0) {
            return Result<std::string>::Err(createError(
                ErrorKinds::TIMEOUT_ERROR,
                "No data received (timeout)",
                {{"timeout_ms", std::to_string(timeout_ms)}}
            ));
        }

        std::string data(buffer.begin(), buffer.begin() + bytes_read);
        return Result<std::string>::Ok(data);

    } catch (const boost::system::system_error& e) {
        if (e.code() == boost::asio::error::would_block ||
            e.code() == boost::asio::error::timed_out) {
            return Result<std::string>::Err(createError(
                ErrorKinds::TIMEOUT_ERROR,
                "Receive timeout",
                {{"timeout_ms", std::to_string(timeout_ms)}}
            ));
        }
        return Result<std::string>::Err(createError(
            ErrorKinds::SERIAL_ERROR,
            std::string("Serial receive failed: ") + e.what()
        ));
    } catch (const std::exception& e) {
        return Result<std::string>::Err(createError(
            ErrorKinds::RUNTIME_ERROR,
            std::string("Receive error: ") + e.what()
        ));
    }
}

Result<std::string> SerialClient::sendAndReceive(const std::string& command, int timeout_ms) {
    auto send_result = send(command);
    if (send_result.isErr()) {
        return Result<std::string>::Err(send_result.error());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return receive(timeout_ms);
}

std::map<std::string, std::string> SerialClient::capabilities() const {
    return {
        {"send", "Send data to serial port"},
        {"receive", "Receive data from serial port"},
        {"get_status", "Get serial port status"},
        {"configure", "Reconfigure serial port parameters"}
    };
}

Result<void> SerialClient::configure(const std::map<std::string, std::string>& params) {
    if (!impl_->connected || !impl_->serial_port) {
        return Result<void>::Err(createError(
            ErrorKinds::DEVICE_NOT_CONNECTED,
            "Serial client is not connected"
        ));
    }

    try {
        using namespace boost::asio;

        for (const auto& [key, value] : params) {
            if (key == "baud_rate") {
                int baud_rate = std::stoi(value);
                serial_port_base::baud_rate baud(baud_rate);
                impl_->serial_port->set_option(baud);
                impl_->config["baud_rate"] = value;
            } else if (key == "data_bits") {
                int data_bits = std::stoi(value);
                serial_port_base::character_size char_size(data_bits);
                impl_->serial_port->set_option(char_size);
                impl_->config["data_bits"] = value;
            } else if (key == "stop_bits") {
                int stop_bits = std::stoi(value);
                serial_port_base::stop_bits stop;
                if (stop_bits == 1) stop = serial_port_base::stop_bits(serial_port_base::stop_bits::one);
                else if (stop_bits == 2) stop = serial_port_base::stop_bits(serial_port_base::stop_bits::two);
                else stop = serial_port_base::stop_bits(serial_port_base::stop_bits::one);
                impl_->serial_port->set_option(stop);
                impl_->config["stop_bits"] = value;
            } else if (key == "timeout_ms") {
                double timeout_sec = std::stod(value) / 1000.0;
                impl_->serial_port->set_option(serial_port_base::timeout(boost::posix_time::milliseconds(std::stod(value))));
                impl_->config["timeout_ms"] = value;
            }
        }

        return Result<void>::Ok();

    } catch (const std::exception& e) {
        return Result<void>::Err(createError(
            ErrorKinds::SERIAL_ERROR,
            std::string("Reconfiguration failed: ") + e.what(),
            {{"params_count", std::to_string(params.size())}}
        ));
    }
}

Result<std::map<std::string, std::string>> SerialClient::getStatus() {
    if (!impl_->connected || !impl_->serial_port) {
        std::map<std::string, std::string> status = {
            {"connected", "false"},
            {"client_name", name()},
            {"version", version()}
        };
        return Result<std::map<std::string, std::string>>::Ok(status);
    }

    std::map<std::string, std::string> status = {
        {"connected", impl_->connected ? "true" : "false"},
        {"client_name", name()},
        {"version", version()},
        {"port", impl_->config["port"]},
        {"baud_rate", impl_->config["baud_rate"]},
        {"data_bits", impl_->config["data_bits"]},
        {"stop_bits", impl_->config["stop_bits"]},
        {"is_open", impl_->serial_port->is_open() ? "true" : "false"}
    };

    return Result<std::map<std::string, std::string>>::Ok(status);
}

} // namespace omni_sdk

#else // !OMNI_SDK_ENABLE_SERIAL

namespace omni_sdk {

struct SerialClient::Impl {};

SerialClient::SerialClient() : impl_(std::make_unique<Impl>()) {}
SerialClient::~SerialClient() = default;

Result<void> SerialClient::initialize(const std::map<std::string, std::string>&) {
    return Result<void>::Err(createError(
        ErrorKinds::SERIAL_ERROR,
        "Serial client support not enabled. Build with OMNI_SDK_ENABLE_SERIAL=ON"
    ));
}

Result<void> SerialClient::connect() {
    return Result<void>::Err(createError(
        ErrorKinds::SERIAL_ERROR,
        "Serial client support not enabled. Build with OMNI_SDK_ENABLE_SERIAL=ON"
    ));
}

Result<void> SerialClient::disconnect() { return Result<void>::Ok(); }
bool SerialClient::isConnected() const { return false; }

Result<void> SerialClient::send(const std::string&) {
    return Result<void>::Err(createError(
        ErrorKinds::SERIAL_ERROR,
        "Serial client support not enabled. Build with OMNI_SDK_ENABLE_SERIAL=ON"
    ));
}

Result<std::string> SerialClient::receive(int) {
    return Result<std::string>::Err(createError(
        ErrorKinds::SERIAL_ERROR,
        "Serial client support not enabled. Build with OMNI_SDK_ENABLE_SERIAL=ON"
    ));
}

Result<std::string> SerialClient::sendAndReceive(const std::string&, int) {
    return Result<std::string>::Err(createError(
        ErrorKinds::SERIAL_ERROR,
        "Serial client support not enabled. Build with OMNI_SDK_ENABLE_SERIAL=ON"
    ));
}

std::map<std::string, std::string> SerialClient::capabilities() const { return {}; }

Result<void> SerialClient::configure(const std::map<std::string, std::string>&) {
    return Result<void>::Err(createError(
        ErrorKinds::SERIAL_ERROR,
        "Serial client support not enabled. Build with OMNI_SDK_ENABLE_SERIAL=ON"
    ));
}

Result<std::map<std::string, std::string>> SerialClient::getStatus() {
    std::map<std::string, std::string> status = {
        {"connected", "false"},
        {"client_name", name()},
        {"version", version()}
    };
    return Result<std::map<std::string, std::string>>::Ok(status);
}

} // namespace omni_sdk

#endif // OMNI_SDK_ENABLE_SERIAL
