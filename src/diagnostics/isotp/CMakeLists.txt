# IsoTp Module CMakeLists.txt
# ISO 15765-2 Transport Layer Implementation

cmake_minimum_required(VERSION 3.16)

# IsoTp Library
add_library(isotp STATIC
    isotp_types.h
    isotp_core.h
    isotp_core.c
    isotp_canif.h
    isotp_canif.c
    pdur_interface.h
    pdur_interface.c
)

target_include_directories(isotp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

target_compile_options(isotp PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O2
    -g
)

# Set C standard
target_compile_features(isotp PUBLIC c_std_11)

# Link with common library
target_link_libraries(isotp
    PUBLIC
        common
)

# Install headers
install(FILES
    isotp_types.h
    isotp_core.h
    isotp_canif.h
    pdur_interface.h
    DESTINATION include/diagnostics/isotp
)

# Install library
install(TARGETS isotp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
