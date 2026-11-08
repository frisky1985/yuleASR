# src/application/diagnostic_manager/tests/CMakeLists.txt
# DiagnosticManager SWC host unit tests

add_executable(test_diagnostic_manager
    test_diagnostic_manager.c
)

target_link_libraries(test_diagnostic_manager PRIVATE diagnostic_manager)

add_test(NAME diagnostic_manager COMMAND test_diagnostic_manager)
