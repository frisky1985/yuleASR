# Legacy PduR unit test (migrated out of src/bsw/services/pdur/src/PduR_test.c).
# Links the real service_pdur library; mocks for CanIf/Com/Det are provided by
# the test source itself.

cmake_minimum_required(VERSION 3.16)

add_executable(pdur_legacy_test ${CMAKE_CURRENT_SOURCE_DIR}/legacy_pdur_cases.c)

target_include_directories(pdur_legacy_test
    BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/com/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/os/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(pdur_legacy_test PRIVATE -Wall -Wextra -g -O0)

set_target_properties(pdur_legacy_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_pdur)
    target_link_libraries(pdur_legacy_test PRIVATE service_pdur)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(pdur_legacy_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME pdur_legacy_test COMMAND pdur_legacy_test)

message(STATUS "  Unit test: pdur_legacy_test (legacy PduR cases, migrated from src)")
