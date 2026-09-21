# FiM unit tests
# SUT: src/bsw/services/fim (service_fim). FiM.c depends on Det_ReportError
# (mocked in test_fim_svc.c) and Dem_GetEventFailed (mocked in test_fim_svc.c).
# NEVER link tests/mocks/mock_det.c here — the test defines its own Det_ReportError.
# Target name is fim_svc_test per T-021 assignment.
add_executable(fim_svc_test
    test_fim_svc.c
)

target_include_directories(fim_svc_test PRIVATE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/fim/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/dem/include
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(fim_svc_test PRIVATE -Wall -Wextra -g -O0)
set_target_properties(fim_svc_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_fim)
    target_link_libraries(fim_svc_test PRIVATE service_fim)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(fim_svc_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME fim_svc_test COMMAND fim_svc_test)
