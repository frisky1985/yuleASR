# DocsGeneration.cmake
# Documentation generation support

# Include guard
if(__YULETECH_DOCS_GENERATION__)
    return()
endif()
set(__YULETECH_DOCS_GENERATION__ TRUE)

# Find documentation tools
find_package(Doxygen)
find_program(SPHINX_EXECUTABLE sphinx-build)

# Doxygen configuration
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/docs/doxygen)
    set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
    
    # Default Doxyfile settings
    set(DOXYGEN_PROJECT_NAME "${PROJECT_NAME}")
    set(DOXYGEN_PROJECT_VERSION "${PROJECT_VERSION}")
    set(DOXYGEN_INPUT_DIRS "${CMAKE_SOURCE_DIR}/src")
    set(DOXYGEN_OUTPUT_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
    
    # Generate Doxyfile
    file(WRITE ${CMAKE_BINARY_DIR}/Doxyfile "
PROJECT_NAME = \"${DOXYGEN_PROJECT_NAME}\"
PROJECT_NUMBER = \"${DOXYGEN_PROJECT_VERSION}\"
INPUT = ${DOXYGEN_INPUT_DIRS}
OUTPUT_DIRECTORY = ${DOXYGEN_OUTPUT_DIRECTORY}
RECURSIVE = YES
GENERATE_HTML = YES
GENERATE_LATEX = NO
EXTRACT_ALL = YES
EXTRACT_PRIVATE = NO
EXTRACT_STATIC = YES
CALL_GRAPH = YES
CALLER_GRAPH = YES
HAVE_DOT = NO
OPTIMIZE_OUTPUT_FOR_C = YES
TYPEDEF_HIDES_STRUCT = YES
FILE_PATTERNS = *.c *.h
")

    add_custom_target(docs-doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# Sphinx configuration
if(SPHINX_EXECUTABLE)
    set(SPHINX_SOURCE ${CMAKE_SOURCE_DIR}/docs/sphinx)
    set(SPHINX_BUILD ${CMAKE_BINARY_DIR}/docs/sphinx)
    
    add_custom_target(docs-sphinx
        COMMAND ${SPHINX_EXECUTABLE} -b html ${SPHINX_SOURCE} ${SPHINX_BUILD}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating documentation with Sphinx"
        VERBATIM
    )
endif()

# Combined docs target
if(DOXYGEN_FOUND OR SPHINX_EXECUTABLE)
    add_custom_target(docs)
    if(DOXYGEN_FOUND)
        add_dependencies(docs docs-doxygen)
    endif()
    if(SPHINX_EXECUTABLE)
        add_dependencies(docs docs-sphinx)
    endif()
endif()
