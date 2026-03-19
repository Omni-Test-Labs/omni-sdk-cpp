/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T> error handling.
 */

#include <gtest/gtest.h>
#include "omni_sdk/result.h"

using namespace omni_sdk;

// ============================================================================
// Test Result Creation
// ============================================================================

TEST(ResultCreation, OkCreatesSuccessResult) {
    auto result = Result<int>::Ok(42);
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 42);
}

TEST(ResultCreation, OkWithStringValue) {
    auto result = Result<std::string>::Ok(std::string("test"));
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), "test");
}

TEST(ResultCreation, ErrCreatesErrorResult) {
    Error error("TestError", "Test failed");
    auto result = Result<int>::Err(error);
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, "TestError");
}

TEST(ResultCreation, FromException) {
    std::runtime_error ex("bad value");
    auto result = Result<int>::fromException(ex, "RuntimeError");
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, "RuntimeError");
    EXPECT_TRUE(result.error().message.find("bad value") != std::string::npos);
}

// ============================================================================
// Test Error
// ============================================================================

TEST(ErrorTest, ErrorCreation) {
    Error error("NetworkError", "Connection failed");
    EXPECT_EQ(error.kind, "NetworkError");
    EXPECT_EQ(error.message, "Connection failed");
    EXPECT_TRUE(error.details.empty());
    EXPECT_EQ(error.cause, nullptr);
}

TEST(ErrorTest, ErrorWithDetails) {
    std::map<std::string, std::string> details = {
        {"host", "192.168.1.1"},
        {"port", "22"}
    };
    Error error("NetworkError", "Connection failed", details);
    EXPECT_EQ(error.details["host"], "192.168.1.1");
    EXPECT_EQ(error.details["port"], "22");
}

TEST(ErrorTest, ErrorWithCause) {
    auto cause = std::make_shared<Error>("NetworkError", "Network unreachable");
    Error error("SshError", "SSH connection failed", {}, cause);
    EXPECT_NE(error.cause, nullptr);
    EXPECT_EQ(error.cause->message, "Network unreachable");
}

TEST(ErrorTest, ErrorToString) {
    Error error("TestError", "Test", {{"key", "value"}});
    std::string str = error.toString();
    EXPECT_TRUE(str.find("TestError") != std::string::npos);
    EXPECT_TRUE(str.find("Test") != std::string::npos);
    EXPECT_TRUE(str.find("key=value") != std::string::npos);
}

// ============================================================================
// Test Unwrap
// ============================================================================

TEST(Unwrap, UnwrapOk) {
    auto result = Result<int>::Ok(42);
    EXPECT_EQ(result.unwrap(), 42);
}

TEST(Unwrap, UnwrapErrThrows) {
    Error error("TestError", "Failed");
    auto result = Result<int>::Err(error);
    EXPECT_THROW(result.unwrap(), std::runtime_error);
}

TEST(Unwrap, UnwrapOrWithOk) {
    auto result = Result<int>::Ok(42);
    EXPECT_EQ(result.unwrapOr(99), 42);
}

TEST(Unwrap, UnwrapOrWithErr) {
    Error error("TestError", "Failed");
    auto result = Result<int>::Err(error);
    EXPECT_EQ(result.unwrapOr(99), 99);
}

TEST(Unwrap, UnwrapOrElseWithOk) {
    auto result = Result<int>::Ok(42);
    EXPECT_EQ(result.unwrapOrElse([](const Error& e) { return 99; }), 42);
}

TEST(Unwrap, UnwrapOrElseWithErr) {
    Error error("TestError", "Failed");
    auto result = Result<int>::Err(error);
    EXPECT_EQ(result.unwrapOrElse([](const Error& e) { return 99; }), 99);
}

// ============================================================================
// Test Map
// ============================================================================

TEST(Map, MapOnOk) {
    auto result = Result<int>::Ok(42).map([](int x) { return x * 2; });
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 84);
}

TEST(Map, MapOnErr) {
    Error error("TestError", "Failed");
    auto result = Result<int>::Err(error).map([](int x) { return x * 2; });
    EXPECT_TRUE(result.isErr());
}

TEST(Map, MapChangesType) {
    auto result = Result<int>::Ok(10).map([](int x) { return std::to_string(x); });
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), "10");
}

TEST(Map, MapExceptionBecomesError) {
    auto result = Result<int>::Ok(42).map([](int x) {
        throw std::runtime_error("division by zero in map");
        return 0;
    });
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, "RuntimeError");
    EXPECT_TRUE(result.error().message.find("division by zero") != std::string::npos);
}

TEST(Map, MapChain) {
    auto result = Result<int>::Ok(10)
        .map([](int x) { return x * 2; })
        .map([](int x) { return x + 5; })
        .map([](int x) { return std::to_string(x); });
    EXPECT_EQ(result.unwrap(), "25");
}

// ============================================================================
// Test AndThen
// ============================================================================

TEST(AndThen, AndThenChainAllOk) {
    auto result = Result<int>::Ok(10)
        .andThen([](int x) { return Result<int>::Ok(x + 5); })
        .andThen([](int x) { return Result<int>::Ok(x * 2); });
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 30); // (10 + 5) * 2
}

TEST(AndThen, AndThenShortCircuitsOnError) {
    int call_count = 0;

    auto result = Result<int>::Ok(10)
        .andThen([&call_count](int x) {
            call_count++;
            return Result<int>::Ok(x + 5);
        })
        .andThen([&call_count](int x) -> Result<int> {
            call_count++;
            return Result<int>::Err(Error("TestError", "Failed"));
        })
        .andThen([&call_count](int x) {
            call_count++;
            return Result<int>::Ok(x);
        });

    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(call_count, 2); // Only first two functions called
}

TEST(AndThen, AndThenFromErrorStaysError) {
    Error error("TestError", "Failed");
    auto result = Result<int>::Err(error).andThen([](int x) {
        return Result<int>::Ok(x + 5);
    });
    EXPECT_TRUE(result.isErr());
}

TEST(AndThen, AndThenExceptionInFunction) {
    auto result = Result<int>::Ok(10).andThen([](int x) -> Result<int> {
        throw std::runtime_error("Thrown error");
    });
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, "RuntimeError");
    EXPECT_TRUE(result.error().message.find("Thrown error") != std::string::npos);
}

// ============================================================================
// Test OrElse
// ============================================================================

TEST(OrElse, OrElseOnOkReturnsOriginal) {
    auto result = Result<int>::Ok(42).orElse([](const Error& e) {
        return Result<int>::Ok(99);
    });
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 42);
}

TEST(OrElse, OrElseOnErrCallsFallback) {
    Error error("TestError", "Failed");
    auto result = Result<int>::Err(error).orElse([](const Error& e) {
        return Result<int>::Ok(99);
    });
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 99);
}

TEST(OrElse, OrElseWithDifferentError) {
    Error error("TestError", "Failed");
    auto result = Result<int>::Err(error).orElse([](const Error& e) {
        return Result<int>::Err(Error("OtherError", "Fallback"));
    });
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, "OtherError");
}

// ============================================================================
// Test Accessors
// ============================================================================

TEST(Accessors, ValueOnOk) {
    auto result = Result<int>::Ok(42);
    EXPECT_EQ(result.value(), 42);
}

TEST(Accessors, ValueOnErr) {
    Error error("TestError", "Failed");
    auto result = Result<int>::Err(error);
    EXPECT_THROW(result.value(), std::bad_variant_access);
}

TEST(Accessors, ErrorOnOk) {
    auto result = Result<int>::Ok(42);
    EXPECT_THROW(result.error(), std::bad_variant_access);
}

TEST(Accessors, ErrorOnErr) {
    Error error("TestError", "Failed");
    auto result = Result<int>::Err(error);
    EXPECT_EQ(result.error().kind, "TestError");
}

// ============================================================================
// Test Helper Functions
// ============================================================================

TEST(Helpers, CreateError) {
    auto error = createError(
        "TestError",
        "Test failed",
        {{"key", "value"}}
    );
    EXPECT_EQ(error.kind, "TestError");
    EXPECT_EQ(error.message, "Test failed");
    EXPECT_EQ(error.details["key"], "value");
}

TEST(Helpers, ErrorKindsConstants) {
    EXPECT_STREQ(ErrorKinds::NETWORK_ERROR, "NetworkError");
    EXPECT_STREQ(ErrorKinds::CONNECTION_ERROR, "ConnectionError");
    EXPECT_STREQ(ErrorKinds::CONFIG_ERROR, "ConfigError");
    EXPECT_STREQ(ErrorKinds::SSH_ERROR, "SshError");
    EXPECT_STREQ(ErrorKinds::SERIAL_ERROR, "SerialError");
}

// ============================================================================
// Test Complex Chaining
// ============================================================================

// Simulate load config -> validate -> parse workflow
static Result<std::map<std::string, std::string>> loadConfig(const std::string& path) {
    if (path == "invalid.toml") {
        return Result<std::map<std::string, std::string>>::Err(Error(
            ErrorKinds::CONFIG_ERROR,
            "Config file not found: " + path
        ));
    }
    std::map<std::string, std::string> config = {
        {"host", "192.168.1.1"},
        {"port", "22"}
    };
    return Result<std::map<std::string, std::string>>::Ok(config);
}

static Result<std::map<std::string, std::string>> validateConfig(
    const std::map<std::string, std::string>& config) {
    if (config.find("host") == config.end()) {
        return Result<std::map<std::string, std::string>>::Err(Error(
            ErrorKinds::CONFIG_VALIDATION_ERROR,
            "Missing required field: host"
        ));
    }
    return Result<std::map<std::string, std::string>>::Ok(config);
}

static Result<std::pair<std::string, int>> parseClientConfig(
    const std::map<std::string, std::string>& config) {
    int port = 22;
    auto it = config.find("port");
    if (it != config.end()) {
        port = std::stoi(it->second);
    }
    return Result<std::pair<std::string, int>>::Ok({config.at("host"), port});
}

TEST(ComplexChaining, LoadValidateParseChain) {
    // Happy path
    auto result = loadConfig("devices.toml")
        .andThen(validateConfig)
        .andThen(parseClientConfig);

    EXPECT_TRUE(result.isOk());
    auto [host, port] = result.unwrap();
    EXPECT_EQ(host, "192.168.1.1");
    EXPECT_EQ(port, 22);

    // Error path (invalid file)
    result = loadConfig("invalid.toml")
        .andThen(validateConfig)
        .andThen(parseClientConfig);

    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_ERROR);
    EXPECT_TRUE(result.error().message.find("not found") != std::string::npos);

    // Error path (validation failure)
    result = loadConfig("devices.toml")
        .map([](auto cfg) {
            std::map<std::string, std::string> partial = {{"port", "22"}};
            return partial;
        })
        .andThen(validateConfig)
        .andThen(parseClientConfig);

    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, ErrorKinds::CONFIG_VALIDATION_ERROR);
    EXPECT_TRUE(result.error().message.find("Missing required field") != std::string::npos);
}

// Simulate retry logic using orElse
TEST(ComplexChaining, RetryPatternWithOrElse) {
    int attempt = 0;

    auto connect = [&attempt](const std::string& host) -> Result<std::string> {
        attempt++;
        if (attempt < 3) {
            return Result<std::string>::Err(Error(
                ErrorKinds::CONNECTION_ERROR,
                "Connection attempt " + std::to_string(attempt) + " failed",
                {{"attempt", std::to_string(attempt)}}
            ));
        }
        return Result<std::string>::Ok("Connected to " + host);
    };

    auto retry = [&attempt, &connect](const Result<std::string>& result) -> Result<std::string> {
        if (attempt < 3) {
            return connect("192.168.1.1");
        }
        return result;
    };

    auto result = connect("192.168.1.1")
        .orElse(retry)
        .orElse(retry);

    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), "Connected to 192.168.1.1");
    EXPECT_EQ(attempt, 3);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
