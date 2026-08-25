#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
RTE Generator Tool for YuleTech AutoSAR BSW

Generates RTE interface code (C headers and sources) from JSON SWC configuration.
Follows AutoSAR Classic Platform 4.x standard.

Usage:
    python rte_generator.py --config example_config.json --output src/bsw/rte/generated/
"""

import argparse
import json
import os
import sys
from datetime import datetime


# =============================================================================
# Template fragments
# =============================================================================

HEADER_TEMPLATE = """/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Runtime Environment)
* Dependencies         : Rte, Com, NvM, Dcm
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : {build_date}
* Author               : RTE Generator
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Rte.h"
#include "Rte_Type.h"
#include "Rte_{swc_name}.h"
{extra_includes}

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define RTE_{swc_name_upper}_INSTANCE_ID             (0x00U)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
{local_types}

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define RTE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

{local_vars}

#define RTE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
{local_prototypes}

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

{local_functions}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/
{global_functions}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
"""

HEADER_H_TEMPLATE = """/**
 * @file Rte_{swc_name}.h
 * @brief RTE Interface for {swc_name} following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date {build_date}
 * @author RTE Generator
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Runtime Environment (RTE)
 * Layer: RTE (Runtime Environment)
 * Purpose: Generated RTE interface for Software Component {swc_name}
 *
 * @note THIS FILE IS AUTO-GENERATED. DO NOT MODIFY MANUALLY.
 */

#ifndef RTE_{swc_name_upper}_H
#define RTE_{swc_name_upper}_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte.h"
#include "Rte_Type.h"
{extra_includes}

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define RTE_{swc_name_upper}_VENDOR_ID                   (0x01U)
#define RTE_{swc_name_upper}_MODULE_ID                   (0x70U)
#define RTE_{swc_name_upper}_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define RTE_{swc_name_upper}_AR_RELEASE_MINOR_VERSION    (0x04U)
#define RTE_{swc_name_upper}_SW_MAJOR_VERSION            (0x01U)
#define RTE_{swc_name_upper}_SW_MINOR_VERSION            (0x00U)
#define RTE_{swc_name_upper}_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
{local_types}

/*==================================================================================================
*                                    RTE API FUNCTION PROTOTYPES
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

{function_prototypes}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE API MACROS
==================================================================================================*/
{macros}

#endif /* RTE_{swc_name_upper}_H */
"""


def to_c_type(data_type):
    """Map JSON data type string to C type."""
    mapping = {
        "uint8": "uint8",
        "uint16": "uint16",
        "uint32": "uint32",
        "sint8": "sint8",
        "sint16": "sint16",
        "sint32": "sint32",
        "boolean": "boolean",
        "float32": "float32",
        "float64": "float64",
    }
    return mapping.get(data_type, data_type)


def generate_struct_type(name, fields):
    """Generate a C struct definition from field list."""
    lines = [f"typedef struct", "{"]
    for field in fields:
        lines.append(f"    {to_c_type(field['type'])} {field['name']};")
    lines.append(f"}} {name};")
    return "\n".join(lines)


def parse_config(config_path):
    """Parse JSON configuration file."""
    with open(config_path, 'r', encoding='utf-8') as f:
        return json.load(f)


def generate_swc_header(swc, build_date):
    """Generate Rte_SwcName.h header file content."""
    swc_name = swc["name"]
    swc_name_upper = swc_name.upper()
    ports = swc.get("ports", [])

    extra_includes = set()
    prototypes = []
    macros = []
    struct_types = []

    for port in ports:
        interface_type = port["interfaceType"]
        port_name = port["name"]

        if interface_type == "SenderReceiver":
            for de in port.get("dataElements", []):
                de_name = de["name"]
                de_type = de["type"]
                c_type = to_c_type(de_type)

                if port["direction"] == "Required":
                    # Receiver: Rte_Read
                    prototypes.append(
                        f"extern Std_ReturnType Rte_Read_{swc_name}_{port_name}_{de_name}({c_type}* data);"
                    )
                    macros.append(
                        f"#define RTE_READ_{swc_name_upper}_{port_name.upper()}_{de_name.upper()}(data) \\\n"
                        f"    Rte_Read_{swc_name}_{port_name}_{de_name}(data)"
                    )
                else:
                    # Sender: Rte_Write
                    prototypes.append(
                        f"extern Std_ReturnType Rte_Write_{swc_name}_{port_name}_{de_name}(const {c_type}* data);"
                    )
                    macros.append(
                        f"#define RTE_WRITE_{swc_name_upper}_{port_name.upper()}_{de_name.upper()}(data) \\\n"
                        f"    Rte_Write_{swc_name}_{port_name}_{de_name}(data)"
                    )

                # Also generate Send/Receive macros for queued/non-queued
                if port["direction"] == "Provided":
                    prototypes.append(
                        f"extern Std_ReturnType Rte_Send_{swc_name}_{port_name}(const {c_type}* data);"
                    )
                    macros.append(
                        f"#define RTE_SEND_{swc_name_upper}_{port_name.upper()}(data) \\\n"
                        f"    Rte_Send_{swc_name}_{port_name}(data)"
                    )
                else:
                    prototypes.append(
                        f"extern Std_ReturnType Rte_Receive_{swc_name}_{port_name}({c_type}* data);"
                    )
                    macros.append(
                        f"#define RTE_RECEIVE_{swc_name_upper}_{port_name.upper()}(data) \\\n"
                        f"    Rte_Receive_{swc_name}_{port_name}(data)"
                    )

        elif interface_type == "NvBlock":
            extra_includes.add("#include \"NvM.h\"")
            for de in port.get("dataElements", []):
                de_name = de["name"]
                de_type = de["type"]

                if isinstance(de_type, dict) and de_type.get("kind") == "struct":
                    struct_name = f"Rte_{swc_name}_{port_name}_{de_name}Type"
                    struct_types.append(generate_struct_type(struct_name, de_type.get("fields", [])))
                    c_type = struct_name
                else:
                    c_type = to_c_type(de_type)

                prototypes.append(
                    f"extern Std_ReturnType Rte_Read_{swc_name}_{port_name}_{de_name}({c_type}* data);"
                )
                prototypes.append(
                    f"extern Std_ReturnType Rte_Write_{swc_name}_{port_name}_{de_name}(const {c_type}* data);"
                )
                macros.append(
                    f"#define RTE_READ_{swc_name_upper}_{port_name.upper()}_{de_name.upper()}(data) \\\n"
                    f"    Rte_Read_{swc_name}_{port_name}_{de_name}(data)"
                )
                macros.append(
                    f"#define RTE_WRITE_{swc_name_upper}_{port_name.upper()}_{de_name.upper()}(data) \\\n"
                    f"    Rte_Write_{swc_name}_{port_name}_{de_name}(data)"
                )

        elif interface_type == "ClientServer":
            if port["direction"] == "Required":
                # Client port: Rte_Call
                for op in port.get("operations", []):
                    op_name = op["name"]
                    params = []
                    args = []
                    for p in op.get("arguments", []):
                        p_type = to_c_type(p["type"])
                        if p["direction"] == "in":
                            params.append(f"{p_type} {p['name']}")
                        else:
                            params.append(f"{p_type}* {p['name']}")
                        args.append(p["name"])
                    params_str = ", ".join(params) if params else "void"
                    args_str = ", ".join(args)
                    prototypes.append(
                        f"extern Std_ReturnType Rte_Call_{swc_name}_{port_name}_{op_name}({params_str});"
                    )
                    macros.append(
                        f"#define RTE_CALL_{swc_name_upper}_{port_name.upper()}_{op_name.upper()}({args_str}) \\\n"
                        f"    Rte_Call_{swc_name}_{port_name}_{op_name}({args_str})"
                    )
            else:
                # Server port: Rte_Result + Runnable skeleton
                for op in port.get("operations", []):
                    op_name = op["name"]
                    params = []
                    args = []
                    for p in op.get("arguments", []):
                        p_type = to_c_type(p["type"])
                        if p["direction"] == "in":
                            params.append(f"{p_type} {p['name']}")
                        else:
                            params.append(f"{p_type}* {p['name']}")
                        args.append(p["name"])
                    params_str = ", ".join(params) if params else "void"
                    args_str = ", ".join(args)
                    prototypes.append(
                        f"extern Std_ReturnType Rte_Result_{swc_name}_{port_name}_{op_name}({params_str});"
                    )
                    prototypes.append(
                        f"extern Std_ReturnType Rte_Server_{swc_name}_{port_name}_{op_name}({params_str});"
                    )
                    macros.append(
                        f"#define RTE_RESULT_{swc_name_upper}_{port_name.upper()}_{op_name.upper()}({args_str}) \\\n"
                        f"    Rte_Result_{swc_name}_{port_name}_{op_name}({args_str})"
                    )

        elif interface_type == "ModeSwitch":
            extra_includes.add("#include \"Rte_Bsw.h\"")
            mode_group = port.get("modeGroup", "ModeGroup")
            prototypes.append(
                f"extern Rte_StatusType Rte_Switch_{swc_name}_{port_name}_{mode_group}(uint32 mode);"
            )
            prototypes.append(
                f"extern Rte_StatusType Rte_Mode_{swc_name}_{port_name}_{mode_group}(uint32* mode);"
            )
            macros.append(
                f"#define RTE_SWITCH_{swc_name_upper}_{port_name.upper()}_{mode_group.upper()}(mode) \\\n"
                f"    Rte_Switch_{swc_name}_{port_name}_{mode_group}(mode)"
            )
            macros.append(
                f"#define RTE_MODE_{swc_name_upper}_{port_name.upper()}_{mode_group.upper()}(mode) \\\n"
                f"    Rte_Mode_{swc_name}_{port_name}_{mode_group}(mode)"
            )

    local_types_str = "\n".join(struct_types) if struct_types else "/* No local types defined */"
    extra_includes_str = "\n".join(sorted(extra_includes))
    prototypes_str = "\n".join(prototypes)
    macros_str = "\n".join(macros)

    return HEADER_H_TEMPLATE.format(
        swc_name=swc_name,
        swc_name_upper=swc_name_upper,
        build_date=build_date,
        extra_includes=extra_includes_str,
        local_types=local_types_str,
        function_prototypes=prototypes_str,
        macros=macros_str,
    )


def generate_swc_source(swc, build_date):
    """Generate Rte_SwcName.c source file content."""
    swc_name = swc["name"]
    swc_name_upper = swc_name.upper()
    ports = swc.get("ports", [])

    extra_includes = set()
    local_types_list = []
    local_vars_list = []
    local_prototypes_list = []
    local_functions_list = []
    global_functions_list = []

    # Track COM signal IDs if any
    com_signals = []
    nvm_blocks = []

    for port in ports:
        interface_type = port["interfaceType"]
        port_name = port["name"]

        if interface_type == "SenderReceiver":
            extra_includes.add("#include \"Com.h\"")
            extra_includes.add("#include \"Rte_Cfg.h\"")
            for de in port.get("dataElements", []):
                de_name = de["name"]
                de_type = de["type"]
                c_type = to_c_type(de_type)
                buffer_name = f"Rte_Buffer_{swc_name}_{port_name}_{de_name}"
                signal_id_define = f"RTE_COMSIGNAL_{swc_name_upper}_{port_name.upper()}_{de_name.upper()}"
                com_signals.append({
                    "define": signal_id_define,
                    "id": de.get("comSignalId", 0),
                })

                # Static buffer
                local_vars_list.append(f"STATIC {c_type} {buffer_name};")
                local_vars_list.append(f"STATIC boolean Rte_IsUpdated_{swc_name}_{port_name}_{de_name} = FALSE;")

                if port["direction"] == "Required":
                    # Receiver
                    global_functions_list.append(
                        f"""
/**
 * @brief   Read {de_name} from {port_name}
 */
Std_ReturnType Rte_Read_{swc_name}_{port_name}_{de_name}({c_type}* data)
{{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {{
        uint8 comResult = Com_ReceiveSignal({signal_id_define}, data);
        if (comResult == COM_SERVICE_OK)
        {{
            result = E_OK;
        }}
    }}

    return result;
}}"""
                    )
                    global_functions_list.append(
                        f"""
/**
 * @brief   Receive {de_name} from {port_name}
 */
Std_ReturnType Rte_Receive_{swc_name}_{port_name}({c_type}* data)
{{
    return Rte_Read_{swc_name}_{port_name}_{de_name}(data);
}}"""
                    )
                else:
                    # Sender
                    global_functions_list.append(
                        f"""
/**
 * @brief   Write {de_name} to {port_name}
 */
Std_ReturnType Rte_Write_{swc_name}_{port_name}_{de_name}(const {c_type}* data)
{{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {{
        uint8 comResult = Com_SendSignal({signal_id_define}, data);
        if (comResult == E_OK)
        {{
            {buffer_name} = *data;
            Rte_IsUpdated_{swc_name}_{port_name}_{de_name} = TRUE;
            result = E_OK;
        }}
    }}

    return result;
}}"""
                    )
                    global_functions_list.append(
                        f"""
/**
 * @brief   Send {de_name} via {port_name}
 */
Std_ReturnType Rte_Send_{swc_name}_{port_name}(const {c_type}* data)
{{
    return Rte_Write_{swc_name}_{port_name}_{de_name}(data);
}}"""
                    )

        elif interface_type == "NvBlock":
            extra_includes.add("#include \"NvM.h\"")
            for de in port.get("dataElements", []):
                de_name = de["name"]
                de_type = de["type"]
                buffer_name = f"Rte_Buffer_{swc_name}_{port_name}_{de_name}"
                block_id_define = f"RTE_NVMBLOCK_{swc_name_upper}_{port_name.upper()}_{de_name.upper()}"
                nvm_blocks.append({
                    "define": block_id_define,
                    "id": de.get("nvmBlockId", 0),
                })

                if isinstance(de_type, dict) and de_type.get("kind") == "struct":
                    struct_name = f"Rte_{swc_name}_{port_name}_{de_name}Type"
                    c_type = struct_name
                else:
                    c_type = to_c_type(de_type)

                # Static buffer
                local_vars_list.append(f"STATIC {c_type} {buffer_name};")
                local_vars_list.append(f"STATIC boolean {buffer_name}_IsValid = FALSE;")

                global_functions_list.append(
                    f"""
/**
 * @brief   Read {de_name} from NV memory via {port_name}
 */
Std_ReturnType Rte_Read_{swc_name}_{port_name}_{de_name}({c_type}* data)
{{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {{
        result = NvM_ReadBlock({block_id_define}, data);
        if (result == E_OK)
        {{
            {buffer_name}_IsValid = TRUE;
        }}
    }}

    return result;
}}"""
                )
                global_functions_list.append(
                    f"""
/**
 * @brief   Write {de_name} to NV memory via {port_name}
 */
Std_ReturnType Rte_Write_{swc_name}_{port_name}_{de_name}(const {c_type}* data)
{{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {{
        result = NvM_WriteBlock({block_id_define}, data);
        if (result == E_OK)
        {{
            {buffer_name} = *data;
            {buffer_name}_IsValid = TRUE;
        }}
    }}

    return result;
}}"""
                )

        elif interface_type == "ClientServer":
            extra_includes.add("#include \"Rte_Bsw.h\"")
            extra_includes.add("#include \"Rte_Cfg.h\"")
            extra_includes.add("#include \"string.h\"")
            if port["direction"] == "Required":
                extra_includes.add("#include \"Com.h\"")
            if port["direction"] == "Required":
                # Client port - add static result buffers and generate functions
                for op in port.get("operations", []):
                    op_name = op["name"]
                    result_buf = f"Rte_CSResult_{swc_name}_{port_name}_{op_name}"
                    local_vars_list.append(f"STATIC Std_ReturnType {result_buf} = E_OK;")
                    local_vars_list.append(f"STATIC boolean {result_buf}_Valid = FALSE;")
                    arg_buf = f"Rte_CSArgBuffer_{swc_name}_{port_name}_{op_name}"
                    local_vars_list.append(f"STATIC uint8 {arg_buf}[RTE_MAX_BUFFER_SIZE];")

                    # Add COM signal ID for CS operation dispatch
                    cs_signal_define = f"RTE_COMSIGNAL_{swc_name_upper}_{port_name.upper()}_{op_name.upper()}"
                    cs_signal_id = len(com_signals) + 0x100  # CS signals start at 0x100
                    com_signals.append({
                        "define": cs_signal_define,
                        "id": cs_signal_id,
                    })

                    params = []
                    args = []
                    out_args = []
                    for p in op.get("arguments", []):
                        p_type = to_c_type(p["type"])
                        if p["direction"] == "in":
                            params.append(f"{p_type} {p['name']}")
                        else:
                            params.append(f"{p_type}* {p['name']}")
                            out_args.append(p["name"])
                        args.append(p["name"])
                    params_str = ", ".join(params) if params else "void"

                    # Build argument marshaling: copy 'in' args into static buffer
                    marshal_lines = []
                    offset = 0
                    for p in op.get("arguments", []):
                        p_type = to_c_type(p["type"])
                        if p["direction"] == "in":
                            marshal_lines.append(
                                f"    (void)memcpy(&{arg_buf}[{offset}U], (const void*)&{p['name']}, sizeof({p_type}));"
                            )
                            offset_str = f"sizeof({p_type})"
                            offset += 1  # symbolic; actual offset computed at runtime
                    marshal_code = "\n".join(marshal_lines) if marshal_lines else "    /* No input arguments to marshal */"

                    # Build out-argument initialization
                    out_init_lines = []
                    for a in out_args:
                        out_init_lines.append(f"    if ({a} != NULL_PTR) {{ *{a} = ({to_c_type('uint8')})0U; }}")
                    out_init_code = "\n".join(out_init_lines) if out_init_lines else "    /* No output arguments */"

                    global_functions_list.append(
                        f"""
/**
 * @brief   Call {op_name} on {port_name} (Client)
 * @details Marshals input arguments into static buffer, dispatches via COM signal,
 *          and stores result for asynchronous retrieval via Rte_Result.
 */
Std_ReturnType Rte_Call_{swc_name}_{port_name}_{op_name}({params_str})
{{
    Std_ReturnType result = E_NOT_OK;

    /* Marshal input arguments into static buffer */
{marshal_code}

#if (RTE_COM_SUPPORT == STD_ON)
    /* Dispatch client-server call via COM signal */
    result = Rte_ComSendSignal(RTE_COMSIGNAL_{swc_name_upper}_{port_name.upper()}_{op_name.upper()}, (const void*){arg_buf});
#else
    /* Local client-server call: result pending */
    result = E_OK;
#endif

    if (result == E_OK)
    {{
        /* Initialize output arguments */
{out_init_code}

        /* Store result for async retrieval */
        {result_buf} = result;
        {result_buf}_Valid = TRUE;
    }}

    return result;
}}"""
                    )
            else:
                # Server port - add static result buffers and generate functions
                for op in port.get("operations", []):
                    op_name = op["name"]
                    result_buf = f"Rte_CSResult_{swc_name}_{port_name}_{op_name}"
                    local_vars_list.append(f"STATIC Std_ReturnType {result_buf} = E_OK;")
                    local_vars_list.append(f"STATIC boolean {result_buf}_Valid = FALSE;")

                    params = []
                    args = []
                    for p in op.get("arguments", []):
                        p_type = to_c_type(p["type"])
                        if p["direction"] == "in":
                            params.append(f"{p_type} {p['name']}")
                        else:
                            params.append(f"{p_type}* {p['name']}")
                        args.append(p["name"])
                    params_str = ", ".join(params) if params else "void"

                    # Build void-cast for unused args (server skeleton)
                    void_cast_lines = []
                    for a in args:
                        void_cast_lines.append(f"    (void){a};")
                    void_cast_code = "\n".join(void_cast_lines) if void_cast_lines else "    /* No arguments */"

                    global_functions_list.append(
                        f"""
/**
 * @brief   Server implementation of {op_name} on {port_name}
 * @details Server-side operation handler. This skeleton accepts the call
 *          and returns E_OK. Application logic should be integrated here.
 */
Std_ReturnType Rte_Server_{swc_name}_{port_name}_{op_name}({params_str})
{{
    Std_ReturnType result = E_OK;

    /* Server operation dispatch - application logic to be integrated */
{void_cast_code}

    /* Store result for async retrieval */
    {result_buf} = result;
    {result_buf}_Valid = TRUE;

    return result;
}}"""
                    )
                    global_functions_list.append(
                        f"""
/**
 * @brief   Get result of {op_name} on {port_name}
 * @details Retrieves the stored result from the last server operation invocation.
 *          Returns E_OK if a valid result is available, E_NOT_OK otherwise.
 */
Std_ReturnType Rte_Result_{swc_name}_{port_name}_{op_name}({params_str})
{{
    Std_ReturnType result;

    if ({result_buf}_Valid != FALSE)
    {{
        result = {result_buf};
        /* Clear validity after retrieval (one-shot semantics) */
        {result_buf}_Valid = FALSE;
    }}
    else
    {{
        result = E_NOT_OK;
    }}

{void_cast_code}

    return result;
}}"""
                    )

        elif interface_type == "ModeSwitch":
            extra_includes.add("#include \"Rte_Bsw.h\"")
            extra_includes.add("#include \"Rte_Cfg.h\"")
            mode_group = port.get("modeGroup", "ModeGroup")
            mode_var = f"Rte_CurrentMode_{swc_name}_{port_name}_{mode_group}"
            local_vars_list.append(f"STATIC uint32 {mode_var} = 0U;")
            local_vars_list.append(f"STATIC boolean {mode_var}_Valid = FALSE;")

            global_functions_list.append(
                f"""
/**
 * @brief   Switch mode on {port_name}
 * @details Requests a mode transition via BswM and stores the requested mode
 *          in the local mode buffer. Returns RTE_E_OK on success.
 */
Rte_StatusType Rte_Switch_{swc_name}_{port_name}_{mode_group}(uint32 mode)
{{
    Rte_StatusType result = RTE_E_OK;

#if (RTE_COM_SUPPORT == STD_ON)
    /* Request mode switch via BswM interface */
    Std_ReturnType bswResult;
    bswResult = Rte_Bsw_BswM_RequestMode(RTE_BSW_COMPONENT_MODE_MANAGER, (uint8)mode);
    if (bswResult != E_OK)
    {{
        result = RTE_E_NOK;
    }}
    else
    {{
        {mode_var} = mode;
        {mode_var}_Valid = TRUE;
    }}
#else
    /* Store requested mode locally */
    {mode_var} = mode;
    {mode_var}_Valid = TRUE;
#endif

    return result;
}}"""
            )
            global_functions_list.append(
                f"""
/**
 * @brief   Get current mode on {port_name}
 * @details Reads the current mode from the local mode buffer.
 *          Returns RTE_E_OK if a valid mode is available, RTE_E_NO_DATA otherwise.
 */
Rte_StatusType Rte_Mode_{swc_name}_{port_name}_{mode_group}(uint32* mode)
{{
    Rte_StatusType result = RTE_E_OK;

    if (mode != NULL_PTR)
    {{
        if ({mode_var}_Valid != FALSE)
        {{
            *mode = {mode_var};
        }}
        else
        {{
            *mode = 0U;
            result = RTE_E_NO_DATA;
        }}
    }}
    else
    {{
        result = RTE_E_SEG_FAULT;
    }}

    return result;
}}"""
            )

    # Build signal/block defines
    defines_lines = []
    for sig in com_signals:
        defines_lines.append(f"#define {sig['define']}    ({sig['id']}U)")
    for blk in nvm_blocks:
        defines_lines.append(f"#define {blk['define']}    ({blk['id']}U)")

    local_types_str = "\n".join(local_types_list) if local_types_list else "/* No local types */"
    local_vars_str = "\n".join(local_vars_list) if local_vars_list else "/* No local variables */"
    local_prototypes_str = "\n".join(local_prototypes_list) if local_prototypes_list else "/* No local prototypes */"
    local_functions_str = "\n".join(local_functions_list) if local_functions_list else "/* No local functions */"
    global_functions_str = "\n".join(global_functions_list)

    extra_includes_str = "\n".join(sorted(extra_includes))

    # Prepend defines
    all_content = "\n".join(defines_lines) + "\n\n" if defines_lines else ""

    return all_content + HEADER_TEMPLATE.format(
        build_date=build_date,
        extra_includes=extra_includes_str,
        local_types=local_types_str,
        local_vars=local_vars_str,
        local_prototypes=local_prototypes_str,
        local_functions=local_functions_str,
        global_functions=global_functions_str,
        swc_name=swc_name,
        swc_name_upper=swc_name_upper,
    )


def generate_rte_code(config, output_dir):
    """Generate all RTE files from configuration."""
    build_date = datetime.now().strftime("%Y-%m-%d")
    swcs = config.get("softwareComponents", [])

    os.makedirs(output_dir, exist_ok=True)

    generated_files = []

    for swc in swcs:
        swc_name = swc["name"]

        # Generate header
        h_content = generate_swc_header(swc, build_date)
        h_path = os.path.join(output_dir, f"Rte_{swc_name}.h")
        with open(h_path, 'w', encoding='utf-8') as f:
            f.write(h_content)
        generated_files.append(h_path)

        # Generate source
        c_content = generate_swc_source(swc, build_date)
        c_path = os.path.join(output_dir, f"Rte_{swc_name}.c")
        with open(c_path, 'w', encoding='utf-8') as f:
            f.write(c_content)
        generated_files.append(c_path)

    return generated_files


def main():
    parser = argparse.ArgumentParser(
        description="YuleTech AutoSAR RTE Generator - Generates RTE interface code from JSON config"
    )
    parser.add_argument(
        "--config",
        required=True,
        help="Path to JSON configuration file describing SWCs, ports, and interfaces"
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Output directory for generated C header and source files"
    )
    args = parser.parse_args()

    if not os.path.isfile(args.config):
        print(f"Error: Configuration file not found: {args.config}", file=sys.stderr)
        sys.exit(1)

    try:
        config = parse_config(args.config)
    except json.JSONDecodeError as e:
        print(f"Error: Failed to parse JSON config: {e}", file=sys.stderr)
        sys.exit(1)

    generated = generate_rte_code(config, args.output)

    print(f"RTE Generator completed successfully.")
    print(f"Generated {len(generated)} file(s):")
    for f in generated:
        print(f"  - {f}")


if __name__ == "__main__":
    main()
