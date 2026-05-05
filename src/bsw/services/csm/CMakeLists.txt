# CSM (Crypto Services Manager) Module CMakeLists.txt
# YuleTech AutoSAR BSW Platform

cmake_minimum_required(VERSION 3.13)

# CSM Library
add_library(csm STATIC
    src/Csm.c
)

# Include directories
target_include_directories(csm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/bsw/common
    ${CMAKE_SOURCE_DIR}/src/bsw/mcal/include
)

# Compiler definitions
target_compile_definitions(csm PRIVATE
    CSM_CFG_DEV_ERROR_DETECT=STD_ON
    CSM_CFG_VERSION_INFO_API=STD_ON
    CSM_CFG_ASYNC_SUPPORT=STD_ON
    CSM_CFG_QUEUE_SUPPORT=STD_ON
)

# Compiler options
target_compile_options(csm PRIVATE
    -Wall
    -Wextra
    -Werror=implicit-function-declaration
    -O2
)

# Link dependencies
target_link_libraries(csm PUBLIC
    bsw_common
)

# Export include directories for parent
set(CSM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

# Installation
install(TARGETS csm DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/csm)
