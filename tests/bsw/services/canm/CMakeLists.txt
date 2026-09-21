# CanNm service unit tests (T-021 Batch 2)
#
# Real assertions against src/bsw/services/canm (target service_canm).
# The test file provides its own inline Det_ReportError mock, so mock_det.c
# is intentionally NOT linked here.

if(TARGET service_canm)
    add_executable(canm_test
        test_canm.c
        stubs.c
    )

    target_include_directories(canm_test PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/canm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nm/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(canm_test PRIVATE
            service_canm
            ${TEST_FRAMEWORK_LIB}
        )
    else()
        target_link_libraries(canm_test PRIVATE service_canm)
    endif()

    set_target_properties(canm_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME canm_test COMMAND canm_test)
endif()
