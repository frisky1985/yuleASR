# src/application/io_control/tests/CMakeLists.txt
# IOControl SWC host unit tests

add_executable(test_io_control
    test_io_control.c
)

target_link_libraries(test_io_control PRIVATE io_control)

add_test(NAME io_control COMMAND test_io_control)
