# Test CMakeLists for Boot module
cmake_minimum_required(VERSION 3.16)

# Collect test sources
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE boot)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
