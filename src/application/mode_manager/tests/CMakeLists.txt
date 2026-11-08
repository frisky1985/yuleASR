# src/application/mode_manager/tests/CMakeLists.txt
# ModeManager SWC host unit tests

add_executable(test_mode_manager
    test_mode_manager.c
)

target_link_libraries(test_mode_manager PRIVATE mode_manager)

add_test(NAME mode_manager COMMAND test_mode_manager)
