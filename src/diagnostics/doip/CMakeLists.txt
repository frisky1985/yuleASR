# DoIP Module CMakeLists.txt
# ISO 13400-2 Diagnostic over IP Implementation

cmake_minimum_required(VERSION 3.16)

# DoIP Library
add_library(doip STATIC
    doip_types.h
    doip_core.h
    doip_core.c
)

target_include_directories(doip
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

target_compile_options(doip PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O2
    -g
)

# Set C standard
target_compile_features(doip PUBLIC c_std_11)

# Link with common library
target_link_libraries(doip
    PUBLIC
        common
)

# Install headers
install(FILES
    doip_types.h
    doip_core.h
    DESTINATION include/diagnostics/doip
)

# Install library
install(TARGETS doip
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
