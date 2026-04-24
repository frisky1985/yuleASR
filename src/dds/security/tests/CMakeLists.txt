# CMakeLists.txt for DDS-Security unit tests

cmake_minimum_required(VERSION 3.10)

# Security tests
add_executable(test_dds_security
    test_dds_auth.c
    test_dds_access.c
    test_dds_crypto.c
    test_dds_security_manager.c
    test_runner.c
)

target_include_directories(test_dds_security
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../tests/unity
)

target_link_libraries(test_dds_security
    PRIVATE
        dds_security
        unity
)

# Add test
add_test(NAME dds_security COMMAND test_dds_security)
