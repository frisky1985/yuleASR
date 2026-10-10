# LinTp unit tests — converged to canonical ecual/linTp
#
# After P0-NEW-1 duplicate-module convergence, the canonical LinTp
# implementation lives in ecual/linTp. The services/lntm copy is shimmed.
# The test file provides its own inline Det_ReportError mock, so mock_det.c
# is intentionally NOT linked here (double-definition guard).

if(TARGET ecual_linTp)
    add_executable(lntm_test
        test_lntm.c
    )

    target_include_directories(lntm_test PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/linTp/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/pdur/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/config/input/mcal
        ${CMAKE_SOURCE_DIR}/src/common/include
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(lntm_test PRIVATE
            ecual_linTp
            ${TEST_FRAMEWORK_LIB}
        )
    else()
        target_link_libraries(lntm_test PRIVATE ecual_linTp)
    endif()

    set_target_properties(lntm_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME lntm_test COMMAND lntm_test)
endif()
