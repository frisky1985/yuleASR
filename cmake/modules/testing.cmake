# cmake/modules/testing.cmake
# Testing and code coverage configuration

#==============================================================================
# Testing Framework Setup
#==============================================================================
if(BUILD_TESTS)
    message(STATUS "Configuring testing framework")
    
    # Enable CTest
    enable_testing()
    include(CTest)
    
    # Unity testing framework (bundled)
    set(UNITY_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/tests/unity)
    if(EXISTS ${UNITY_ROOT}/unity.c)
        message(STATUS "  Using Unity testing framework (bundled)")
        set(HAVE_UNITY ON)
        
        # Unity library
        add_library(unity STATIC
            ${UNITY_ROOT}/unity.c
        )
        target_include_directories(unity PUBLIC
            ${UNITY_ROOT}
        )
        
        # Unity configuration
        target_compile_definitions(unity PUBLIC
            UNITY_OUTPUT_COLOR
            UNITY_INCLUDE_DOUBLE
        )
    else()
        message(FATAL_ERROR "Unity testing framework not found in tests/unity/")
    endif()
    
    # Test output directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
    
    # Add test subdirectories
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

#==============================================================================
# Code Coverage Configuration
#==============================================================================
if(ENABLE_COVERAGE)
    message(STATUS "Configuring code coverage")
    
    # Coverage output directory
    set(COVERAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage)
    file(MAKE_DIRECTORY ${COVERAGE_OUTPUT_DIR})
    
    # Coverage target using gcovr
    if(HAVE_GCOVR)
        add_custom_target(coverage-gcovr
            COMMAND ${GCOVR_PATH}
                -r ${CMAKE_CURRENT_SOURCE_DIR}
                --object-directory=${CMAKE_BINARY_DIR}
                --xml
                -o ${COVERAGE_OUTPUT_DIR}/coverage.xml
            COMMAND ${GCOVR_PATH}
                -r ${CMAKE_CURRENT_SOURCE_DIR}
                --object-directory=${CMAKE_BINARY_DIR}
                --html
                --html-details
                -o ${COVERAGE_OUTPUT_DIR}/coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report with gcovr"
        )
    endif()
    
    # Coverage target using lcov
    if(HAVE_LCOV AND GENHTML_PATH)
        add_custom_target(coverage-lcov
            # Capture coverage data
            COMMAND ${LCOV_PATH}
                --capture
                --directory ${CMAKE_BINARY_DIR}
                --output-file ${COVERAGE_OUTPUT_DIR}/coverage.info
            # Remove system headers
            COMMAND ${LCOV_PATH}
                --remove ${COVERAGE_OUTPUT_DIR}/coverage.info
                '/usr/*'
                '${CMAKE_CURRENT_SOURCE_DIR}/tests/*'
                '${CMAKE_CURRENT_SOURCE_DIR}/src/platform/freertos/kernel/*'
                --output-file ${COVERAGE_OUTPUT_DIR}/coverage.info
            # Generate HTML report
            COMMAND ${GENHTML_PATH}
                ${COVERAGE_OUTPUT_DIR}/coverage.info
                --output-directory ${COVERAGE_OUTPUT_DIR}/html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report with lcov"
        )
    endif()
    
    # Combined coverage target
    if(HAVE_GCOVR OR (HAVE_LCOV AND GENHTML_PATH))
        add_custom_target(coverage)
        if(HAVE_GCOVR)
            add_dependencies(coverage coverage-gcovr)
        endif()
        if(HAVE_LCOV AND GENHTML_PATH)
            add_dependencies(coverage coverage-lcov)
        endif()
    endif()
    
    # Coverage cleanup target
    add_custom_target(coverage-clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_OUTPUT_DIR}
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
        COMMENT "Cleaning coverage data"
    )
endif()

#==============================================================================
# Test Categories
#==============================================================================
# Unit tests
if(BUILD_TESTS)
    add_custom_target(test-unit
        COMMAND ${CMAKE_CTEST_COMMAND} -R unit --output-on-failure
        DEPENDS eth_dds
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running unit tests"
    )
    
    # Integration tests
    add_custom_target(test-integration
        COMMAND ${CMAKE_CTEST_COMMAND} -R integration --output-on-failure
        DEPENDS eth_dds
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running integration tests"
    )
    
    # System tests
    add_custom_target(test-system
        COMMAND ${CMAKE_CTEST_COMMAND} -R system --output-on-failure
        DEPENDS eth_dds
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running system tests"
    )
    
    # All tests
    add_custom_target(test-all
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS eth_dds
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running all tests"
    )
endif()

#==============================================================================
# Memory Check (Valgrind)
#==============================================================================
find_program(VALGRIND_PATH valgrind)
if(VALGRIND_PATH AND BUILD_TESTS)
    message(STATUS "Found valgrind - memory check available")
    
    add_custom_target(memcheck
        COMMAND ${VALGRIND_PATH}
            --tool=memcheck
            --leak-check=full
            --show-leak-kinds=all
            --track-origins=yes
            --error-exitcode=1
            $<TARGET_FILE:test_runner>
        DEPENDS test_runner
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running valgrind memory check"
    )
endif()

#==============================================================================
# Static Analysis Target
#==============================================================================
if(HAVE_CPPCHECK)
    add_custom_target(static-analysis
        COMMAND ${CPPCHECK_PATH}
            --enable=all
            --error-exitcode=1
            --suppress=missingIncludeSystem
            --xml
            --xml-version=2
            -I ${CMAKE_CURRENT_SOURCE_DIR}/src
            -I ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            2> ${CMAKE_BINARY_DIR}/cppcheck-report.xml
        COMMENT "Running static analysis with cppcheck"
    )
endif()

#==============================================================================
# Performance Benchmarks
#==============================================================================
if(BUILD_TESTS)
    add_custom_target(benchmark
        COMMAND ${CMAKE_CTEST_COMMAND} -R benchmark --output-on-failure
        DEPENDS eth_dds
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running performance benchmarks"
    )
endif()

#==============================================================================
# Test Summary Report
#==============================================================================
if(BUILD_TESTS)
    add_custom_target(test-report
        COMMAND ${CMAKE_COMMAND} -E echo "Test Summary Report"
        COMMAND ${CMAKE_COMMAND} -E echo "==================="
        COMMAND ${CMAKE_CTEST_COMMAND} -N
        DEPENDS test-all
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
