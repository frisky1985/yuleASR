# Mqtt unit tests — substantiated against the production Mqtt implementation.
#
# Targets:
#   mqtt_test         - Mqtt.c core (state machine, packet encoding, API paths).
#                       Compiles a GENERATED white-box copy of Mqtt.c (see
#                       below) plus the real Mqtt_Lcfg.c / Mqtt_Tls.c, and
#                       links the REAL mbedTLS (mbedtls/mbedx509/mbedcrypto)
#                       + mcal_crypto static pool, like service_mqtt does.
#   mqtt_tls_test     - Mqtt_Tls.c compiled directly against the real mbedtls
#                       headers, with every mbedtls function replaced by the
#                       configurable stubs in mbedtls_stubs.c (success + failure
#                       paths).
#   mqtt_certmgr_test - Mqtt_CertMgr.c compiled directly against the real
#                       mbedtls headers with the same mbedtls stubs.
#
# Notes:
# - The tests define their own Det_ReportError mock, so tests/mocks/mock_det.c
#   is intentionally NOT linked. tests/mocks must NOT be on the include path
#   either: its TcpIp.h is incomplete and would shadow the real one.
# - The REAL mbedtls headers come first on the include path on purpose:
#   mqtt/include contains a minimal local mbedtls/x509_crt.h stub whose struct
#   layout does NOT match the real library. Compiling Mqtt_Tls.c/Mqtt_CertMgr.c
#   against it while linking anything from real mbedTLS would corrupt memory.

cmake_minimum_required(VERSION 3.16)

set(MQTT_SRC_DIR ${CMAKE_SOURCE_DIR}/src/bsw/services/mqtt/src)
set(MQTT_INC_DIR ${CMAKE_SOURCE_DIR}/src/bsw/services/mqtt/include)

#------------------------------------------------------------------------------
# Generated white-box copy of Mqtt.c (mqtt_test only)
#
# The MQTT state machine never reaches MQTT_STATE_CONNECTED through the public
# API (no CONNACK processing exists in the production code), so all CONNECTED
# branches are unreachable otherwise. The generated copy demotes the three
# static module variables of Mqtt.c to external linkage and appends a tiny
# test-only accessor appendix. The production source file is NOT modified;
# the copy lives under tests/bsw/services/mqtt/whitebox/ (generated).
#------------------------------------------------------------------------------
set(MQTT_WB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/whitebox/src/bsw/services/mqtt/src)
set(MQTT_WB_FILE ${MQTT_WB_DIR}/Mqtt.c)
file(MAKE_DIRECTORY ${MQTT_WB_DIR})

file(READ ${MQTT_SRC_DIR}/Mqtt.c MQTT_WB_TEXT)
string(REPLACE
    "static boolean Mqtt_Initialized = FALSE;"
    "boolean Mqtt_Initialized = FALSE;"
    MQTT_WB_TEXT "${MQTT_WB_TEXT}")
string(REPLACE
    "static Mqtt_InternalConnectionType Mqtt_Connections[MQTT_MAX_CONNECTIONS];"
    "Mqtt_InternalConnectionType Mqtt_Connections[MQTT_MAX_CONNECTIONS];"
    MQTT_WB_TEXT "${MQTT_WB_TEXT}")
string(REPLACE
    "static uint32 Mqtt_TickCount = 0U;"
    "uint32 Mqtt_TickCount = 0U;"
    MQTT_WB_TEXT "${MQTT_WB_TEXT}")

set(MQTT_WB_APPEND [=[
/*============================================================================
 * TEST-ONLY APPENDIX (generated for white-box unit tests).
 * Not part of the production source; exposes internal state so the test can
 * simulate broker responses (e.g. CONNACK -> CONNECTED) that the production
 * state machine cannot reach on its own.
 *===========================================================================*/
void Mqtt_Test_SetState(uint8 id, uint8 newState);
void Mqtt_Test_SetState(uint8 id, uint8 newState)
{
    Mqtt_Connections[id].state = (Mqtt_ConnectionStateType)newState;
}

void Mqtt_Test_SetPendingPing(uint8 id, uint8 v);
void Mqtt_Test_SetPendingPing(uint8 id, uint8 v)
{
    Mqtt_Connections[id].pendingPing = (v != 0U) ? TRUE : FALSE;
}

uint16 Mqtt_Test_GetPacketIdCounter(uint8 id);
uint16 Mqtt_Test_GetPacketIdCounter(uint8 id)
{
    return Mqtt_Connections[id].packetIdCounter;
}

void Mqtt_Test_SetUseTls(uint8 id, uint8 v);
void Mqtt_Test_SetUseTls(uint8 id, uint8 v)
{
    Mqtt_Connections[id].useTls = (v != 0U) ? TRUE : FALSE;
}

void Mqtt_Test_SetTlsContext(uint8 id, void* ctx);
void Mqtt_Test_SetTlsContext(uint8 id, void* ctx)
{
    Mqtt_Connections[id].tlsContext = (Mqtt_TlsContextType)ctx;
}
]=])

# Only rewrite when the content changed (avoids rebuilding on every configure).
set(MQTT_WB_NEW_TEXT "${MQTT_WB_TEXT}\n${MQTT_WB_APPEND}")
set(MQTT_WB_OLD_TEXT "")
if(EXISTS ${MQTT_WB_FILE})
    file(READ ${MQTT_WB_FILE} MQTT_WB_OLD_TEXT)
endif()
if(NOT MQTT_WB_OLD_TEXT STREQUAL MQTT_WB_NEW_TEXT)
    file(WRITE ${MQTT_WB_FILE} "${MQTT_WB_NEW_TEXT}")
endif()

#------------------------------------------------------------------------------
# Shared include paths (REAL mbedtls headers first!)
#------------------------------------------------------------------------------
set(MQTT_TEST_INCLUDES
    ${CMAKE_SOURCE_DIR}/third_party/mbedtls/include
    ${TEST_FRAMEWORK_INCLUDE}
    ${MQTT_INC_DIR}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/tcpip/include
    ${CMAKE_SOURCE_DIR}/src/bsw/mcal/crypto/include
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/src/bsw/services/det/include
    ${CMAKE_SOURCE_DIR}/src/bsw/os/include
)

#------------------------------------------------------------------------------
# Target: mqtt_test
#------------------------------------------------------------------------------
add_executable(mqtt_test
    ${MQTT_WB_FILE}          # generated white-box copy of Mqtt.c
    ${MQTT_SRC_DIR}/Mqtt_Lcfg.c
    ${MQTT_SRC_DIR}/Mqtt_Tls.c
    ${CMAKE_SOURCE_DIR}/src/bsw/mcal/crypto/src/Crypto_MbedTLS_Mem.c
    test_mqtt.c
    stubs.c
)

target_include_directories(mqtt_test PRIVATE ${MQTT_TEST_INCLUDES})
target_compile_definitions(mqtt_test PRIVATE MBEDTLS_ALLOW_PRIVATE_ACCESS)
# Real mbedTLS 3.x ssl_setup needs ~40 KiB for its in/out buffers; the default
# mcal_crypto pool (32 KiB) makes Mqtt_Tls_CreateContext fail with ALLOC_FAILED.
# The header documents -DCRYPTO_MBEDTLS_MEM_POOL_SIZE as the override knob.
target_compile_definitions(mqtt_test PRIVATE CRYPTO_MBEDTLS_MEM_POOL_SIZE=131072U)
target_compile_options(mqtt_test PRIVATE -Wall -Wextra -Wno-deprecated-declarations -g -O0)

if(TARGET mbedtls)
    target_link_libraries(mqtt_test PRIVATE mbedtls)
endif()
if(TARGET mbedx509)
    target_link_libraries(mqtt_test PRIVATE mbedx509)
endif()
if(TARGET mbedcrypto)
    target_link_libraries(mqtt_test PRIVATE mbedcrypto)
endif()
# NOTE: mcal_crypto is deliberately NOT linked -- its Crypto_MbedTLS_Mem.c is
# compiled above with the enlarged pool instead (see CRYPTO_MBEDTLS_MEM_POOL_SIZE).
if(TEST_FRAMEWORK_LIB)
    target_link_libraries(mqtt_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

set_target_properties(mqtt_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME mqtt_test COMMAND mqtt_test)

#------------------------------------------------------------------------------
# Target: mqtt_tls_test (Mqtt_Tls.c + mbedtls stubs)
#------------------------------------------------------------------------------
add_executable(mqtt_tls_test
    ${MQTT_SRC_DIR}/Mqtt_Tls.c
    test_mqtt_tls.c
    mbedtls_stubs.c
    stubs.c
)

target_include_directories(mqtt_tls_test PRIVATE ${MQTT_TEST_INCLUDES})
target_compile_definitions(mqtt_tls_test PRIVATE MBEDTLS_ALLOW_PRIVATE_ACCESS)
target_compile_options(mqtt_tls_test PRIVATE -Wall -Wextra -Wno-deprecated-declarations -g -O0)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(mqtt_tls_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

set_target_properties(mqtt_tls_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME mqtt_tls_test COMMAND mqtt_tls_test)

#------------------------------------------------------------------------------
# Target: mqtt_certmgr_test (Mqtt_CertMgr.c + mbedtls stubs)
#------------------------------------------------------------------------------
add_executable(mqtt_certmgr_test
    ${MQTT_SRC_DIR}/Mqtt_CertMgr.c
    test_mqtt_certmgr.c
    mbedtls_stubs.c
)

target_include_directories(mqtt_certmgr_test PRIVATE ${MQTT_TEST_INCLUDES})
target_compile_definitions(mqtt_certmgr_test PRIVATE MBEDTLS_ALLOW_PRIVATE_ACCESS)
target_compile_options(mqtt_certmgr_test PRIVATE -Wall -Wextra -Wno-deprecated-declarations -g -O0)

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(mqtt_certmgr_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

set_target_properties(mqtt_certmgr_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)
add_test(NAME mqtt_certmgr_test COMMAND mqtt_certmgr_test)

message(STATUS "  Unit tests: mqtt_test, mqtt_tls_test, mqtt_certmgr_test (MQTT client service)")
