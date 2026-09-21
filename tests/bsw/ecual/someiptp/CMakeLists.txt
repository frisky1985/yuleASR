# SomeIpTp unit tests — substantiated against the production SOME/IP Transport
# Protocol implementation (src/bsw/services/someiptp). Links the real
# service_someiptp library; the external dependencies (Det_ReportError,
# SoAd_Transmit, SomeIpXf_RxIndication) are provided by the test source itself
# as host-safe mocks, so no mocks from tests/mocks are linked.
#
# PRODUCTION DEFECT WORKAROUND (test-side only; src/ is NOT modified):
#   SomeIpTp.c stops emitting API implementations after SomeIpTp_Transmit()
#   (an early `#define SOMEIPTP_STOP_SEC_CODE / #include "MemMap.h"` block
#   terminates the file), so the APIs SomeIpTp_CancelTransmit(),
#   SomeIpTp_RxIndication(), SomeIpTp_TxConfirmation(), SomeIpTp_MainFunction(),
#   SomeIpTp_BuildTpHeader(), SomeIpTp_ParseTpHeader() and
#   SomeIpTp_GetRxBufferStatus() are declared in SomeIpTp.h but have no
#   definition in the library. The test only exercises APIs that really exist
#   in the SUT (Init/DeInit/GetVersionInfo/Transmit/RxIndication via the
#   reassembly path reachable through SomeIpTp_Transmit callers is NOT used —
#   RxIndication is undefined, hence it is not called here).
#
# Note: src/bsw/services/someiptp/src/SomeIpTp_Test.c also defines SoAd_Transmit
# and SomeIpXf_RxIndication mocks plus a main(), but it is excluded from the
# service_someiptp library (file(GLOB) filter drops *_Test.c) and is NOT
# compiled into this test either.

cmake_minimum_required(VERSION 3.16)

add_executable(someiptp_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_someiptp.c
)

target_include_directories(someiptp_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/someiptp/include
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/include/autosar
        ${CMAKE_SOURCE_DIR}/config/input/services
)

target_compile_options(someiptp_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DUNIT_TEST
)

set_target_properties(someiptp_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_someiptp)
    target_link_libraries(someiptp_test PRIVATE service_someiptp)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(someiptp_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME someiptp_test COMMAND someiptp_test)

message(STATUS "  Unit test: someiptp_test (SOME/IP Transport Protocol)")
