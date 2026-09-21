# LdCom unit tests
# SUT: src/bsw/services/ldcom (service_ldcom). The test file defines its own
# Det_ReportError mock, so tests/mocks/mock_det.c must NOT be linked.
# LdCom.h includes Com_Types.h which includes PduR.h -> pdur/include needed.
add_executable(ldcom_test
    test_ldcom.c
)

target_include_directories(ldcom_test PRIVATE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/ldcom/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/include/autosar/classic/com
)

target_compile_options(ldcom_test PRIVATE -Wall -Wextra -g -O0)
set_target_properties(ldcom_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_ldcom)
    target_link_libraries(ldcom_test PRIVATE service_ldcom)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ldcom_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ldcom_test COMMAND ldcom_test)
