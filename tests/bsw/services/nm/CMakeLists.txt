# Nm service unit tests (T-021 Batch 2)
#
# Real assertions against src/bsw/services/nm (target service_nm).
# The test file provides its own inline Det_ReportError mock, so mock_det.c
# is intentionally NOT linked here (double-definition guard).

if(TARGET service_nm)
    add_executable(nm_test
        test_nm.c
    )

    target_include_directories(nm_test PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nm/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(nm_test PRIVATE
            service_nm
            ${TEST_FRAMEWORK_LIB}
        )
    else()
        target_link_libraries(nm_test PRIVATE service_nm)
    endif()

    set_target_properties(nm_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME nm_test COMMAND nm_test)
endif()
