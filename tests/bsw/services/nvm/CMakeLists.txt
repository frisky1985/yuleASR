# NvM unit tests — substantiated against the production NvM implementation.
# Links the real service_nvm library; the only external dependencies
# (Det_ReportError, MemIf_* driver API) are provided by the test sources
# themselves, so no mocks from tests/mocks are linked.

cmake_minimum_required(VERSION 3.16)

set(NVM_TEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_nvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs.c
)

add_executable(nvm_test ${NVM_TEST_SRC})

target_include_directories(nvm_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nvm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/memif/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/fee/include
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ea/include
)

target_compile_options(nvm_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(nvm_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_nvm)
    target_link_libraries(nvm_test PRIVATE service_nvm)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(nvm_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME nvm_test COMMAND nvm_test)

message(STATUS "  Unit test: nvm_test (NVRAM Manager)")

# ---------------------------------------------------------------------------
# White-box internal test: compiles NvM.c directly via nvm_expose.c
# (STATIC removed) so the internal error branches can be driven directly.
# Does NOT link service_nvm to avoid duplicate symbols.
# ---------------------------------------------------------------------------
add_executable(nvm_internal_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_nvm_internal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/nvm_expose.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs.c
)

target_include_directories(nvm_internal_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nvm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nvm/src
        ${CMAKE_SOURCE_DIR}/src/bsw/services/memif/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/fee/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ea/include
)

target_compile_options(nvm_internal_test PRIVATE -Wall -Wextra -g -O0)

set_target_properties(nvm_internal_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(nvm_internal_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME nvm_internal_test COMMAND nvm_internal_test)
message(STATUS "  Unit test: nvm_internal_test (NvM white-box internals)")
