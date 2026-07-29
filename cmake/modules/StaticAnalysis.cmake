# StaticAnalysis.cmake
# Static analysis tools integration

# Include guard
if(__YULETECH_STATIC_ANALYSIS__)
    return()
endif()
set(__YULETECH_STATIC_ANALYSIS__ TRUE)

# Find analysis tools
find_program(CPPCHECK_EXECUTABLE cppcheck)
find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
find_program(PC_LINT_EXECUTABLE pclp64)

# CppCheck
if(CPPCHECK_EXECUTABLE)
    set(CPPCHECK_ARGS
        --enable=all
        --std=c99
        --platform=unix32
        --suppress=missingIncludeSystem
        --template='{file}:{line}:{column}: {severity}: {message} [{id}]'
        --quiet
    )
    
    add_custom_target(analysis-cppcheck
        COMMAND ${CPPCHECK_EXECUTABLE} ${CPPCHECK_ARGS}
            ${CMAKE_SOURCE_DIR}/src
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running CppCheck static analysis"
        VERBATIM
    )
endif()

# Clang-Tidy
if(CLANG_TIDY_EXECUTABLE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.c
    )
    
    add_custom_target(analysis-clang-tidy
        COMMAND ${CLANG_TIDY_EXECUTABLE}
            -p ${CMAKE_BINARY_DIR}
            ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running Clang-Tidy static analysis"
        VERBATIM
    )
endif()

# PC-Lint (commercial)
if(PC_LINT_EXECUTABLE)
    add_custom_target(analysis-pclint
        COMMAND ${PC_LINT_EXECUTABLE}
            -i${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/bsw/**/*.c
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running PC-Lint static analysis"
        VERBATIM
    )
endif()

# Combined analysis target
if(CPPCHECK_EXECUTABLE OR CLANG_TIDY_EXECUTABLE OR PC_LINT_EXECUTABLE)
    add_custom_target(analysis)
    if(CPPCHECK_EXECUTABLE)
        add_dependencies(analysis analysis-cppcheck)
    endif()
    if(CLANG_TIDY_EXECUTABLE)
        add_dependencies(analysis analysis-clang-tidy)
    endif()
    if(PC_LINT_EXECUTABLE)
        add_dependencies(analysis analysis-pclint)
    endif()
endif()
