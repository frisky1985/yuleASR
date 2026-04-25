# src/transport/CMakeLists.txt
# Transport layer abstraction

set(TRANSPORT_SOURCES
    transport_interface.c
    transport_manager.c
    udp_transport.c
    shm_transport.c
    eth_transport.c
)

set(TRANSPORT_HEADERS
    transport_interface.h
    transport_manager.h
    udp_transport.h
    shm_transport.h
    eth_transport.h
)

# Create transport library
add_library(eth_dds_transport STATIC
    ${TRANSPORT_SOURCES}
)

target_include_directories(eth_dds_transport PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(eth_dds_transport PUBLIC
    eth_dds_common
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_dds_transport PRIVATE
        -Wall
        -Wextra
    )
endif()

# Install headers
install(FILES ${TRANSPORT_HEADERS}
    DESTINATION include/eth_dds/transport
    COMPONENT headers
)
