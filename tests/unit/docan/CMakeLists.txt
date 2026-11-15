# Legacy DoCan unit test (migrated out of src/bsw/services/docan/src/DoCan_test.c).
# Links the real service_docan library; mocks for CanTp/Dcm/Det are provided by
# the test source itself.

cmake_minimum_required(VERSION 3.16)

add_executable(docan_legacy_test ${CMAKE_CURRENT_SOURCE_DIR}/legacy_docan_cases.c)

target_include_directories(docan_legacy_test
    BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/docan/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/cantp/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/dcm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/os/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(docan_legacy_test PRIVATE -Wall -Wextra -g -O0)

set_target_properties(docan_legacy_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_docan)
    target_link_libraries(docan_legacy_test PRIVATE service_docan)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(docan_legacy_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

# NOTE (2026-11-05): built but NOT registered with ctest yet. The migrated
# rxindication case assumes a PduMapping that the production DoCan_Lcfg does not
# provide. See fix-ledger E-3.
# add_test(NAME docan_legacy_test COMMAND docan_legacy_test)

message(STATUS "  Unit test: docan_legacy_test (legacy DoCan cases, migrated from src)")
