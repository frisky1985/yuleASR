# CanSM service unit tests (T-021 Batch 2)
#
# Real assertions against src/bsw/services/cansm (target service_cansm).
# The test file provides its own inline Det_ReportError mock, so mock_det.c
# is intentionally NOT linked here (double-definition guard).

if(TARGET service_cansm)
    add_executable(cansm_test
        test_cansm.c
        stubs.c
    )

    target_include_directories(cansm_test PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/cansm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/comM/include
        ${CMAKE_SOURCE_DIR}/src/bsw/ecual/canif/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
        ${CMAKE_SOURCE_DIR}/config/input/ecual
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(cansm_test PRIVATE
            service_cansm
            ${TEST_FRAMEWORK_LIB}
        )
    else()
        target_link_libraries(cansm_test PRIVATE service_cansm)
    endif()

    set_target_properties(cansm_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME cansm_test COMMAND cansm_test)
endif()
