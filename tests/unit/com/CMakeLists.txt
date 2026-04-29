# COM Module Unit Tests CMakeLists.txt

cmake_minimum_required(VERSION 3.14)

# COM Test Suite
set(COM_TEST_SOURCES
    test_com_init.c
    test_com_signal.c
    test_com_signalgroup.c
    test_com_main.c
    test_com_transmission.c
    test_com_confirmation.c
    test_com_txmode.c
    test_com_packing.c
    test_com_deadline_monitoring.c
    test_com_error_handling.c
)

# Create test executables for each test file
foreach(test_source ${COM_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    
    add_executable(${test_name}
        ${test_source}
        ${CMAKE_SOURCE_DIR}/src/autosar/classic/com/Com.c
        ${CMAKE_SOURCE_DIR}/src/autosar/classic/com/Com_Main.c
        ${CMAKE_SOURCE_DIR}/src/autosar/classic/com/Com_Signal.c
        ${CMAKE_SOURCE_DIR}/src/autosar/classic/com/Com_Transmit.c
        ${CMAKE_SOURCE_DIR}/src/autosar/classic/com/Com_Confirmation.c
        ${CMAKE_SOURCE_DIR}/src/autosar/classic/com/Com_TxMode.c
        ${CMAKE_SOURCE_DIR}/src/autosar/classic/com/Com_DeadlineMon.c
        ${CMAKE_SOURCE_DIR}/src/autosar/classic/com/Com_ErrorHandling.c
    )
    
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/autosar/classic/com
        ${CMAKE_SOURCE_DIR}/src/autosar/classic/com
        ${CMAKE_SOURCE_DIR}/tests/unity
        ${CMAKE_SOURCE_DIR}/tests/mocks
    )
    
    target_link_libraries(${test_name} PRIVATE
        unity
    )
    
    target_compile_definitions(${test_name} PRIVATE
        UNIT_TEST
        COM_DEV_ERROR_DETECT=STD_ON
        COM_MAX_SIGNALS=32
        COM_MAX_SIGNAL_GROUPS=16
        COM_MAX_IPDUS=16
        COM_MAX_IPDU_GROUPS=8
        COM_MAX_SHADOW_BUFFER_SIZE=64
        COM_MAX_TX_REQUESTS=32
        COM_MAX_RETRY_QUEUE_SIZE=16
    )
    
    add_test(NAME com_${test_name} COMMAND ${test_name})
endforeach()

# Combined COM test target
add_custom_target(test-com-all
    COMMAND ${CMAKE_CTEST_COMMAND} -R com_ --output-on-failure
    DEPENDS ${COM_TEST_SOURCES}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all COM module unit tests"
)

# Coverage target for COM module
if(ENABLE_COVERAGE)
    add_custom_target(coverage-com
        COMMAND ${CMAKE_COMMAND} -E echo "Generating COM module coverage report..."
        COMMAND ${LCOV_PATH} --capture --directory ${CMAKE_BINARY_DIR} 
                --output-file ${CMAKE_BINARY_DIR}/com_coverage.info
                --include "*/autosar/classic/com/*"
        COMMAND ${GENHTML_PATH} ${CMAKE_BINARY_DIR}/com_coverage.info
                --output-directory ${CMAKE_BINARY_DIR}/com_coverage_html
        COMMENT "Generating COM module coverage report"
    )
endif()
