# Crypto Stack Tests CMakeLists.txt

# Include Unity
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH CRYPTO_STACK_DIR)
cmake_path(GET CRYPTO_STACK_DIR PARENT_PATH SRC_DIR)
cmake_path(GET SRC_DIR PARENT_PATH PROJECT_ROOT)

set(UNITY_DIR ${PROJECT_ROOT}/tests/unity)

# SecOC Core Tests
add_executable(test_secoc_core
    test_secoc_core.c
    ${UNITY_DIR}/unity.c
)

target_include_directories(test_secoc_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UNITY_DIR}
    ${CRYPTO_STACK_DIR}/secoc
    ${PROJECT_ROOT}/src/dds/security
    ${PROJECT_ROOT}/src/dds/rtps
    ${PROJECT_ROOT}/src/common
)

target_link_libraries(test_secoc_core PRIVATE
    secoc
)

add_test(NAME test_secoc_core COMMAND test_secoc_core)

# SecOC Freshness Tests
add_executable(test_secoc_freshness
    test_secoc_freshness.c
    ${UNITY_DIR}/unity.c
)

target_include_directories(test_secoc_freshness PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${UNITY_DIR}
    ${CRYPTO_STACK_DIR}/secoc
    ${PROJECT_ROOT}/src/dds/security
    ${PROJECT_ROOT}/src/dds/rtps
    ${PROJECT_ROOT}/src/common
)

target_link_libraries(test_secoc_freshness PRIVATE
    secoc
)

add_test(NAME test_secoc_freshness COMMAND test_secoc_freshness)

# Test runner
add_custom_target(run_secoc_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_secoc_core test_secoc_freshness
)
