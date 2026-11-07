# ==============================================================================
# SanitizerFlags.cmake - Address / Undefined Behavior Sanitizer configuration
# ==============================================================================
# Usage (native host builds only, ignored for cross-compilation):
#   cmake -DYULE_ENABLE_ASAN=ON  ..   # AddressSanitizer (heap/stack/global bounds, UAF)
#   cmake -DYULE_ENABLE_UBSAN=ON ..   # UndefinedBehaviorSanitizer (shift/overflow/alignment)
#   both options may be combined.
#
# Flags are applied to ALL targets (BSW static libraries + tests), so an
# overflow inside a library is instrumented just like one inside a test.
# ==============================================================================

option(YULE_ENABLE_ASAN  "Enable AddressSanitizer for native builds" OFF)
option(YULE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for native builds" OFF)

if(YULE_ENABLE_ASAN OR YULE_ENABLE_UBSAN)
    if(CMAKE_CROSSCOMPILING)
        message(WARNING
            "YULE_ENABLE_ASAN/YULE_ENABLE_UBSAN ignored: sanitizers are only "
            "supported for native host builds (cross toolchain has no ASan runtime)")
        set(YULE_ENABLE_ASAN OFF)
        set(YULE_ENABLE_UBSAN OFF)
    elseif(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(WARNING
            "YULE_ENABLE_ASAN/YULE_ENABLE_UBSAN ignored: unsupported compiler "
            "'${CMAKE_C_COMPILER_ID}' (GCC/Clang required)")
        set(YULE_ENABLE_ASAN OFF)
        set(YULE_ENABLE_UBSAN OFF)
    endif()
endif()

if(YULE_ENABLE_ASAN OR YULE_ENABLE_UBSAN)
    set(_san_list "")
    set(_san_desc "")

    if(YULE_ENABLE_ASAN)
        list(APPEND _san_list "address")
        string(APPEND _san_desc "Address")
    endif()
    if(YULE_ENABLE_UBSAN)
        # -fsanitize=undefined covers array-bounds, shift, signed overflow,
        # alignment, null-deref etc. (portable across GCC and Clang).
        # -fno-sanitize-recover turns a finding into a hard abort so the ctest
        # exit code fails instead of printing and continuing.
        list(APPEND _san_list "undefined")
        string(APPEND _san_desc " UndefinedBehavior")
    endif()

    string(REPLACE ";" "," _san_flags "-fsanitize=${_san_list}")

    set(_san_compile_flags
        "${_san_flags}"
        "-fno-omit-frame-pointer"
        "-fno-common"
        "-g")
    if(YULE_ENABLE_UBSAN)
        list(APPEND _san_compile_flags "-fno-sanitize-recover=undefined")
        # GCC emits -Wconversion for compound assignments (e.g. `u8 |= x << i`)
        # only when UBSan instruments them, which would break -Werror builds
        # for pre-existing, non-UB code. Downgrade the diagnostic (do not
        # disable it) so sanitizer runs still surface the warning without
        # failing the build; the regular build keeps -Werror=conversion.
        list(APPEND _san_compile_flags "-Wno-error=conversion")
    endif()

    add_compile_options(${_san_compile_flags})
    add_link_options(${_san_flags})
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${_san_flags}")

    message(STATUS "Sanitizer:${_san_desc}Sanitizer enabled (${_san_flags})")
endif()
