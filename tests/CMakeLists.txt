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

# Substantiated service-layer unit tests (real production source + mocks)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/ecum)
    add_subdirectory(bsw/services/ecum)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/crc)
    add_subdirectory(bsw/services/crc)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/det)
    add_subdirectory(bsw/services/det)
endif()

# ── T-021 Batch 2: substantiated service-layer tests (anchors replaced by workers) ──
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/bswm)
    add_subdirectory(bsw/services/bswm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/canm)
    add_subdirectory(bsw/services/canm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/cansm)
    add_subdirectory(bsw/services/cansm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/cantpsyn)
    add_subdirectory(bsw/services/cantpsyn)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/com)
    add_subdirectory(bsw/services/com)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/comm)
    add_subdirectory(bsw/services/comm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/csm)
    add_subdirectory(bsw/services/csm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/dcm)
    add_subdirectory(bsw/services/dcm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/dem)
    add_subdirectory(bsw/services/dem)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/dlt)
    add_subdirectory(bsw/services/dlt)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/e2e)
    add_subdirectory(bsw/services/e2e)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/ecuc)
    add_subdirectory(bsw/services/ecuc)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/ea)
    add_subdirectory(bsw/services/ea)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/fim)
    add_subdirectory(bsw/services/fim)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/flsstst)
    add_subdirectory(bsw/services/flsstst)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/keym)
    add_subdirectory(bsw/services/keym)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/ldcom)
    add_subdirectory(bsw/services/ldcom)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/linsm)
    add_subdirectory(bsw/services/linsm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/lntm)
    add_subdirectory(bsw/services/lntm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/memif)
    add_subdirectory(bsw/services/memif)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/mqtt)
    add_subdirectory(bsw/services/mqtt)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/nm)
    add_subdirectory(bsw/services/nm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/nvm)
    add_subdirectory(bsw/services/nvm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/pdur)
    add_subdirectory(bsw/services/pdur)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/ramsafety)
    add_subdirectory(bsw/services/ramsafety)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/ramtst)
    add_subdirectory(bsw/services/ramtst)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/schm)
    add_subdirectory(bsw/services/schm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/sd)
    add_subdirectory(bsw/services/sd)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/secoc)
    add_subdirectory(bsw/services/secoc)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/stbm)
    add_subdirectory(bsw/services/stbm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/swc)
    add_subdirectory(bsw/services/swc)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/tm)
    add_subdirectory(bsw/services/tm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/udpnm)
    add_subdirectory(bsw/services/udpnm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/services/wdgm)
    add_subdirectory(bsw/services/wdgm)
endif()

# ── T-021 Batch 3: substantiated ecual-layer tests (anchors replaced by workers) ──
# @ANCHOR3:cantp
# @ANCHOR3:cantrcv
# @ANCHOR3:cryif
# @ANCHOR3:docan
# @ANCHOR3:doip
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/ecual/eth)
    add_subdirectory(bsw/ecual/eth)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/ecual/ethsm)
    add_subdirectory(bsw/ecual/ethsm)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/ecual/ethswt)
    add_subdirectory(bsw/ecual/ethswt)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/ecual/ethtrcv)
    add_subdirectory(bsw/ecual/ethtrcv)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/ecual/ethtsyn)
    add_subdirectory(bsw/ecual/ethtsyn)
endif()
# @ANCHOR3:frif
# @ANCHOR3:frtp
# @ANCHOR3:iohwab
# @ANCHOR3:ipdum
# @ANCHOR3:j1939nm
# @ANCHOR3:j1939tp
# @ANCHOR3:linif
# @ANCHOR3:linm
# @ANCHOR3:linnm
# @ANCHOR3:lintp
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/ecual/lintrcv)
    add_subdirectory(bsw/ecual/lintrcv)
endif()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bsw/ecual/mem)
    add_subdirectory(bsw/ecual/mem)
endif()
# @ANCHOR3:someip
# @ANCHOR3:someiptp
# @ANCHOR3:someipsd
# @ANCHOR3:someipsd2
# @ANCHOR3:someipxf
# @ANCHOR3:srp
# @ANCHOR3:tcpip
# @ANCHOR3:wdgif
# @ANCHOR3:xcp

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
