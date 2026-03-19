/**
 * @file basic_usage.cpp
 * @brief Basic usage example for Omni SDK (C++).
 *
 * Demonstrates how to use the SDK to manage devices.
 */

#include <iostream>
#include <omni_sdk/omni_sdk.h>
#include <omni_sdk/result.h>
#include <omni_sdk/error_kinds.h>

using namespace omni_sdk;

void main_mock_mode() {
    std::cout << "========================================" << std::endl;
    std::cout << "Omni SDK - Basic Usage Example (C++)" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n[1] Demonstrating Result<T> error handling..." << std::endl;

    // Example: Successful result
    auto result = Result<int>::Ok(42);
    std::cout << "  Result.ok(42): " << (result.isOk() ? "OK" : "ERR") << std::endl;
    if (result.isOk()) {
        std::cout << "  Value: " << result.value() << std::endl;
    }

    // Example: Error result
    auto errorResult = Result<int>::Err(Error{
        ErrorKinds::NETWORK_ERROR,
        "Network timeout"
    });
    std::cout << "  Result.err(...): " << (errorResult.isOk() ? "OK" : "ERR") << std::endl;
    if (errorResult.isErr()) {
        std::cout << "  Error: " << errorResult.error().message << std::endl;
    }

    std::cout << "\n[2] Demonstrating Result<T> chaining with and_then..." << std::endl;

    auto chained = Result<int>::Ok(5)
        .andThen([](int value) {
            return Result<int>::Ok(value * 2);
        })
        .andThen([](int value) {
            return Result<int>::Ok(value + 10);
        })
        .andThen([](int value) {
            return Result<int>::Ok(value * value);
        });

    std::cout << "  Chain: 5 -> ... -> " << chained.value()
              << " (expected: 400)" << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "Mock mode example completed!" << std::endl;
    std::cout << "========================================" << std::endl;
}

void main_real_mode() {
    std::cout << "========================================" << std::endl;
    std::cout << "Omni SDK - Real Usage Example (C++)" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n[1] Loading configuration from devices.toml..." << std::endl;

    auto devices_result = OmniSDK::initializeFromConfig("devices.toml");

    if (!devices_result.isOk()) {
        std::cerr << "  ✗ Failed to load config: "
                  << devices_result.error().message << std::endl;
        return;
    }

    auto devices = devices_result.value();
    std::cout << "  ✓ Loaded " << devices.size() << " device(s)" << std::endl;

    for (const auto& [id, device] : devices) {
        std::cout << "    - " << id << ": " << device->getName() << std::endl;
    }

    std::cout << "\n[2] Connecting to device..." << std::endl;

    if (devices.empty()) {
        std::cout << "  ✗ No devices configured" << std::endl;
        return;
    }

    auto first_device = devices.begin()->first;
    auto device_result = OmniSDK::connectDevice(first_device, devices);

    if (!device_result.isOk()) {
        std::cerr << "  ✗ Failed to connect: "
                  << device_result.error().message << std::endl;
        return;
    }

    auto device = device_result.value();
    std::cout << "  ✓ Connected to " << device->getName() << std::endl;

    std::cout << "  Clients: ";
    auto clients = device->listClients();
    for (size_t i = 0; i < clients.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << clients[i];
    }
    std::cout << std::endl;

    std::cout << "\n[3] Disconnecting..." << std::endl;
    device->disconnectAll();
    std::cout << "  ✓ Disconnected" << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "Example completed!" << std::endl;
    std::cout << "========================================" << std::endl;
}

int main() {
    // Run mock mode first (doesn't require real device)
    main_mock_mode();

    // Uncomment below to run with real device
    // main_real_mode();

    return 0;
}
