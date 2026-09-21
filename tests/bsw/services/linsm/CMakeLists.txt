# LinSM service unit tests (T-021 Batch 2)
#
# Real assertions against src/bsw/services/linsm (target service_linsm).
# The test file provides its own inline Det_ReportError mock, so mock_det.c
# is intentionally NOT linked here (double-definition guard).

if(TARGET service_linsm)
    add_executable(linsm_test
        test_linsm.c
    )

    target_include_directories(linsm_test PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/linsm/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(linsm_test PRIVATE
            service_linsm
            ${TEST_FRAMEWORK_LIB}
        )
    else()
        target_link_libraries(linsm_test PRIVATE service_linsm)
    endif()

    set_target_properties(linsm_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME linsm_test COMMAND linsm_test)
endif()
