# Tests directory CMakeLists.txt

# Test executables
add_executable(test_result test_result.cpp)
add_executable(test_framework test_framework.cpp)
add_executable(test_integration test_integration.cpp)

# Link test_result
target_link_libraries(test_result
    omni_sdk
    GTest::GTest
    GTest::Main
    GTest::Mock
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
    GTest::Mock
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
    GTest::Mock
    pthread
)

target_include_directories(test_integration PUBLIC
    ${C_SOURCE_DIR}/include
)

add_test(NAME test_integration COMMAND test_integration)
