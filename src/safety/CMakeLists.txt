# src/safety/CMakeLists.txt
# Safety modules (ASIL-D compliant)
#
# Sub-module switches previously had no default value anywhere in the tree,
# so every sub-module was silently skipped (11 .c files, ~10k LOC never
# compiled). Define defaults here; YULE_ENABLE_SAFETY (PlatformConfig.cmake,
# default ON) gates the whole src/safety subtree from the top-level build.

# RAM ECC module (SECDED)
option(ENABLE_SAFETY_RAM "Build safety RAM ECC module" ON)

# NVM module — no nvm/ sub-directory exists in-tree (only ram/ + saferam/);
# kept OFF so add_subdirectory(nvm) is not attempted on a missing directory.
option(ENABLE_SAFETY_NVM "Build safety NVM module" OFF)

# SafeRAM module (memory protection)
option(ENABLE_SAFETY_SAFERAM "Build safety SafeRAM module" ON)

if(ENABLE_SAFETY_RAM)
    add_subdirectory(ram)
endif()

if(ENABLE_SAFETY_NVM AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/nvm)
    add_subdirectory(nvm)
endif()

if(ENABLE_SAFETY_SAFERAM)
    add_subdirectory(saferam)
endif()
