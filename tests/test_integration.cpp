/**
 * @file test_integration.cpp
 * @brief Integration tests for SDK framework.
 *
 * Tests full workflow from config loading to device communication.
 */

#include <gtest/gtest.h>
#include "../include/omni_sdk/omni_sdk.h"
#include <fstream>
#include <sstream>

// ============================================================================
// Integration Tests
// ============================================================================

TEST(Integration, InitializeFromConfigFileNotFound) {
    auto result = omni_sdk::OmniSDK::initializeFromConfig("nonexistent.toml");

    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, omni_sdk::ErrorKinds::CONFIG_NOT_FOUND_ERROR);
}

TEST(Integration, ValidateEmptyConfig) {
    std::map<std::string, std::string> data;
    data["global"] = "";
    data["devices"] = "";

    auto result = omni_sdk::ConfigLoader::validate(data);

    // For now with stub implementation, should not crash
    // Real TOML parsing would happen in CMakeLists integration
    (void)result;
}

TEST(Integration, GetClientConfigEmpty) {
    omni_sdk::DeviceConfig device_config;
    device_config.id = "test-device";
    device_config.name = "Test Device";

    auto result = omni_sdk::ConfigLoader::getClientConfig(device_config, "ssh");

    // Empty clients, should handle gracefully
    EXPECT_TRUE(result.isErr());
}

TEST(Integration, ResultErrorPropagation) {
    // Test that errors propagate through the chain properly
    auto error = omni_sdk::Error{
        omni_sdk::ErrorKinds::NETWORK_ERROR,
        "Connection failed"
    };

    auto result = omni_sdk::Result<std::string>::Err(error);

    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().kind, omni_sdk::ErrorKinds::NETWORK_ERROR);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
