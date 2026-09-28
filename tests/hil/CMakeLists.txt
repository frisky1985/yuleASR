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

# 本文件是 standalone 工程 (CI: `cmake ../tests/hil` 于 build-hil 内)，因此
# CMAKE_SOURCE_DIR 指向 tests/hil 而非仓库根；用相对本文件的方式定位仓库根，
# 从而把 AUTOSAR 平台类型头 (include/autosar/Platform_Types.h) 纳入搜索路径。
# 缺少该路径时 test_hil_can.c / test_hil_diag.c 会报 "unknown type name 'uint8'"。
get_filename_component(YULE_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

target_include_directories(hil_interface_test PRIVATE
    ${CMOCKA_INCLUDE_DIRS}
    ${YULE_REPO_ROOT}/include/autosar
)

target_link_directories(hil_interface_test PRIVATE ${CMOCKA_LIBRARY_DIRS})
target_link_libraries(hil_interface_test PRIVATE ${CMOCKA_LIBRARIES})

# add_test() 需要 enable_testing() 才会注册到 CTest；CI 目前直接执行
# ./hil_interface_test，此处补上以便 `ctest` 也可发现该用例。
enable_testing()
add_test(NAME hil_interface_test COMMAND hil_interface_test)
