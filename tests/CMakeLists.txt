# Tests CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

# Find test framework (Unity)
find_package(Unity QUIET)

if(NOT UNITY_FOUND)
    # Use bundled Unity (tests/unit/framework/unity.c) — 系统无 Unity 包时
    # 编译 bundled 实现为 Unity 库, 保证依赖 Unity 断言的测试可链接
    message(STATUS "Unity not found, using bundled Unity framework")
    add_library(Unity STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/unit/framework/unity.c
    )
    target_include_directories(Unity PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/unit/framework
    )
    set(TEST_FRAMEWORK_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/unit/framework)
    set(TEST_FRAMEWORK_LIB Unity)
else()
    # 系统 Unity 包 (ThrowTheSwitch) 提供 imported target Unity::unity
    set(TEST_FRAMEWORK_INCLUDE ${UNITY_INCLUDE_DIRS})
    set(TEST_FRAMEWORK_LIB Unity::unity)
endif()

# Enable testing
enable_testing()

# Unit tests
add_subdirectory(unit)

# Integration tests
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration)
    add_subdirectory(integration)
endif()

# MCAL mock tests (links real production source with MockHAL)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mock/CMakeLists_MCAL_Tests.txt)
    include(${CMAKE_CURRENT_SOURCE_DIR}/mock/CMakeLists_MCAL_Tests.txt)
endif()

# ── S1: S0 smoke test — native runtime verification (FreeRTOS Posix port) ──
# Only on native (non-cross) builds: the Posix port runs on Linux/macOS compile
# but is only *supported* at runtime on Linux (see sprint-contract-next.md).
if(NOT CMAKE_CROSSCOMPILING)
    add_executable(s0_smoke_test
        ${CMAKE_CURRENT_SOURCE_DIR}/s0_smoke_test.c
    )

    target_link_libraries(s0_smoke_test
        PRIVATE
            Os
            Rte
            service_det
            # Real-product BSW services referenced by the Rte ASW component
            # table (Rte_AswScheduler.c): BswM/Com/Dcm/Dem/NvM MainFunctions.
            # (Tech-debt T1: previously absent - the link only succeeded
            # because a weak Os_GlobalState dropped Os_Cfg.c/Os_TaskEntries/
            # Rte_AswScheduler from the link entirely.)
            service_bswm
            service_com
            service_dcm
            service_dem
            service_nvm
            # MemIf (NvM dependency) and PduR (Com dependency)
            service_memif
            service_pdur
            # ECUAL: CanIf (Com dependency) and MemIf (NvM dependency)
            ecual_canif
            ecual_memif
            # MCAL Can + ECUAL Ea/Fee (CanIf and NvM-MemIf hardware backends)
            mcal_can
            ecual_ea
            ecual_fee
            # MCAL Eep + Fls (Ea/Fee hardware backends)
            mcal_eep
            mcal_fls
            # ASW components registered in the Rte component table
            engine_control
            vehicle_dynamics
            diagnostic_manager
            communication_manager
            mode_manager
            watchdog_manager
            storage_manager
            io_control
    )

    set_target_properties(s0_smoke_test PROPERTIES
        C_STANDARD 99
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # MemIf 有意双挂：service_memif（NvM 依赖）+ ecual_memif（ECUAL 抽象层），
    # 与 configurator-linkage 门禁白名单一致（YAC-CI-004 登记）。
    # Linux/GNU ld 需 --allow-multiple-definition 容忍；macOS ld64 无此选项且不报错。
    if(UNIX AND NOT APPLE)
        target_link_options(s0_smoke_test PRIVATE -Wl,--allow-multiple-definition)
    endif()

    add_test(NAME s0_smoke COMMAND s0_smoke_test)

    # Hard ctest-level timeout: the test has its own watchdog, but this
    # keeps CI from hanging even if the watchdog path regresses.
    set_tests_properties(s0_smoke PROPERTIES TIMEOUT 60)
endif()

message(STATUS "Tests configured")
