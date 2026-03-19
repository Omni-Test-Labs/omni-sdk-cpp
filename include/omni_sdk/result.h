/**
 * @file result.h
 * @brief Result<T>: Unified error handling using monadic pattern.
 *
 * All SDK methods return Result<T> instead of throwing exceptions.
 * Forces explicit error handling while maintaining clean code with chaining.
 *
 * Example:
 *     auto result = connectToDevice("192.168.1.1");
 *
 *     if (result.isOk()) {
 *         auto device = result.value();
 *         std::cout << "Connected: " << device << std::endl;
 *     } else {
 *         auto err = result.error();
 *         std::cout << "Failed: " << err.kind << " - " << err.message << std::endl;
 *     }
 *
 *     // Chaining with and_then
 *     auto result = loadConfig("config.toml")
 *         .and_then([](auto cfg) { return initializeClients(cfg); })
 *         .and_then([](auto clients) { return connectDevice(clients); });
 *
 *     if (result.isOk()) {
 *         std::cout << "Success!" << std::endl;
 *     } else {
 *         std::cout << "Error: " << result.error().message << std::endl;
 *     }
 */

#pragma once

#include <variant>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include <functional>
#include <type_traits>
#include <vector>

namespace omni_sdk {

// Forward declaration with explicit specialization for void
template<typename T>
class Result;

template<>
class Result<void>;

/**
 * @brief Error information with context and optional cause.
 */
struct Error {
    std::string kind;                           // Error type (e.g., "NetworkError", "DeviceError")
    std::string message;                        // Human-readable description
    std::map<std::string, std::string> details;  // Additional context
    std::shared_ptr<Error> cause;               // Optional causing error

    Error()
        : cause(nullptr) {}

    Error(const std::string& k, const std::string& m)
        : kind(k), message(m), cause(nullptr) {}

    Error(const std::string& k, const std::string& m,
          const std::map<std::string, std::string>& d)
        : kind(k), message(m), details(d), cause(nullptr) {}

    Error(const std::string& k, const std::string& m,
          const std::map<std::string, std::string>& d,
          const std::shared_ptr<Error>& c)
        : kind(k), message(m), details(d), cause(c) {}

    /**
     * @brief Convert error to string representation.
     */
    std::string toString() const {
        std::string s = kind + ": " + message;
        if (!details.empty()) {
            s += " (details: {";
            for (const auto& [key, value] : details) {
                s += key + "=" + value + ", ";
            }
            if (s.back() == ' ') s = s.substr(0, s.length() - 2);
            s += "})";
        }
        if (cause) {
            s += "\n  Caused by: " + cause->toString();
        }
        return s;
    }
};

/**
 * @brief Result<T>: Monadic type for error handling.
 *
 * Represents either success (ok) or failure (err).
 */
template<typename T>
class Result {
private:
    std::variant<T, Error> value_;
    bool is_ok_;

    // Private default constructor used by static factory methods
    Result() = default;

public:
    /**
     * @brief Create a successful result.
     * @param value The success value
     */
    static Result Ok(T value) {
        Result result;
        result.value_ = std::move(value);
        result.is_ok_ = true;
        return result;
    }

    /**
     * @brief Create an error result.
     * @param error The error information
     */
    static Result Err(Error error) {
        Result result;
        result.value_ = std::move(error);
        result.is_ok_ = false;
        return result;
    }

    /**
     * @brief Create error from exception.
     * @param exception The exception to convert
     * @param kind Error kind (defaults to "RuntimeError")
     */
    static Result fromException(const std::exception& exception,
                                const std::string& kind = "RuntimeError") {
        return Result::Err(Error(
            kind,
            exception.what(),
            {{"exception_type", typeid(exception).name()}}
        ));
    }

    /**
     * @brief Check if result is success.
     */
    bool isOk() const { return is_ok_; }

    /**
     * @brief Check if result is error.
     */
    bool isErr() const { return !is_ok_; }

    /**
     * @brief Unwrap the value or throw if error.
     * Use only when you're certain the result is ok.
     * @throws std::runtime_error If result is err
     */
    T unwrap() const {
        if (!is_ok_) {
            throw std::runtime_error("Attempted to unwrap error: " +
                                    std::get<Error>(value_).message);
        }
        return std::get<T>(value_);
    }

    /**
     * @brief Unwrap value or return default if error.
     * @param default Default value to return on error
     */
    T unwrapOr(const T& defaultValue) const {
        return is_ok_ ? std::get<T>(value_) : defaultValue;
    }

    /**
     * @brief Unwrap value or compute from error.
     * @param fn Function that takes error and returns default value
     */
    template<typename F>
    T unwrapOrElse(F fn) const {
        return is_ok_ ? std::get<T>(value_) : fn(std::get<Error>(value_));
    }

    /**
     * @brief Transform success value using a function.
     * @param fn Function to apply to success value
     * @return New Result with transformed value
     */
    template<typename F>
    auto map(F fn) const -> Result<decltype(fn(std::declval<T>()))> {
        using ReturnType = decltype(fn(std::declval<T>()));

        if (!is_ok_) {
            return Result<ReturnType>::Err(std::get<Error>(value_));
        }

        try {
            return Result<ReturnType>::Ok(fn(std::get<T>(value_)));
        } catch (const std::exception& e) {
            return Result<ReturnType>::Err(Error{
                "RuntimeError",
                "map function failed: " + std::string(e.what()),
                {{"exception_type", typeid(e).name()}}
            });
        }
    }

    /**
     * @brief Chain operations: only calls fn if result is ok.
     * Short-circuits on first error.
     * @param fn Function that takes value and returns Result
     * @return Result of fn if ok, or this error
     */
    template<typename F>
    auto andThen(F fn) const -> decltype(fn(std::declval<T>())) {
        using ReturnType = decltype(fn(std::declval<T>()));

        if (!is_ok_) {
            return ReturnType::Err(std::get<Error>(value_));
        }

        try {
            return fn(std::get<T>(value_));
        } catch (const std::exception& e) {
            return ReturnType::Err(Error{
                "RuntimeError",
                "and_then function failed: " + std::string(e.what()),
                {{"exception_type", typeid(e).name()}}
            });
        }
    }

    /**
     * @brief Provide fallback if result is err.
     * @param fn Function that takes error and returns Result
     * @return This result if ok, or result from fn
     */
    template<typename F>
    Result<T> orElse(F fn) const {
        if (is_ok_) {
            return Result<T>::Ok(std::get<T>(value_));
        }

        try {
            return fn(std::get<Error>(value_));
        } catch (const std::exception& e) {
            return Result<T>::Err(Error{
                "RuntimeError",
                "or_else function failed: " + std::string(e.what()),
                {{"exception_type", typeid(e).name()}}
            });
        }
    }

    /**
     * @brief Get the error (only valid if isErr()).
     */
    const Error& error() const {
        return std::get<Error>(value_);
    }

    /**
     * @brief Get the value (only valid if isOk()).
     */
    const T& value() const {
        return std::get<T>(value_);
    }
};

/**
 * @brief Helper function to create error.
 */
inline Error createError(const std::string& kind,
                         const std::string& message,
                         const std::map<std::string, std::string>& details = {},
                         const std::shared_ptr<Error>& cause = nullptr) {
    return Error(kind, message, details, cause);
}

/**
 * @brief Standard error kinds used across the SDK.
 */
struct ErrorKinds {
    static constexpr const char* NETWORK_ERROR = "NetworkError";
    static constexpr const char* CONNECTION_ERROR = "ConnectionError";
    static constexpr const char* TIMEOUT_ERROR = "TimeoutError";
    static constexpr const char* AUTHENTICATION_ERROR = "AuthenticationError";

    static constexpr const char* DEVICE_ERROR = "DeviceError";
    static constexpr const char* DEVICE_NOT_FOUND_ERROR = "DeviceNotFoundError";
    static constexpr const char* DEVICE_BUSY_ERROR = "DeviceBusyError";
    static constexpr const char* DEVICE_NOT_CONNECTED = "DeviceNotConnected";

    static constexpr const char* CONFIG_ERROR = "ConfigError";
    static constexpr const char* CONFIG_NOT_FOUND_ERROR = "ConfigNotFoundError";
    static constexpr const char* CONFIG_VALIDATION_ERROR = "ConfigValidationError";
    static constexpr const char* UNKNOWN_CLIENT_TYPE_ERROR = "UnknownClientTypeError";

    static constexpr const char* RUNTIME_ERROR = "RuntimeError";
    static constexpr const char* INVALID_OPERATION_ERROR = "InvalidOperationError";
    static constexpr const char* SERIALIZATION_ERROR = "SerializationError";

    static constexpr const char* SSH_ERROR = "SshError";
    static constexpr const char* SERIAL_ERROR = "SerialError";
    static constexpr const char* ADB_ERROR = "AdbError";
    static constexpr const char* HTTP_ERROR = "HttpError";
    static constexpr const char* WEBSOCKET_ERROR = "WebSocketError";
    static constexpr const char* GRPC_ERROR = "GrpcError";
    static constexpr const char* SNMP_ERROR = "SnmpError";
    static constexpr const char* NETCONF_ERROR = "NetconfError";
};

/**
 * @brief Specialization of Result<void> for operations with no return value.
 * Cannot use std::variant<void, Error>, so use a simpler representation.
 */
template<>
class Result<void> {
private:
    std::shared_ptr<Error> error_;

    Result(std::shared_ptr<Error> err) : error_(err) {}
    Result(std::nullptr_t) : error_(nullptr) {}

public:
    static Result Ok() {
        return Result(nullptr);
    }

    static Result Err(Error error) {
        return Result(std::make_shared<Error>(std::move(error)));
    }

    static Result fromException(const std::exception& exception,
                                const std::string& kind = "RuntimeError") {
        return Result::Err(Error(
            kind,
            exception.what(),
            {{"exception_type", typeid(exception).name()}}
        ));
    }

    bool isOk() const { return error_ == nullptr; }
    bool isErr() const { return error_ != nullptr; }

    void unwrap() const {
        if (!isOk()) {
            throw std::runtime_error("Attempted to unwrap error: " + error_->message);
        }
    }

    void unwrapOr(...) const {
        // No-op for void success
        if (isErr()) {
            // Could throw or log here
        }
    }

    template<typename F>
    Result<void> map(F fn) const {
        if (isErr()) {
            return *this;
        }
        try {
            fn();
            return Result::Ok();
        } catch (const std::exception& e) {
            return Result::Err(Error{
                "RuntimeError",
                "map function failed: " + std::string(e.what()),
                {{"exception_type", typeid(e).name()}}
            });
        }
    }

    template<typename F>
    auto andThen(F fn) const -> decltype(fn()) {
        using ReturnType = decltype(fn());

        if (isErr()) {
            return ReturnType::Err(*error_);
        }

        try {
            return fn();
        } catch (const std::exception& e) {
            return ReturnType::Err(Error{
                "RuntimeError",
                "and_then function failed: " + std::string(e.what()),
                {{"exception_type", typeid(e).name()}}
            });
        }
    }

    template<typename F>
    Result<void> orElse(F fn) const {
        if (isOk()) {
            return Result::Ok();
        }

        try {
            return fn(*error_);
        } catch (const std::exception& e) {
            return Result::Err(Error{
                "RuntimeError",
                "or_else function failed: " + std::string(e.what()),
                {{"exception_type", typeid(e).name()}}
            });
        }
    }

    const Error& error() const {
        return *error_;
    }
};

} // namespace omni_sdk