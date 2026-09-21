# Csm unit tests — substantiated against the production Csm implementation.
#
# The production Csm.c is compiled directly (unmodified) instead of linking
# service_csm: the latter PUBLIC-links mcal_crypto, which has a pre-existing
# build defect with YULE_ENABLE_MCAL=OFF (Crypto_S32K312_Hsm.h includes
# ModuleId.h from src/common/include, not on the mcal include path — a src/
# issue outside the scope of test substantiation). The Csm_Cfg_* backend and
# CryIf symbols are stubbed locally.

cmake_minimum_required(VERSION 3.16)

set(CSM_PROD_SRC ${CMAKE_SOURCE_DIR}/src/bsw/services/csm/src/Csm.c)

set(CSM_TEST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_csm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs.c
)

add_executable(csm_test
    ${CSM_PROD_SRC}
    ${CSM_TEST_SRC}
)

target_include_directories(csm_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/csm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/cryif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/dem/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(csm_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(csm_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(csm_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME csm_test COMMAND csm_test)

message(STATUS "  Unit test: csm_test (Crypto Service Manager)")
