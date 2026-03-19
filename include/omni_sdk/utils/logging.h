/**
 * @file logging.h
 * @brief Logging utilities compatible with Result<T> pattern.
 *
 * Simple logging implementation without external dependencies.
 */

#pragma once

#include <string>
#include <sstream>
#include <iostream>

namespace omni_sdk {

// Forward declaration
struct Error;

/**
 * @brief Simple logger for SDK.
 */
class Logger {
public:
    enum class Level {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };

    explicit Logger(const std::string& name = "omni_sdk");

    void setLevel(Level level) { level_ = level; }

    void debug(const std::string& message) {
        log(Level::Debug, "DEBUG", message);
    }

    void info(const std::string& message) {
        log(Level::Info, "INFO", message);
    }

    void warning(const std::string& message) {
        log(Level::Warning, "WARNING", message);
    }

    void error(const std::string& message, const Error& error);

    void critical(const std::string& message) {
        log(Level::Critical, "CRITICAL", message);
    }

    static Logger& get() {
        static Logger instance("omni_sdk");
        return instance;
    }

private:
    std::string name_;
    Level level_ = Level::Info;

    void log(Level level, const std::string& level_str, const std::string& message);
};

} // namespace omni_sdk
