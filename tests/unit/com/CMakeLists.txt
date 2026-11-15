# Legacy Com unit test (migrated out of src/bsw/services/com/src/Com_test.c).
# Links the real service_com library; mocks for PduR and Det are provided by
# the test source itself.

cmake_minimum_required(VERSION 3.16)

add_executable(com_legacy_test ${CMAKE_CURRENT_SOURCE_DIR}/legacy_com_cases.c)

target_include_directories(com_legacy_test
    BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/com/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
        ${CMAKE_SOURCE_DIR}/src/bsw/os/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(com_legacy_test PRIVATE -Wall -Wextra -g -O0)

set_target_properties(com_legacy_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_com)
    target_link_libraries(com_legacy_test PRIVATE service_com)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(com_legacy_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME com_legacy_test COMMAND com_legacy_test)

message(STATUS "  Unit test: com_legacy_test (legacy Com cases, migrated from src)")
