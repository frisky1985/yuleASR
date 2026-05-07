# BSW Services Layer CMakeLists.txt
# Services: Com, PduR, NvM, Dcm, Dem, Dlt, DoCan, DoIp, WdgM, Lockstep, RamSafety, Csm

cmake_minimum_required(VERSION 3.16)

# Common include directories for services
set(SERVICES_COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/com/include
    ${CMAKE_CURRENT_SOURCE_DIR}/pdur/include
    ${CMAKE_CURRENT_SOURCE_DIR}/nvm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dcm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dem/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dlt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/docan/include
    ${CMAKE_CURRENT_SOURCE_DIR}/doip/include
    ${CMAKE_CURRENT_SOURCE_DIR}/wdgm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lockstep/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ramsafety/include
    ${CMAKE_CURRENT_SOURCE_DIR}/csm/include
)

#======================
# Communication (Com)
#======================
file(GLOB COM_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/com/src/*.c
)
add_library(Com STATIC ${COM_SOURCES})
target_include_directories(Com
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/com/include
    PRIVATE
        ${SERVICES_COMMON_INCLUDES}
)
target_link_libraries(Com PUBLIC PduR)
set_target_properties(Com PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# PDU Router (PduR)
#======================
file(GLOB PDUR_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/pdur/src/*.c
)
add_library(PduR STATIC ${PDUR_SOURCES})
target_include_directories(PduR
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/pdur/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/canif/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/cantp/include
)
set_target_properties(PduR PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# NVRAM Manager (NvM)
#======================
file(GLOB NVM_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/nvm/src/*.c
)
add_library(NvM STATIC ${NVM_SOURCES})
target_include_directories(NvM
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/nvm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/fee/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/memif/include
)
set_target_properties(NvM PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# Diagnostic Communication Manager (Dcm)
#======================
file(GLOB DCM_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/dcm/src/*.c
)
add_library(Dcm STATIC ${DCM_SOURCES})
target_include_directories(Dcm
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/dcm/include
    PRIVATE
        ${SERVICES_COMMON_INCLUDES}
)
target_link_libraries(Dcm PUBLIC Dem PduR)
set_target_properties(Dcm PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# Diagnostic Event Manager (Dem)
#======================
file(GLOB DEM_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/dem/src/*.c
)
add_library(Dem STATIC ${DEM_SOURCES})
target_include_directories(Dem
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/dem/include
)
set_target_properties(Dem PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# Diagnostic Log and Trace (Dlt)
#======================
file(GLOB DLT_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/dlt/src/*.c
)
add_library(Dlt STATIC ${DLT_SOURCES})
target_include_directories(Dlt
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/dlt/include
)
set_target_properties(Dlt PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# CAN Diagnostic Transport (DoCan)
#======================
file(GLOB DOCAN_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/docan/src/*.c
)
add_library(DoCan STATIC ${DOCAN_SOURCES})
target_include_directories(DoCan
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/docan/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/cantp/include
)
set_target_properties(DoCan PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# IP Diagnostic Transport (DoIp)
#======================
file(GLOB DOIP_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/doip/src/*.c
)
add_library(DoIp STATIC ${DOIP_SOURCES})
target_include_directories(DoIp
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/doip/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../ecual/ethif/include
)
set_target_properties(DoIp PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# Watchdog Manager (WdgM)
#======================
file(GLOB WDGM_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/wdgm/src/*.c
)
add_library(WdgM STATIC ${WDGM_SOURCES})
target_include_directories(WdgM
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/wdgm/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../mcal/wdg/include
)
set_target_properties(WdgM PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# Lockstep Monitor
#======================
if(YULE_ENABLE_SAFETY)
    file(GLOB LOCKSTEP_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/lockstep/src/*.c
    )
    add_library(Lockstep STATIC ${LOCKSTEP_SOURCES})
    target_include_directories(Lockstep
        PUBLIC 
            ${CMAKE_CURRENT_SOURCE_DIR}/lockstep/include
    )
    set_target_properties(Lockstep PROPERTIES
        C_STANDARD 99
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
    )
endif()

#======================
# RAM Safety
#======================
if(YULE_ENABLE_SAFETY)
    file(GLOB RAMSAFETY_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/ramsafety/src/*.c
    )
    add_library(RamSafety STATIC ${RAMSAFETY_SOURCES})
    target_include_directories(RamSafety
        PUBLIC 
            ${CMAKE_CURRENT_SOURCE_DIR}/ramsafety/include
    )
    set_target_properties(RamSafety PROPERTIES
        C_STANDARD 99
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
    )
endif()

#======================
# Crypto Services Manager (Csm)
#======================
file(GLOB CSM_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/csm/src/*.c
)
add_library(Csm STATIC ${CSM_SOURCES})
target_include_directories(Csm
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/csm/include
        ${CMAKE_SOURCE_DIR}/src/bsw/common
)
set_target_properties(Csm PROPERTIES
    C_STANDARD 99
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/bsw/services
)

#======================
# Services Meta-Target
#======================
add_library(BswServices INTERFACE)
target_link_libraries(BswServices INTERFACE
    Com
    PduR
    NvM
    Dcm
    Dem
    Dlt
    DoCan
    DoIp
    WdgM
    Csm
)
if(YULE_ENABLE_SAFETY)
    target_link_libraries(BswServices INTERFACE Lockstep RamSafety)
endif()

message(STATUS "BSW Services configured")
