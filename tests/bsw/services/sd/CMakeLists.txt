# Sd unit tests
# SUT: src/bsw/services/sd (service_sd). The test file defines its own
# Det_ReportError mock, so tests/mocks/mock_det.c must NOT be linked.
add_executable(sd_test
    test_sd.c
)

target_include_directories(sd_test PRIVATE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/sd/include
    ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(sd_test PRIVATE -Wall -Wextra -g -O0)
set_target_properties(sd_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_sd)
    target_link_libraries(sd_test PRIVATE service_sd)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(sd_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME sd_test COMMAND sd_test)
