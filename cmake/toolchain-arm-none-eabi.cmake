# ARM GCC Toolchain for S32K312 (ARM Cortex-M7)
# Toolchain file for cross-compiling to ARM Cortex-M7

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Target triple
set(TARGET_TRIPLE "arm-none-eabi")

# Find ARM GCC toolchain
if(NOT DEFINED ENV{ARM_GCC_PATH})
    # Try to find in common locations
    find_path(ARM_GCC_BIN_PATH 
        NAMES arm-none-eabi-gcc
        PATHS
            /opt/gcc-arm-none-eabi/bin
            /usr/bin
            /usr/local/bin
            "C:/Program Files (x86)/GNU Tools ARM Embedded/bin"
            "C:/Program Files/GNU Tools ARM Embedded/bin"
        DOC "ARM GCC binary directory"
    )
else()
    set(ARM_GCC_BIN_PATH "$ENV{ARM_GCC_PATH}/bin")
endif()

# Set toolchain programs
set(CMAKE_C_COMPILER "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-gcc" CACHE FILEPATH "C Compiler")
set(CMAKE_CXX_COMPILER "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-g++" CACHE FILEPATH "C++ Compiler")
set(CMAKE_ASM_COMPILER "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-gcc" CACHE FILEPATH "ASM Compiler")
set(CMAKE_AR "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-ranlib" CACHE FILEPATH "Ranlib")
set(CMAKE_OBJCOPY "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-objcopy" CACHE FILEPATH "Objcopy")
set(CMAKE_OBJDUMP "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-objdump" CACHE FILEPATH "Objdump")
set(CMAKE_SIZE "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-size" CACHE FILEPATH "Size")
set(CMAKE_NM "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-nm" CACHE FILEPATH "NM")
set(CMAKE_STRIP "${ARM_GCC_BIN_PATH}/${TARGET_TRIPLE}-strip" CACHE FILEPATH "Strip")

# Don't run linker on compiler check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ARM Cortex-M7 specific flags for S32K312
set(ARM_CPU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")

# Common flags
set(CMAKE_C_FLAGS_INIT "${ARM_CPU_FLAGS} -fdata-sections -ffunction-sections -Wall -Wextra")
set(CMAKE_CXX_FLAGS_INIT "${ARM_CPU_FLAGS} -fdata-sections -ffunction-sections -Wall -Wextra -fno-rtti -fno-exceptions")
set(CMAKE_ASM_FLAGS_INIT "${ARM_CPU_FLAGS} -x assembler-with-cpp")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "${ARM_CPU_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage -specs=nano.specs -specs=nosys.specs")

# Set library search path to avoid host system libraries
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Print toolchain info
message(STATUS "ARM GCC Toolchain configured")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Target: ${TARGET_TRIPLE}")
message(STATUS "  CPU: Cortex-M7 (hard float)")
