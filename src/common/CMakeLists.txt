# src/common/CMakeLists.txt
# Common utilities and types

set(COMMON_SOURCES
    utils/eth_utils.c
    log/dds_log.c
)

set(COMMON_HEADERS
    types/eth_types.h
    utils/eth_utils.h
    log/dds_log.h
    autosar_types.h
    autosar_common.h
    autosar_errors.h
)

# Create common library
add_library(eth_dds_common STATIC
    ${COMMON_SOURCES}
)

target_include_directories(eth_dds_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/types
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/log
)

target_link_libraries(eth_dds_common PUBLIC
    $<$<BOOL:${ENABLE_PLATFORM_LINUX}>:Threads::Threads>
    $<$<NOT:$<BOOL:${ENABLE_PLATFORM_BAREMETAL}>>:${MATH_LIBRARY}>
)

# Set compiler options
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_dds_common PRIVATE
        -Wall
        -Wextra
    )
endif()
