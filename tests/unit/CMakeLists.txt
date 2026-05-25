# Unit Tests CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Test executable for each module
set(UNIT_TEST_MODULES
    PduR
    NvM
    Com
    Dem
    Dcm
    WdgM
)

# Add subdirectory tests (RTE, etc.)
add_subdirectory(rte)

foreach(module ${UNIT_TEST_MODULES})
    # Check if test file exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${module}_test.c)
        set(TEST_NAME ${module}_UnitTest)
        add_executable(${TEST_NAME} 
            ${CMAKE_CURRENT_SOURCE_DIR}/${module}_test.c
        )
        
        target_include_directories(${TEST_NAME}
            PRIVATE
                ${TEST_FRAMEWORK_INCLUDE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../../src/bsw/services/${module}/include
        )
        
        # Link with module library (use object files for testing)
        if(TARGET ${module})
            target_link_libraries(${TEST_NAME} PRIVATE ${module})
        endif()
        
        if(TEST_FRAMEWORK_LIB)
            target_link_libraries(${TEST_NAME} PRIVATE ${TEST_FRAMEWORK_LIB})
        endif()
        
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        
        set_target_properties(${TEST_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
        )
        
        message(STATUS "  Unit test: ${TEST_NAME}")
    endif()
endforeach()

# Test utilities
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_utils.c)
    add_library(TestUtils STATIC ${CMAKE_CURRENT_SOURCE_DIR}/test_utils.c)
    target_include_directories(TestUtils
        PUBLIC 
            ${TEST_FRAMEWORK_INCLUDE}
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
