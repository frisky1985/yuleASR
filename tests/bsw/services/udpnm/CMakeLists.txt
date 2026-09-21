# UdpNm service unit tests (T-021 Batch 2)
#
# Real assertions against src/bsw/services/udpnm (target service_udpNm).
# Both test files provide their own inline Det_ReportError mock, so
# mock_det.c is intentionally NOT linked here (double-definition guard).
# Two executables: udpnm_test (API guards/state) and udpnm_svc_test
# (callback/service paths).

if(TARGET service_udpNm)
    add_executable(udpnm_test
        test_udpnm.c
    )

    target_include_directories(udpnm_test PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/udpnm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nm/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(udpnm_test PRIVATE
            service_udpNm
            ${TEST_FRAMEWORK_LIB}
        )
    else()
        target_link_libraries(udpnm_test PRIVATE service_udpNm)
    endif()

    set_target_properties(udpnm_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME udpnm_test COMMAND udpnm_test)

    add_executable(udpnm_svc_test
        test_udpnm_svc.c
    )

    target_include_directories(udpnm_svc_test PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/udpnm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/nm/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
    )

    if(TEST_FRAMEWORK_LIB)
        target_link_libraries(udpnm_svc_test PRIVATE
            service_udpNm
            ${TEST_FRAMEWORK_LIB}
        )
    else()
        target_link_libraries(udpnm_svc_test PRIVATE service_udpNm)
    endif()

    set_target_properties(udpnm_svc_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME udpnm_svc_test COMMAND udpnm_svc_test)
endif()
