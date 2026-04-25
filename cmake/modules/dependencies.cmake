# cmake/modules/dependencies.cmake
# External dependency management

#==============================================================================
# Threading Library
#==============================================================================
if(ENABLE_PLATFORM_LINUX)
    message(STATUS "Looking for threading library")
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
    message(STATUS "  Found: ${CMAKE_THREAD_LIBS_INIT}")
endif()

#==============================================================================
# Math Library
#==============================================================================
if(NOT ENABLE_PLATFORM_BAREMETAL)
    message(STATUS "Looking for math library")
    find_library(MATH_LIBRARY m)
    if(MATH_LIBRARY)
        message(STATUS "  Found: ${MATH_LIBRARY}")
    else()
        message(WARNING "  Math library not found")
    endif()
endif()

#==============================================================================
# Doxygen (for documentation)
#==============================================================================
if(BUILD_DOCUMENTATION)
    message(STATUS "Looking for Doxygen")
    find_package(Doxygen OPTIONAL_COMPONENTS dot)
    
    if(DOXYGEN_FOUND)
        message(STATUS "  Found: Doxygen ${DOXYGEN_VERSION}")
        if(DOXYGEN_DOT_FOUND)
            message(STATUS "  Found: Graphviz dot")
        endif()
        
        # Configure Doxyfile
        set(DOXYGEN_PROJECT_NAME "ETH-DDS Integration")
        set(DOXYGEN_PROJECT_VERSION ${PROJECT_VERSION})
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_XML YES)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_EXTRACT_PRIVATE NO)
        set(DOXYGEN_EXTRACT_STATIC YES)
        set(DOXYGEN_RECURSIVE YES)
        set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
        set(DOXYGEN_WARN_IF_DOC_ERROR YES)
        set(DOXYGEN_WARN_NO_PARAMDOC YES)
        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
        
        doxygen_add_docs(
            docs
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/docs
            COMMENT "Generating API documentation with Doxygen"
        )
    else()
        message(WARNING "  Doxygen not found - documentation will not be built")
        set(BUILD_DOCUMENTATION OFF)
    endif()
endif()

#==============================================================================
# GTest/GMock (optional, for C++ tests)
#==============================================================================
if(BUILD_TESTS)
    message(STATUS "Looking for GoogleTest")
    find_package(GTest QUIET)
    
    if(GTEST_FOUND)
        message(STATUS "  Found: GTest ${GTEST_VERSION}")
        set(HAVE_GTEST ON)
    else()
        message(STATUS "  Not found - using Unity for C tests")
        set(HAVE_GTEST OFF)
    endif()
endif()

#==============================================================================
# gcovr/lcov (for coverage)
#==============================================================================
if(ENABLE_COVERAGE)
    message(STATUS "Looking for coverage tools")
    
    find_program(GCOVR_PATH gcovr)
    if(GCOVR_PATH)
        message(STATUS "  Found: gcovr")
        set(HAVE_GCOVR ON)
    else()
        message(WARNING "  gcovr not found - XML coverage reports unavailable")
    endif()
    
    find_program(LCOV_PATH lcov)
    if(LCOV_PATH)
        message(STATUS "  Found: lcov")
        set(HAVE_LCOV ON)
    else()
        message(WARNING "  lcov not found - HTML coverage reports may be limited")
    endif()
    
    find_program(GENHTML_PATH genhtml)
    if(GENHTML_PATH)
        message(STATUS "  Found: genhtml")
    endif()
endif()

#==============================================================================
# Code Formatting Tools
#==============================================================================
find_program(CLANG_FORMAT_PATH clang-format)
if(CLANG_FORMAT_PATH)
    message(STATUS "Found: clang-format")
    set(HAVE_CLANG_FORMAT ON)
    
    # Add format target
    file(GLOB_RECURSE ALL_SOURCE_FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    )
    
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT_PATH} -i ${ALL_SOURCE_FILES}
        COMMENT "Running clang-format on all source files"
    )
    
    add_custom_target(
        format-check
        COMMAND ${CLANG_FORMAT_PATH} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "Checking code formatting"
    )
else()
    message(STATUS "clang-format not found - code formatting unavailable")
endif()

#==============================================================================
# Static Analysis Tools
#==============================================================================
if(ENABLE_STATIC_ANALYSIS)
    # cppcheck
    find_program(CPPCHECK_PATH cppcheck)
    if(CPPCHECK_PATH)
        message(STATUS "Found: cppcheck")
        set(HAVE_CPPCHECK ON)
        
        add_custom_target(
            cppcheck
            COMMAND ${CPPCHECK_PATH}
                --enable=all
                --error-exitcode=1
                --suppress=missingIncludeSystem
                -I ${CMAKE_CURRENT_SOURCE_DIR}/src
                -I ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}/src
            COMMENT "Running cppcheck static analysis"
        )
    else()
        message(WARNING "cppcheck not found")
    endif()
    
    # clang-tidy
    find_program(CLANG_TIDY_PATH clang-tidy)
    if(CLANG_TIDY_PATH)
        message(STATUS "Found: clang-tidy")
        set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY_PATH})
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PATH})
    else()
        message(WARNING "clang-tidy not found")
    endif()
endif()

#==============================================================================
# Package Generation Tools
#==============================================================================
if(BUILD_PACKAGE)
    # DEB/RPM package generation
    find_program(DPKG_DEB_PATH dpkg-deb)
    find_program(RPMBUILD_PATH rpmbuild)
    
    if(DPKG_DEB_PATH)
        message(STATUS "Found: dpkg-deb (DEB package generation available)")
    endif()
    
    if(RPMBUILD_PATH)
        message(STATUS "Found: rpmbuild (RPM package generation available)")
    endif()
endif()

#==============================================================================
# FreeRTOS Detection (when using external FreeRTOS)
#==============================================================================
if(ENABLE_PLATFORM_FREERTOS)
    # Check if FreeRTOS is available
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/freertos/kernel/include/FreeRTOS.h)
        message(STATUS "Found: FreeRTOS kernel in repository")
        set(FREERTOS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/freertos/kernel)
        set(FREERTOS_FOUND ON)
    else()
        message(FATAL_ERROR "FreeRTOS kernel not found. "
            "Ensure src/platform/freertos/kernel/ exists")
    endif()
endif()

#==============================================================================
# Python (for code generation tools)
#==============================================================================
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    message(STATUS "Found: Python ${Python3_VERSION}")
    set(HAVE_PYTHON ON)
    
    # Check for required Python packages
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import yaml; import jinja2"
        RESULT_VARIABLE PYTHON_DEPS_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )
    
    if(PYTHON_DEPS_RESULT EQUAL 0)
        message(STATUS "  Python dependencies: OK")
        set(HAVE_PYTHON_DEPS ON)
    else()
        message(WARNING "  Python yaml/jinja2 not found - code generation tools may fail")
        set(HAVE_PYTHON_DEPS OFF)
    endif()
else()
    message(WARNING "Python3 not found - code generation tools unavailable")
endif()

#==============================================================================
# Summary
#==============================================================================
message(STATUS "")
message(STATUS "Dependency Summary:")
message(STATUS "  Threads: ${CMAKE_THREAD_LIBS_INIT}")
if(MATH_LIBRARY)
    message(STATUS "  Math: ${MATH_LIBRARY}")
endif()
if(DOXYGEN_FOUND)
    message(STATUS "  Doxygen: Yes")
else()
    message(STATUS "  Doxygen: No")
endif()
if(GTEST_FOUND)
    message(STATUS "  GTest: Yes")
else()
    message(STATUS "  GTest: No")
endif()
if(HAVE_CLANG_FORMAT)
    message(STATUS "  clang-format: Yes")
else()
    message(STATUS "  clang-format: No")
endif()
if(HAVE_CPPCHECK)
    message(STATUS "  cppcheck: Yes")
else()
    message(STATUS "  cppcheck: No")
endif()
if(Python3_FOUND)
    message(STATUS "  Python: ${Python3_VERSION}")
else()
    message(STATUS "  Python: No")
endif()
message(STATUS "")
