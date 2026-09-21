# Mqtt unit tests — substantiated against the production Mqtt implementation
# Links the real service_mqtt library with TcpIp socket stubs.
# Note: the test defines its own Det_ReportError mock, so tests/mocks/mock_det.c
# is intentionally NOT linked. tests/mocks must NOT be on the include path
# either: its TcpIp.h is incomplete and would shadow the real one.

cmake_minimum_required(VERSION 3.16)

add_executable(mqtt_test
    test_mqtt.c
    stubs.c
)

target_include_directories(mqtt_test
    PRIVATE
        ${TEST_FRAMEWORK_INCLUDE}
        ${CMAKE_SOURCE_DIR}/src/bsw/services/mqtt/include
        ${CMAKE_SOURCE_DIR}/src/bsw/services/tcpip/include
        ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(mqtt_test PRIVATE
    -Wall
    -Wextra
    -g
    -O0
)

set_target_properties(mqtt_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_mqtt)
    target_link_libraries(mqtt_test PRIVATE service_mqtt)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(mqtt_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME mqtt_test COMMAND mqtt_test)

message(STATUS "  Unit test: mqtt_test (MQTT client service)")
