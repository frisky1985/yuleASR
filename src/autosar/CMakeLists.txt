# AUTOSAR RTE Integration Module CMakeLists.txt
# AUTOSAR R22-11 compliant - ASIL-D Safety Level

cmake_minimum_required(VERSION 3.10)

# AUTOSAR Adaptive ara::com DDS Binding
set(ADAPTIVE_SOURCES
    adaptive/ara_com_dds.c
)

set(ADAPTIVE_HEADERS
    adaptive/ara_com_dds.h
)

# AUTOSAR Classic RTE DDS Integration
set(CLASSIC_SOURCES
    classic/rte_dds.c
)

set(CLASSIC_HEADERS
    classic/rte_dds.h
)

# E2E Protection
set(E2E_SOURCES
    e2e/e2e_protection.c
    e2e/e2e_state_machine.c
    e2e/e2e_dds_integration.c
)

set(E2E_HEADERS
    e2e/e2e_protection.h
    e2e/e2e_state_machine.h
    e2e/e2e_dds_integration.h
)

# ARXML Parser
set(ARXML_SOURCES
    arxml/arxml_parser.c
)

set(ARXML_HEADERS
    arxml/arxml_parser.h
)

# Service Layer Modules (WdgM, etc.)
set(SERVICE_SOURCES
    service/wdgM/wdgM.c
    service/wdgM/wdgIf.c
    service/wdgM/wdgM_BswM.c
    service/wdgM/wdgM_Lcfg.c
)

set(SERVICE_HEADERS
    service/wdgM/wdgM.h
    service/wdgM/wdgM_Cfg.h
    service/wdgM/wdgIf.h
    service/wdgM/wdgM_BswM.h
)

# Create AUTOSAR library
add_library(autosar_rte STATIC
    ${ADAPTIVE_SOURCES}
    ${CLASSIC_SOURCES}
    ${E2E_SOURCES}
    ${ARXML_SOURCES}
    ${SERVICE_SOURCES}
)

# Set include directories
target_include_directories(autosar_rte
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../common
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/adaptive
        ${CMAKE_CURRENT_SOURCE_DIR}/classic
        ${CMAKE_CURRENT_SOURCE_DIR}/e2e
        ${CMAKE_CURRENT_SOURCE_DIR}/arxml
        ${CMAKE_CURRENT_SOURCE_DIR}/service
        ${CMAKE_CURRENT_SOURCE_DIR}/service/wdgM
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/autosar/service/wdgM
)

# Compiler flags for ASIL-D compliance
target_compile_options(autosar_rte PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -Wundef
    -Wpointer-arith
    -Wcast-align
    -Wstrict-overflow=5
    -Wwrite-strings
    -Waggregate-return
    -Wcast-qual
    -Wswitch-default
    -Wswitch-enum
    -Wunreachable-code
    -Wconversion
    -Wformat=2
    -Wformat-security
    -Wmissing-include-dirs
    -Winit-self
    -Wmissing-prototypes
    -Wstrict-prototypes
    -Wold-style-definition
    -Wmissing-declarations
    -O2
    -fdata-sections
    -ffunction-sections
    -fno-strict-aliasing
)

# Safety-related definitions
target_compile_definitions(autosar_rte PRIVATE
    AUTOSAR_R22_11=1
    ASIL_D_COMPLIANT=1
    E2E_PROTECTION_ENABLED=1
    RTE_DDS_ENABLED=1
    ARA_COM_DDS_ENABLED=1
)

# Link with DDS runtime if available
if(TARGET dds_runtime)
    target_link_libraries(autosar_rte PUBLIC dds_runtime)
endif()

# Install targets
install(TARGETS autosar_rte
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES
    ${ADAPTIVE_HEADERS}
    ${CLASSIC_HEADERS}
    ${E2E_HEADERS}
    ${ARXML_HEADERS}
    DESTINATION include/autosar
)

install(FILES
    ${SERVICE_HEADERS}
    DESTINATION include/autosar/service/wdgM
)

# Summary message
message(STATUS "AUTOSAR RTE Module configured:")
message(STATUS "  - Adaptive ara::com DDS: ENABLED")
message(STATUS "  - Classic RTE DDS: ENABLED")
message(STATUS "  - E2E Protection: ENABLED")
message(STATUS "  - E2E State Machine: ENABLED")
message(STATUS "  - E2E DDS Integration: ENABLED")
message(STATUS "  - E2E Profiles: P01, P02, P04, P05, P06, P07, P11, P22")
message(STATUS "  - ARXML Parser: ENABLED")
message(STATUS "  - Service Layer (WdgM): ENABLED")
message(STATUS "  - ASIL-D Compliance: YES")
message(STATUS "  - AUTOSAR Version: R22-11")