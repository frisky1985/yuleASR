# WdgM Module CMakeLists.txt
# Watchdog Manager Service Layer

set(WDGM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/wdgM.c
    ${CMAKE_CURRENT_SOURCE_DIR}/wdgIf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/wdgM_BswM.c
    ${CMAKE_CURRENT_SOURCE_DIR}/wdgM_Lcfg.c
)

set(WDGM_HEADERS
    ${PROJECT_SOURCE_DIR}/include/autosar/service/wdgM/wdgM.h
    ${PROJECT_SOURCE_DIR}/include/autosar/service/wdgM/wdgM_Cfg.h
    ${PROJECT_SOURCE_DIR}/include/autosar/service/wdgM/wdgIf.h
    ${PROJECT_SOURCE_DIR}/include/autosar/service/wdgM/wdgM_BswM.h
)

# Create static library
add_library(wdgm STATIC ${WDGM_SOURCES})

# Set include directories
target_include_directories(wdgm
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/autosar/service/wdgM
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set compile options for ASIL-D compliance
target_compile_options(wdgm
    PRIVATE
        -std=c11
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -O2
        -fomit-frame-pointer
        -ffunction-sections
        -fdata-sections
)

# Add definitions
target_compile_definitions(wdgm
    PRIVATE
        WDGM_DEV_ERROR_DETECT=1
        WDGM_VERSION_INFO_API=1
)

# Set properties
set_target_properties(wdgm PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "wdgm"
)

# Install headers
install(FILES ${WDGM_HEADERS}
    DESTINATION include/autosar/service/wdgM
)

# Install library
install(TARGETS wdgm
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
