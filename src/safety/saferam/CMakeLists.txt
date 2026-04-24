# SafeRAM Module CMakeLists.txt
# AUTOSAR R22-11 compliant, ISO 26262 ASIL-D

# SafeRAM source files
set(SAFERAM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ram_partition.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stack_protection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_protection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/safe_data.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fault_injection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/saferam_manager.c
)

# SafeRAM header files
set(SAFERAM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/ram_partition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stack_protection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/heap_protection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/safe_data.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fault_injection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/saferam_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/saferam.h
)

# Create static library
add_library(saferam STATIC ${SAFERAM_SOURCES})

# Include directories
target_include_directories(saferam
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

# Compile options for safety-critical code
target_compile_options(saferam
    PRIVATE
        -Wall
        -Wextra
        -Werror
        -O2
        -fno-omit-frame-pointer
        -fstack-protector-strong
        -Wstack-protector
        --param ssp-buffer-size=4
)

# ASIL-D specific defines
target_compile_definitions(saferam
    PUBLIC
        SAFERAM_ASIL_D=1
        SAFERAM_ENABLE_ASSERTS=1
)

# Link options
target_link_options(saferam
    PRIVATE
        -Wl,--no-undefined
)

# Install targets
install(TARGETS saferam
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${SAFERAM_HEADERS}
    DESTINATION include/safety/saferam
)
