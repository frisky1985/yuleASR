# ==============================================================================
# SanitizerFlags.cmake - Address/Undefined Behavior Sanitizer configuration
# ==============================================================================
# Usage (native host builds only):
#   cmake -DYULE_ENABLE_ASAN=ON ..   # AddressSanitizer (array/heap bounds, UAF)
#
# ASan applies to ALL targets (BSW sources + tests) so cross-boundary
# overflows inside static libraries are also instrumented.
# ==============================================================================

option(YULE_ENABLE_ASAN "Enable AddressSanitizer for native builds" OFF)

if(YULE_ENABLE_ASAN)
    if(CMAKE_CROSSCOMPILING)
        message(WARNING "YULE_ENABLE_ASAN ignored: sanitizer only supported for native host builds")
    else()
        set(ENABLE_SANITIZERS ON CACHE BOOL "Enable sanitizers (set by YULE_ENABLE_ASAN)" FORCE)
    endif()
endif()

if(ENABLE_SANITIZERS AND (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang"))
    set(_SAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer -fno-common")
    add_compile_options(${_SAN_FLAGS})
    add_link_options(-fsanitize=address)
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -fsanitize=address")
    message(STATUS "Sanitizer: AddressSanitizer enabled")
endif()
