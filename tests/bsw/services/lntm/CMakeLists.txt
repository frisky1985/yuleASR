# LinTp service unit tests (T-021 Batch 2)
#
# Real assertions against src/bsw/services/lntm (target service_lntm).
# The test file provides its own inline Det_ReportError mock, so mock_det.c
# is intentionally NOT linked here (double-definition guard).

if(TARGET service_lntm)
    add_executable(lntm_test
        test_lntm.c
    )

    target_include_directories(lntm_test PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/lntm/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/src/common/include
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(lntm_test PRIVATE
            service_lntm
            ${TEST_FRAMEWORK_LIB}
        )
    else()
        target_link_libraries(lntm_test PRIVATE service_lntm)
    endif()

    set_target_properties(lntm_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME lntm_test COMMAND lntm_test)
endif()
