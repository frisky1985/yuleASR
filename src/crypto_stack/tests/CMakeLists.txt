# Crypto Stack Tests CMakeLists.txt
# 集成: 由 src/crypto_stack/CMakeLists.txt 在 BUILD_TESTING 时挂载
# 测试文件使用 assert 风格 (自带 TEST_ASSERT 宏), 无需 Unity

cmake_minimum_required(VERSION 3.16)

set(CRYPTO_STACK_TESTS
    test_csm
    test_cryif
    test_keym
    test_secoc_core
    test_secoc_freshness
)

foreach(test_name ${CRYPTO_STACK_TESTS})
    add_executable(${test_name}
        ${test_name}.c
    )

    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../csm
        ${CMAKE_CURRENT_SOURCE_DIR}/../cryif
        ${CMAKE_CURRENT_SOURCE_DIR}/../keym
        ${CMAKE_CURRENT_SOURCE_DIR}/../secoc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../dds/security
        ${CMAKE_CURRENT_SOURCE_DIR}/../../dds/rtps
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common/types
        ${PROJECT_SOURCE_DIR}/include/autosar
    )

    target_link_libraries(${test_name} PRIVATE crypto_stack)

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Test runner 聚合目标
add_custom_target(run_crypto_stack_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${CRYPTO_STACK_TESTS}
)
