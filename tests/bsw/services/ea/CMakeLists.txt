# Ea (EEPROM Abstraction) unit tests — substantiated against the production
# Ea implementation (library target ecual_ea).
# Two executables:
#   ea_test      — module API tests (Ea_Init/Read/Write/Invalidate/Erase/...)
#   ea_svc_test  — service-façade tests (SWS_EaSvc_* requirements mapped onto
#                  the Ea.h public API; there is no separate EaSvc module)
# The only external dependencies (Det_ReportError, Eep_* driver API) are
# provided by the test sources themselves, so no mocks from tests/mocks are
# linked.

cmake_minimum_required(VERSION 3.16)

set(EA_TEST_INCLUDES
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/ea/include
    ${CMAKE_SOURCE_DIR}/src/bsw/services/memif/include
    ${CMAKE_SOURCE_DIR}/include/autosar
    ${CMAKE_SOURCE_DIR}/config/input/ecual
    ${CMAKE_SOURCE_DIR}/src/common/include
    ${CMAKE_SOURCE_DIR}/src/bsw/ecual/fee/include
)

set(EA_TEST_COMPILE_OPTIONS
    -Wall
    -Wextra
    -g
    -O0
)

# ---------------------------------------------------------------------------
# SUT library fix-up (no src/ file is modified):
# src/bsw/ecual/CMakeLists.txt omits src/common/include, but Ea.c includes the
# ECUAL MemIf.h (src/bsw/ecual/memif/include/MemIf.h) which — since a Sep 3
# header edit — includes ModuleId.h from src/common/include. Without this the
# ecual_ea library itself cannot compile (also breaks build-native if rebuilt).
# The services layer (src/bsw/services/CMakeLists.txt) already adds this dir.
# ---------------------------------------------------------------------------
if(TARGET ecual_ea)
    target_include_directories(ecual_ea PRIVATE ${CMAKE_SOURCE_DIR}/src/common/include)
endif()

# ---------------------------------------------------------------------------
# ea_test
# ---------------------------------------------------------------------------
add_executable(ea_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ea.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eep_stubs.c
)

target_include_directories(ea_test PRIVATE ${EA_TEST_INCLUDES})
target_compile_options(ea_test PRIVATE ${EA_TEST_COMPILE_OPTIONS})

set_target_properties(ea_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET ecual_ea)
    target_link_libraries(ea_test PRIVATE ecual_ea)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ea_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ea_test COMMAND ea_test)

# ---------------------------------------------------------------------------
# ea_svc_test
# ---------------------------------------------------------------------------
add_executable(ea_svc_test
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ea_svc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/eep_stubs.c
)

target_include_directories(ea_svc_test PRIVATE ${EA_TEST_INCLUDES})
target_compile_options(ea_svc_test PRIVATE ${EA_TEST_COMPILE_OPTIONS})

set_target_properties(ea_svc_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET ecual_ea)
    target_link_libraries(ea_svc_test PRIVATE ecual_ea)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(ea_svc_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME ea_svc_test COMMAND ea_svc_test)

message(STATUS "  Unit test: ea_test, ea_svc_test (EEPROM Abstraction)")
