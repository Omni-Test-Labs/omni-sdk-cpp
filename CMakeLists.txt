cmake_minimum_required(VERSION 3.15)
project(omni-sdk-cpp VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SDK_SOURCES
    src/result.cpp
    src/config.cpp
    src/omni_sdk.cpp
)

set(HEADERS
    include/omni_sdk/result.h
    include/omni_sdk/client.h
    include/omni_sdk/device.h
    include/omni_sdk/config.h
    include/omni_sdk/omni_sdk.h
)

# Main library
add_library(omni_sdk ${SDK_SOURCES} ${HEADERS})
target_include_directories(omni_sdk PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Find dependencies
find_package(Threads REQUIRED)

# Option to build with libssh2 (SSH client)
option(OMNI_SDK_ENABLE_SSH "Enable SSH client support" OFF)
if(OMNI_SDK_ENABLE_SSH)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSH2 REQUIRED libssh2)
    target_include_directories(omni_sdk PRIVATE ${LIBSSH2_INCLUDE_DIRS})
    target_link_libraries(omni_sdk PRIVATE ${LIBSSH2_LIBRARIES})
endif()

# Option to build with Boost.Asio (Serial client)
option(OMNI_SDK_ENABLE_SERIAL "Enable Serial client support" OFF)
if(OMNI_SDK_ENABLE_SERIAL)
    find_package(Boost REQUIRED COMPONENTS system)
    target_include_directories(omni_sdk PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(omni_sdk PRIVATE Boost::system)
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
