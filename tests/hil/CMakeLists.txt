# yuleASR HIL C interface tests
# Builds test_hil_runner (cmocka-based) when cmocka is available.
# Hardware-dependent cases skip() at runtime; the build validates the
# HIL test harness compiles against the real BSW headers.

cmake_minimum_required(VERSION 3.16)
project(yuleASR_HIL_Tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig QUIET)
pkg_check_modules(CMOCKA QUIET cmocka)

if(NOT CMOCKA_FOUND)
    message(WARNING "cmocka not found — HIL C tests skipped (install libcmocka-dev to enable)")
    return()
endif()

add_executable(hil_interface_test
    test_hil_runner.c
    test_hil_can.c
    test_hil_diag.c
)

target_include_directories(hil_interface_test PRIVATE ${CMOCKA_INCLUDE_DIRS})
target_link_libraries(hil_interface_test PRIVATE ${CMOCKA_LIBRARIES})

add_test(NAME hil_interface_test COMMAND hil_interface_test)
