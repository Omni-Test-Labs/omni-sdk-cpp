cmake_minimum_required(VERSION 3.15)
project(omni-sdk-cpp VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Code coverage (for testing)
option(BUILD_COVERAGE "Build with code coverage support" OFF)
if(BUILD_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Building with code coverage support")
        add_compile_options(-g -O0 --coverage)
        add_link_options(--coverage)
    else()
        message(WARNING "Coverage is only supported with GCC or Clang")
    endif()
endif()

# Compiler settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SDK_SOURCES
    src/config.cpp
    src/omni_sdk.cpp
)

set(HEADERS
    include/omni_sdk/result.h
    include/omni_sdk/client.h
    include/omni_sdk/device.h
    include/omni_sdk/config.h
    include/omni_sdk/omni_sdk.h
    include/omni_sdk/clients/serial_client.h
    include/omni_sdk/clients/ssh_client.h
)

# Option to build with libssh2 (SSH client)
option(OMNI_SDK_ENABLE_SSH "Enable SSH client support" OFF)
if(OMNI_SDK_ENABLE_SSH)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSH2 REQUIRED libssh2)
    list(APPEND SDK_SOURCES src/clients/ssh_client.cpp)
    find_package(Threads REQUIRED)
endif()

# Option to build with Boost.Asio (Serial client)
option(OMNI_SDK_ENABLE_SERIAL "Enable Serial client support" OFF)
if(OMNI_SDK_ENABLE_SERIAL)
    find_package(Boost REQUIRED COMPONENTS system thread)
    list(APPEND SDK_SOURCES src/clients/serial_client.cpp)
    find_package(Threads REQUIRED)
endif()

# Main library
add_library(omni_sdk ${SDK_SOURCES} ${HEADERS})
target_include_directories(omni_sdk PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Find dependencies
find_package(Threads REQUIRED)
target_link_libraries(omni_sdk PRIVATE Threads::Threads)

# Link SSH dependencies
if(OMNI_SDK_ENABLE_SSH)
    target_include_directories(omni_sdk PRIVATE ${LIBSSH2_INCLUDE_DIRS})
    target_link_libraries(omni_sdk PRIVATE ${LIBSSH2_LIBRARIES})
    target_compile_definitions(omni_sdk PRIVATE OMNI_SDK_ENABLE_SSH)
endif()

# Link Serial dependencies
if(OMNI_SDK_ENABLE_SERIAL)
    target_include_directories(omni_sdk PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(omni_sdk PRIVATE Boost::system Boost::thread)
    target_compile_definitions(omni_sdk PRIVATE OMNI_SDK_ENABLE_SERIAL)
endif()

# Find TOML++ (configuration)
find_package(tomlplusplus CONFIG)
if(tomlplusplus_FOUND)
    target_link_libraries(omni_sdk PRIVATE tomlplusplus::tomlplusplus)
else()
    message(WARNING "toml++ not found, configuration loading will be limited")
endif()

# Testing
enable_testing()
find_package(GTest)
if(GTest_FOUND)
    add_subdirectory(tests)
else()
    message(STATUS "GTest not found, unit tests disabled")
endif()

# Installation
install(TARGETS omni_sdk
    EXPORT omni_sdk_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/omni_sdk DESTINATION include)

install(EXPORT omni_sdk_targets
    NAMESPACE omni_sdk::
    DESTINATION lib/cmake/omni_sdk
)
