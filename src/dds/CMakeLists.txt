# src/dds/CMakeLists.txt
# DDS middleware components

set(DDS_CORE_SOURCES
    core/dds_domain.c
    core/dds_entity.c
    core/dds_qos.c
    rtps/rtps_message.c
    rtps/rtps_discovery.c
    rtps/rtps_state.c
    rtps/rtps_wire.c
    pubsub/dds_publisher.c
    pubsub/dds_subscriber.c
    pubsub/dds_topic.c
    pubsub/dds_writer.c
    pubsub/dds_reader.c
)

set(DDS_CORE_HEADERS
    core/dds_core.h
    core/dds_domain.h
    core/dds_entity.h
    core/dds_qos.h
    rtps/rtps_message.h
    rtps/rtps_discovery.h
    rtps/rtps_state.h
    rtps/rtps_wire.h
    pubsub/dds_publisher.h
    pubsub/dds_subscriber.h
    pubsub/dds_topic.h
    pubsub/dds_writer.h
    pubsub/dds_reader.h
    pubsub/dds_pubsub.h
)

# DDS Security
if(ENABLE_DDS_SECURITY)
    list(APPEND DDS_CORE_SOURCES
        security/dds_security_manager.c
        security/dds_auth.c
        security/dds_crypto.c
        security/dds_access.c
    )
    list(APPEND DDS_CORE_HEADERS
        security/dds_security_manager.h
        security/dds_security_types.h
        security/dds_auth.h
        security/dds_crypto.h
        security/dds_access.h
    )
endif()

# DDS Runtime monitoring
if(ENABLE_DDS_RUNTIME)
    list(APPEND DDS_CORE_SOURCES
        runtime/dds_runtime.c
    )
    list(APPEND DDS_CORE_HEADERS
        runtime/dds_runtime.h
    )
endif()

# Create DDS library
add_library(eth_dds_middleware STATIC
    ${DDS_CORE_SOURCES}
)

target_include_directories(eth_dds_middleware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/rtps
    ${CMAKE_CURRENT_SOURCE_DIR}/pubsub
    ${CMAKE_CURRENT_SOURCE_DIR}/security
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)

target_link_libraries(eth_dds_middleware PUBLIC
    eth_dds_common
    eth_dds_transport
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(eth_dds_middleware PRIVATE
        -Wall
        -Wextra
    )
endif()

# Install headers
install(FILES ${DDS_CORE_HEADERS}
    DESTINATION include/eth_dds/dds
    COMPONENT headers
)
