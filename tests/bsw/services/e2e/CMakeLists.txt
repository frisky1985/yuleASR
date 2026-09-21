# E2E unit tests
# SUT: src/bsw/services/e2e (service_e2e). The SUT never calls Det_ReportError
# (NULL args yield E_NOT_OK return codes), so no Det mock is required.
# Only E2E.c / E2E_P01.c objects are pulled by the linker; the P05/P06/P07
# objects that need Crc_* are not referenced by these tests.
add_executable(e2e_test
    test_e2e.c
)

target_include_directories(e2e_test PRIVATE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/e2e/include
    ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(e2e_test PRIVATE -Wall -Wextra -g -O0)
set_target_properties(e2e_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_e2e)
    target_link_libraries(e2e_test PRIVATE service_e2e)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(e2e_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME e2e_test COMMAND e2e_test)
