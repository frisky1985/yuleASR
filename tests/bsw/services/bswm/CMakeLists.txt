# BswM unit tests — substantiated against the production BswM implementation.
# Both test files define their own Det_ReportError mock, so tests/mocks/mock_det.c
# must NOT be linked (duplicate symbol).

cmake_minimum_required(VERSION 3.16)

set(BSWM_TEST_INCLUDES
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/bswm/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
    ${CMAKE_SOURCE_DIR}/src/bsw/os/include
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/config/input/services
)

add_executable(bswm_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_bswm.c
)

add_executable(bswm_svc_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_bswm_svc.c
)

foreach(bswm_target bswm_test bswm_svc_test)
    target_include_directories(${bswm_target}
        PRIVATE
            ${BSWM_TEST_INCLUDES}
    )

    target_compile_options(${bswm_target} PRIVATE
        -Wall
        -Wextra
        -g
        -O0
    )

    set_target_properties(${bswm_target} PROPERTIES
        C_STANDARD 99
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    if(TARGET service_bswm)
        target_link_libraries(${bswm_target} PRIVATE service_bswm)
    endif()

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(${bswm_target} PRIVATE ${TEST_FRAMEWORK_LIB})
    endif()
endforeach()

add_test(NAME bswm_test COMMAND bswm_test)
add_test(NAME bswm_svc_test COMMAND bswm_svc_test)

message(STATUS "  Unit test: bswm_test, bswm_svc_test (BSW Mode Manager)")
