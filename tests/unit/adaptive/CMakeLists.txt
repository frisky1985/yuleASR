###############################################################################
# ara::exec + ara::sm Unit Tests CMakeLists.txt
###############################################################################

cmake_minimum_required(VERSION 3.14)
project(adaptive_autosar_tests LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/../../../include
    ${CMAKE_SOURCE_DIR}/../../../src/common
    ${CMAKE_SOURCE_DIR}/../../../src/autosar/adaptive
    ${GTEST_INCLUDE_DIRS}
)

# Source files for ara::exec
set(EXEC_SOURCES
    ${CMAKE_SOURCE_DIR}/../../../src/autosar/adaptive/exec/process_manager.c
    ${CMAKE_SOURCE_DIR}/../../../src/autosar/adaptive/exec/resource_manager.c
    ${CMAKE_SOURCE_DIR}/../../../src/autosar/adaptive/exec/sandbox.c
    ${CMAKE_SOURCE_DIR}/../../../src/autosar/adaptive/exec/exec_manager.cpp
)

# Source files for ara::sm
set(SM_SOURCES
    ${CMAKE_SOURCE_DIR}/../../../src/autosar/adaptive/sm/state_machine.c
    ${CMAKE_SOURCE_DIR}/../../../src/autosar/adaptive/sm/function_group.c
    ${CMAKE_SOURCE_DIR}/../../../src/autosar/adaptive/sm/sm_interface.c
    ${CMAKE_SOURCE_DIR}/../../../src/autosar/adaptive/sm/state_manager.cpp
)

# Common headers
set(COMMON_HEADERS
    ${CMAKE_SOURCE_DIR}/../../../src/common/autosar_types.h
    ${CMAKE_SOURCE_DIR}/../../../src/common/autosar_errors.h
)

# Test executable: Process Manager
add_executable(test_exec_process_manager
    test_exec_process_manager.cpp
    ${EXEC_SOURCES}
    ${COMMON_HEADERS}
)

target_link_libraries(test_exec_process_manager
    GTest::gtest_main
    pthread
)

# Test executable: Resource Manager
add_executable(test_exec_resource_manager
    test_exec_resource_manager.cpp
    ${EXEC_SOURCES}
    ${COMMON_HEADERS}
)

target_link_libraries(test_exec_resource_manager
    GTest::gtest_main
    pthread
)

# Test executable: Sandbox
add_executable(test_exec_sandbox
    test_exec_sandbox.cpp
    ${EXEC_SOURCES}
    ${COMMON_HEADERS}
)

target_link_libraries(test_exec_sandbox
    GTest::gtest_main
    pthread
)

# Test executable: State Machine
add_executable(test_sm_state_machine
    test_sm_state_machine.cpp
    ${SM_SOURCES}
    ${COMMON_HEADERS}
)

target_link_libraries(test_sm_state_machine
    GTest::gtest_main
    pthread
)

# Test executable: Function Group
add_executable(test_sm_function_group
    test_sm_function_group.cpp
    ${SM_SOURCES}
    ${COMMON_HEADERS}
)

target_link_libraries(test_sm_function_group
    GTest::gtest_main
    pthread
)

# Test executable: SM Interface
add_executable(test_sm_interface
    test_sm_interface.cpp
    ${SM_SOURCES}
    ${COMMON_HEADERS}
)

target_link_libraries(test_sm_interface
    GTest::gtest_main
    pthread
)

# Test executable: Integration
add_executable(test_exec_sm_integration
    test_exec_sm_integration.cpp
    ${EXEC_SOURCES}
    ${SM_SOURCES}
    ${COMMON_HEADERS}
)

target_link_libraries(test_exec_sm_integration
    GTest::gtest_main
    pthread
)

# Add tests
add_test(NAME ProcessManager COMMAND test_exec_process_manager)
add_test(NAME ResourceManager COMMAND test_exec_resource_manager)
add_test(NAME Sandbox COMMAND test_exec_sandbox)
add_test(NAME StateMachine COMMAND test_sm_state_machine)
add_test(NAME FunctionGroup COMMAND test_sm_function_group)
add_test(NAME SMInterface COMMAND test_sm_interface)
add_test(NAME Integration COMMAND test_exec_sm_integration)

# Coverage options (if enabled)
if(ENABLE_COVERAGE)
    target_compile_options(test_exec_process_manager PRIVATE --coverage)
    target_link_options(test_exec_process_manager PRIVATE --coverage)
    
    target_compile_options(test_exec_resource_manager PRIVATE --coverage)
    target_link_options(test_exec_resource_manager PRIVATE --coverage)
    
    target_compile_options(test_exec_sandbox PRIVATE --coverage)
    target_link_options(test_exec_sandbox PRIVATE --coverage)
    
    target_compile_options(test_sm_state_machine PRIVATE --coverage)
    target_link_options(test_sm_state_machine PRIVATE --coverage)
    
    target_compile_options(test_sm_function_group PRIVATE --coverage)
    target_link_options(test_sm_function_group PRIVATE --coverage)
    
    target_compile_options(test_sm_interface PRIVATE --coverage)
    target_link_options(test_sm_interface PRIVATE --coverage)
    
    target_compile_options(test_exec_sm_integration PRIVATE --coverage)
    target_link_options(test_exec_sm_integration PRIVATE --coverage)
endif()
