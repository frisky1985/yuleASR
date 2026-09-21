# KeyM unit tests
# SUT: src/bsw/services/keym (service_keym). KeyM.c only depends on Det,
# which is mocked inside test_keym.c (never link tests/mocks/mock_det.c).
add_executable(keym_test
    test_keym.c
)

target_include_directories(keym_test PRIVATE
    ${TEST_FRAMEWORK_INCLUDE}
    ${CMAKE_SOURCE_DIR}/src/bsw/services/keym/include
    ${CMAKE_SOURCE_DIR}/include/autosar
)

target_compile_options(keym_test PRIVATE -Wall -Wextra -g -O0)
set_target_properties(keym_test PROPERTIES
    C_STANDARD 99
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

if(TARGET service_keym)
    target_link_libraries(keym_test PRIVATE service_keym)
endif()

if(TEST_FRAMEWORK_LIB)
    target_link_libraries(keym_test PRIVATE ${TEST_FRAMEWORK_LIB})
endif()

add_test(NAME keym_test COMMAND keym_test)
