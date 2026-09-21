# MemIf unit tests — substantiated against the production MemIf implementation.
# Links the real service_memif library; the only external dependency
# (Det_ReportError) is provided by the test file itself, so no mocks are linked.
# MEMIF_FEE_ENABLED/MEMIF_EA_ENABLED are never defined by the SUT config, so no
# Fee_*/Ea_* symbols are required at link time.

cmake_minimum_required(VERSION 3.16)

set(MEMIF_SVC_TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/test_memif_svc.c)

add_executable(memif_svc_test ${MEMIF_SVC_TEST_SRC})

target_include_directories(memif_svc_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/memif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/fee/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ea/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(memif_svc_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(memif_svc_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_memif)
    target_link_libraries(memif_svc_test PRIVATE service_memif)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(memif_svc_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME memif_svc_test COMMAND memif_svc_test)

message(STATUS "  Unit test: memif_svc_test (Memory Interface)")
