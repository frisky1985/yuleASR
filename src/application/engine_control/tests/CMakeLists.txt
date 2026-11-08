# src/application/engine_control/tests/CMakeLists.txt
# EngineControl SWC host unit tests

add_executable(test_engine_control
    test_engine_control.c
)

target_link_libraries(test_engine_control PRIVATE engine_control)

add_test(NAME engine_control COMMAND test_engine_control)
