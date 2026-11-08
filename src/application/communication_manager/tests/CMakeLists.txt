# src/application/communication_manager/tests/CMakeLists.txt
# CommunicationManager SWC host unit tests

add_executable(test_communication_manager
    test_communication_manager.c
)

target_link_libraries(test_communication_manager PRIVATE communication_manager)

add_test(NAME communication_manager COMMAND test_communication_manager)
