# CMakeLists.txt for WdgIf Module
# Watchdog Interface - ECUAL Layer

# Source files
set(WDGIF_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/WdgIf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/WdgIf_Hw_Aurix.c
    ${CMAKE_CURRENT_SOURCE_DIR}/WdgIf_Hw_S32K3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/WdgIf_Hw_Emulator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/WdgIf_Gpt.c
)

# Header directories
set(WDGIF_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/ecual/wdgIf
    ${CMAKE_SOURCE_DIR}/src/common
)

# Create library
add_library(wdgif STATIC ${WDGIF_SOURCES})

target_include_directories(wdgif PUBLIC
    ${WDGIF_INCLUDE_DIRS}
)

# Compiler options for safety (ASIL-D)
target_compile_options(wdgif PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wcast-align
    -Wshadow
    -Wundef
    -Os
    -ffunction-sections
    -fdata-sections
)

# Target specific definitions
if(WDGIF_TARGET_AURIX_TC3XX)
    target_compile_definitions(wdgif PRIVATE WDGIF_TARGET_AURIX_TC3XX)
elseif(WDGIF_TARGET_AURIX_TC4XX)
    target_compile_definitions(wdgif PRIVATE WDGIF_TARGET_AURIX_TC4XX)
elseif(WDGIF_TARGET_S32K3XX)
    target_compile_definitions(wdgif PRIVATE WDGIF_TARGET_S32K3XX)
elseif(WDGIF_TARGET_S32G3)
    target_compile_definitions(wdgif PRIVATE WDGIF_TARGET_S32G3)
else()
    target_compile_definitions(wdgif PRIVATE WDGIF_TARGET_EMULATOR)
endif()

# Enable safety features
target_compile_definitions(wdgif PRIVATE
    WDGIF_DEV_ERROR_DETECT=STD_ON
    WDGIF_VERSION_INFO_API=STD_ON
    WDGIF_EXTERNAL_WDG_SUPPORTED
)

# Test executable
if(BUILD_TESTING)
    add_executable(test_wdgif
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_wdgif.c
    )
    
    target_link_libraries(test_wdgif PRIVATE wdgif)
    
    target_include_directories(test_wdgif PRIVATE
        ${WDGIF_INCLUDE_DIRS}
    )
    
    # Add test
    add_test(NAME wdgif_unit_tests COMMAND test_wdgif)
endif()

# Installation
install(TARGETS wdgif
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ecual/wdgIf
    DESTINATION include/ecual
)
