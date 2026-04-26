# CMakeLists.txt for EthIf Module
# Ethernet Interface - ECUAL Layer

# Source files
set(ETHIF_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/EthIf.c
)

# Header directories
set(ETHIF_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/ecual/ethIf
    ${CMAKE_SOURCE_DIR}/src/common
)

# Create library
add_library(ethif STATIC ${ETHIF_SOURCES})

target_include_directories(ethif PUBLIC
    ${ETHIF_INCLUDE_DIRS}
)

# Compiler options for ASIL-B
target_compile_options(ethif PRIVATE
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
if(ETHIF_TARGET_AURIX_TC3XX)
    target_compile_definitions(ethif PRIVATE ETHIF_TARGET_AURIX_TC3XX)
elseif(ETHIF_TARGET_AURIX_TC4XX)
    target_compile_definitions(ethif PRIVATE ETHIF_TARGET_AURIX_TC4XX)
elseif(ETHIF_TARGET_S32K3XX)
    target_compile_definitions(ethif PRIVATE ETHIF_TARGET_S32K3XX)
elseif(ETHIF_TARGET_S32G3)
    target_compile_definitions(ethif PRIVATE ETHIF_TARGET_S32G3)
else()
    target_compile_definitions(ethif PRIVATE ETHIF_TARGET_EMULATOR)
endif()

# Enable features
target_compile_definitions(ethif PRIVATE
    ETHIF_DEV_ERROR_DETECT=STD_ON
    ETHIF_VERSION_INFO_API=STD_ON
    ETHIF_ENABLE_TX_BUFFERING=STD_ON
    ETHIF_ENABLE_RX_BUFFERING=STD_ON
    ETHIF_ENABLE_VLAN_PROCESSING=STD_ON
    ETHIF_ENABLE_TIMESTAMP=STD_ON
    STD_ON=1
    STD_OFF=0
)

# Link with common AUTOSAR types
target_link_libraries(ethif PUBLIC
    autosar_types
)

# Test executable
if(BUILD_TESTING)
    add_executable(test_ethif
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_ethif.c
    )
    
    target_link_libraries(test_ethif PRIVATE ethif)
    
    target_include_directories(test_ethif PRIVATE
        ${ETHIF_INCLUDE_DIRS}
    )
    
    # Add test
    add_test(NAME ethif_unit_tests COMMAND test_ethif)
endif()

# Installation
install(TARGETS ethif
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ecual/ethIf
    DESTINATION include/ecual
)
