# Tests directory CMakeLists.txt

# Test executable
add_executable(test_result
    test_result.cpp
)

target_link_libraries(test_result
    omni_sdk
    GTest::GTest
    GTest::Main
)

target_include_directories(test_result PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Register test
add_test(NAME test_result COMMAND test_result)
