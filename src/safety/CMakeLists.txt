# src/safety/CMakeLists.txt
# Safety modules (ASIL-D compliant)

# RAM ECC module
if(ENABLE_SAFETY_RAM)
    add_subdirectory(ram)
endif()

# NVM module
if(ENABLE_SAFETY_NVM)
    add_subdirectory(nvm)
endif()

# SafeRAM module
if(ENABLE_SAFETY_SAFERAM)
    add_subdirectory(saferam)
endif()
