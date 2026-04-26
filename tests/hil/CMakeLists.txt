# HIL Interface CMake Configuration
cmake_minimum_required(VERSION 3.10)
project(hil_interface C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source files
set(HIL_SOURCES
    hil_interface.c
)

set(HIL_HEADERS
    hil_interface.h
)

# Create library
add_library(hil_interface STATIC ${HIL_SOURCES})

# Include directories
target_include_directories(hil_interface PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Platform detection
if(WIN32)
    target_compile_definitions(hil_interface PRIVATE _WIN32)
elseif(UNIX)
    target_compile_definitions(hil_interface PRIVATE _POSIX_C_SOURCE=200809L)
    target_link_libraries(hil_interface pthread)
endif()

# Test executable
enable_testing()

add_executable(hil_interface_test
    test_hil_interface.c
)

target_link_libraries(hil_interface_test
    hil_interface
)

add_test(NAME hil_interface_test COMMAND hil_interface_test)

# Installation
install(TARGETS hil_interface
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${HIL_HEADERS}
    DESTINATION include/hil
)
