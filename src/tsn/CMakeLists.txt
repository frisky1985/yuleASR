# TSN Protocol Stack CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# TSN Library
set(TSN_SOURCES
    gptp/gptp.c
    tas/tas.c
    fp/frame_preemption.c
    cbs/cbs.c
    srp/stream_reservation.c
    tsn_stack.c
)

set(TSN_HEADERS
    gptp/gptp.h
    tas/tas.h
    fp/frame_preemption.h
    cbs/cbs.h
    srp/stream_reservation.h
    tsn_stack.h
)

# Create TSN library
add_library(tsn_stack STATIC ${TSN_SOURCES})

target_include_directories(tsn_stack
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(tsn_stack
    PUBLIC
        pthread
        rt
)

# Compiler flags for ASIL-D safety
target_compile_options(tsn_stack PRIVATE
    -Wall
    -Wextra
    -Werror
    -O2
    -fdata-sections
    -ffunction-sections
)

# Enable POSIX features
target_compile_definitions(tsn_stack PRIVATE
    _POSIX_C_SOURCE=200809L
)

# TSN Test Executable
add_executable(tsn_stack_test
    tests/test_tsn_stack.c
)

target_link_libraries(tsn_stack_test
    PRIVATE
        tsn_stack
        pthread
        rt
)

# Print configuration
message(STATUS "TSN Stack Configuration:")
message(STATUS "  Version: 1.0.0")
message(STATUS "  Standards: IEEE 802.1AS, 802.1Qbv, 802.1Qbu, 802.1Qav, 802.1Qcc")
message(STATUS "  Sources: ${TSN_SOURCES}")
