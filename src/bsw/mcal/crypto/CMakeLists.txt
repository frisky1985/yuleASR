# Crypto (MCAL) module-level CMakeLists
# ---------------------------------------------------------------------------
# The production library target `mcal_crypto` is created by
# src/bsw/mcal/CMakeLists.txt (file(GLOB) over <module>/src/*.c).
# This file only mounts the module's host unit tests so that they can live
# next to the driver sources instead of under tests/unit/.
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
