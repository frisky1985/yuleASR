# src/application/storage_manager/tests/CMakeLists.txt
# StorageManager SWC host unit tests

add_executable(test_storage_manager
    test_storage_manager.c
)

target_link_libraries(test_storage_manager PRIVATE storage_manager)

add_test(NAME storage_manager COMMAND test_storage_manager)
