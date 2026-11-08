# src/application/watchdog_manager/tests/CMakeLists.txt
# WatchdogManager SWC host unit tests

add_executable(test_watchdog_manager
    test_watchdog_manager.c
)

target_link_libraries(test_watchdog_manager PRIVATE watchdog_manager)

add_test(NAME watchdog_manager COMMAND test_watchdog_manager)
