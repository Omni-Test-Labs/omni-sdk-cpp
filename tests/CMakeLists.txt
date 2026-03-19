# Tests directory CMakeLists.txt

# Test executables
add_executable(test_result test_result.cpp)
add_executable(test_framework test_framework.cpp)
add_executable(test_integration test_integration.cpp)

# Conditionally add client tests
if(OMNI_SDK_ENABLE_SERIAL)
    add_executable(test_serial_client test_serial_client.cpp)
    target_link_libraries(test_serial_client
        omni_sdk
        GTest::GTest
        GTest::Main
        pthread
    )
    target_include_directories(test_serial_client PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    )
    add_test(NAME test_serial_client COMMAND test_serial_client)
endif()

if(OMNI_SDK_ENABLE_SSH)
    add_executable(test_ssh_client test_ssh_client.cpp)
    target_link_libraries(test_ssh_client
        omni_sdk
        GTest::GTest
        GTest::Main
        pthread
    )
    target_include_directories(test_ssh_client PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    )
    add_test(NAME test_ssh_client COMMAND test_ssh_client)
endif()

# Link test_result
target_link_libraries(test_result
    omni_sdk
    GTest::GTest
    GTest::Main
    pthread
)

target_include_directories(test_result PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME test_result COMMAND test_result)

# Link test_framework
target_link_libraries(test_framework
    omni_sdk
    GTest::GTest
    GTest::Main
    pthread
)

target_include_directories(test_framework PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME test_framework COMMAND test_framework)

# Link test_integration
target_link_libraries(test_integration
    omni_sdk
    GTest::GTest
    GTest::Main
    pthread
)

target_include_directories(test_integration PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME test_integration COMMAND test_integration)
