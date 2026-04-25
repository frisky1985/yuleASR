#!/usr/bin/env python3
"""
RTE (Runtime Environment) Generator
Generates RTE interface code for AUTOSAR-DDS integration
"""

import os
import re
from datetime import datetime
from typing import List, Dict, Optional, Any, Set
from pathlib import Path
from dataclasses import dataclass

from autosar_arxml_parser import (
    AutosarARXMLParser, ARXMLSoftwareComponent, ARXMLPort,
    ARXMLPortInterface, PortInterfaceType, ARXMLDataElement
)


@dataclass
class RTEConfig:
    """RTE generation configuration"""
    target_os: str = "FreeRTOS"  # FreeRTOS, OSEK, POSIX
    dds_implementation: str = "FastDDS"
    memory_strategy: str = "static"  # static, dynamic, pool
    error_handling: str = "det"  # det, exception, return_code
    safety_level: str = "QM"  # QM, ASIL_A, ASIL_B, ASIL_C, ASIL_D
    use_e2e_protection: bool = False
    optimization: str = "speed"  # speed, size, debug


class RTEGenerator:
    """
    RTE Code Generator for AUTOSAR-DDS Integration
    
    Generates:
    - RTE header files (Rte_<swc>.h)
    - RTE source files (Rte_<swc>.c)
    - RTE type definitions (Rte_Type.h)
    - DDS adapter layer
    - Configuration headers
    """
    
    def __init__(self, config: Optional[RTEConfig] = None):
        self.config = config or RTEConfig()
        self.parser = AutosarARXMLParser()
        self.output_dir = "generated/RTE"
        
    def generate_from_arxml(self, arxml_file: str, output_dir: Optional[str] = None) -> Dict[str, str]:
        """Generate RTE code from ARXML file"""
        if output_dir:
            self.output_dir = output_dir
        
        # Parse ARXML
        if not self.parser.parse_file(arxml_file):
            return {}
        
        generated_files = {}
        
        # Generate RTE types header
        types_header = self._generate_rte_types_header()
        generated_files["Rte_Type.h"] = types_header
        
        # Generate RTE configuration
        rte_cfg = self._generate_rte_config()
        generated_files["Rte_Cfg.h"] = rte_cfg
        
        # Generate per-component RTE files
        for swc_name, swc in self.parser.software_components.items():
            # Generate header
            swc_header = self._generate_swc_rte_header(swc)
            generated_files[f"Rte_{swc_name}.h"] = swc_header
            
            # Generate source
            swc_source = self._generate_swc_rte_source(swc)
            generated_files[f"Rte_{swc_name}.c"] = swc_source
        
        # Generate DDS adapter
        dds_adapter_header = self._generate_dds_adapter_header()
        generated_files["Rte_DdsAdapter.h"] = dds_adapter_header
        
        dds_adapter_source = self._generate_dds_adapter_source()
        generated_files["Rte_DdsAdapter.c"] = dds_adapter_source
        
        # Generate main RTE implementation
        rte_main = self._generate_rte_main()
        generated_files["Rte_Main.c"] = rte_main
        
        # Generate CMakeLists.txt
        cmake_file = self._generate_cmake()
        generated_files["CMakeLists.txt"] = cmake_file
        
        return generated_files
    
    def _generate_rte_types_header(self) -> str:
        """Generate RTE_Type.h - common RTE type definitions"""
        header = f'''/**
 * @file Rte_Type.h
 * @brief RTE Type Definitions
 * @generated {datetime.now().isoformat()}
 * @warning This file is auto-generated. Do not modify manually.
 */

#ifndef RTE_TYPE_H
#define RTE_TYPE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* RTE Version */
#define RTE_MAJOR_VERSION 4
#define RTE_MINOR_VERSION 0
#define RTE_PATCH_VERSION 0

/* Standard Return Codes */
#define RTE_E_OK                    0x00
#define RTE_E_INVALID               0x01
#define RTE_E_TIMEOUT               0x02
#define RTE_E_LIMIT                 0x03
#define RTE_E_NO_DATA               0x04
#define RTE_E_TRANSMIT_ACK          0x05
#define RTE_E_NEVER_RECEIVED        0x06
#define RTE_E_UNCONNECTED           0x07
#define RTE_E_IN_EXCLUSIVE_AREA     0x08
#define RTE_E_SEG_FAULT             0x09
#define RTE_E_OUT_OF_RANGE          0x0A
#define RTE_E_SERIALIZATION_ERROR   0x0B
#define RTE_E_DDS_ERROR             0x0C

/* Data Consistency Types */
typedef enum {{
    RTE_IMPLICIT = 0,
    RTE_EXPLICIT
}} Rte_DataConsistencyType;

/* Buffer Status */
typedef enum {{
    RTE_BUFFER_EMPTY = 0,
    RTE_BUFFER_VALID,
    RTE_BUFFER_INVALID,
    RTE_BUFFER_BUSY
}} Rte_BufferStatus;

/* Communication Status */
typedef enum {{
    RTE_COMM_DISCONNECTED = 0,
    RTE_COMM_CONNECTING,
    RTE_COMM_CONNECTED,
    RTE_COMM_ERROR
}} Rte_CommStatus;

/* DDS-Specific Types */
typedef void* Rte_DdsTopicHandle;
typedef void* Rte_DdsReaderHandle;
typedef void* Rte_DdsWriterHandle;
typedef void* Rte_DdsParticipantHandle;

typedef struct {{
    uint32_t domain_id;
    const char* topic_name;
    const char* type_name;
    const char* qos_profile;
}} Rte_DdsTopicConfig;

/* E2E Protection Types */
#ifdef RTE_USE_E2E_PROTECTION
typedef struct {{
    uint8_t  dataIdMode;
    uint16_t dataId;
    uint16_t dataLength;
    uint8_t  crcOffset;
    uint8_t  counterOffset;
    uint8_t  dataIdNibbleOffset;
}} Rte_E2EConfig;

typedef struct {{
    uint8_t  counter;
    uint16_t crc;
    uint8_t  dataIdNibble;
}} Rte_E2EHeader;
#endif

/* RTE Instance Handle */
typedef uint16_t Rte_InstanceHandle;

/* Standard Data Types */
typedef uint8_t  Rte_Boolean;
typedef uint8_t  Rte_UInt8;
typedef uint16_t Rte_UInt16;
typedef uint32_t Rte_UInt32;
typedef uint64_t Rte_UInt64;
typedef int8_t   Rte_SInt8;
typedef int16_t  Rte_SInt16;
typedef int32_t  Rte_SInt32;
typedef int64_t  Rte_SInt64;
typedef float    Rte_Float32;
typedef double   Rte_Float64;

/* Application-specific types (populated from ARXML) */
'''
        
        # Add type definitions from ARXML
        for dt_name, dt in self.parser.data_types.items():
            c_type = self._map_arxml_type_to_c(dt)
            header += f"typedef {c_type} Rte_{dt_name};\n"
        
        header += f'''

/* RTE Callback Types */
typedef void (*Rte_Callback)(void);
typedef void (*Rte_DataReceivedCallback)(Rte_InstanceHandle instance, const void* data);
typedef void (*Rte_OperationCallback)(Rte_InstanceHandle instance, void* result);

/* RTE Error Hook */
typedef void (*Rte_ErrorHook)(uint8_t errorCode, const char* module, const char* function);

#endif /* RTE_TYPE_H */
'''
        return header
    
    def _generate_rte_config(self) -> str:
        """Generate RTE configuration header"""
        cfg = f'''/**
 * @file Rte_Cfg.h
 * @brief RTE Configuration
 * @generated {datetime.now().isoformat()}
 */

#ifndef RTE_CFG_H
#define RTE_CFG_H

/* OS Configuration */
#define RTE_OS_{self.config.target_os.upper()}
#define RTE_TARGET_OS "{self.config.target_os}"

/* DDS Implementation */
#define RTE_DDS_{self.config.dds_implementation.upper()}

/* Memory Strategy */
#define RTE_MEMORY_{self.config.memory_strategy.upper()}

/* Error Handling */
#define RTE_ERROR_{self.config.error_handling.upper()}

/* Safety Level */
#define RTE_ASIL_{self.config.safety_level.upper()}

/* Feature Configuration */
'''
        
        if self.config.use_e2e_protection:
            cfg += "#define RTE_USE_E2E_PROTECTION 1\n"
        else:
            cfg += "#define RTE_USE_E2E_PROTECTION 0\n"
        
        cfg += f'''
/* Component Configuration */
#define RTE_NUM_COMPONENTS {len(self.parser.software_components)}

/* DDS Configuration */
#define RTE_DDS_DOMAIN_ID 0
#define RTE_DDS_MAX_TOPICS 32
#define RTE_DDS_MAX_PARTICIPANTS 8
#define RTE_DDS_MAX_READERS_PER_TOPIC 4
#define RTE_DDS_MAX_WRITERS_PER_TOPIC 4

/* Buffer Sizes */
#define RTE_DEFAULT_QUEUE_SIZE 10
#define RTE_MAX_QUEUE_SIZE 100

/* Timing Configuration */
#define RTE_DEFAULT_TIMEOUT_MS 100
#define RTE_MAX_TIMEOUT_MS 10000

/* Component Declarations */
'''
        
        # Add component declarations
        for i, swc_name in enumerate(self.parser.software_components.keys()):
            cfg += f"#define RTE_COMPONENT_{swc_name.upper()} {i}\n"
        
        cfg += f'''
/* RTE API Configuration */
#define RTE_API_VERSION_MAJOR 4
#define RTE_API_VERSION_MINOR 0

/* Debug Configuration */
#ifdef RTE_DEBUG
    #define RTE_LOG_LEVEL RTE_LOG_DEBUG
    #define RTE_ENABLE_ASSERTS 1
#else
    #define RTE_LOG_LEVEL RTE_LOG_ERROR
    #define RTE_ENABLE_ASSERTS 0
#endif

#endif /* RTE_CFG_H */
'''
        return cfg
    
    def _generate_swc_rte_header(self, swc: ARXMLSoftwareComponent) -> str:
        """Generate RTE header for a specific SWC"""
        swc_name = swc.name
        swc_upper = swc_name.upper()
        
        header = f'''/**
 * @file Rte_{swc_name}.h
 * @brief RTE Interface for {swc_name}
 * @generated {datetime.now().isoformat()}
 */

#ifndef RTE_{swc_upper}_H
#define RTE_{swc_upper}_H

#include "Rte_Type.h"
#include "Rte_Cfg.h"

/* Instance Handle */
#define Rte_Instance_{swc_name} ((Rte_InstanceHandle)RTE_COMPONENT_{swc_upper})

/* Runnable Declarations */
'''
        
        # Add runnable declarations
        for behavior in swc.internal_behaviors:
            for runnable in behavior.get('runnables', []):
                header += f"void {runnable['symbol']}(void);\n"
        
        header += f'''
/* Port Definitions */
typedef struct {{
    const char* portName;
    uint16_t portId;
    bool isProvided;
    void* interfaceRef;
}} Rte_{swc_name}_PortInfo;

/* Port Handles */
typedef enum {{
'''
        
        # Add port handles
        for i, port in enumerate(swc.ports):
            header += f"    Rte_{swc_name}_Port_{port.name} = {i},\n"
        
        header += f"    Rte_{swc_name}_NumPorts\n}} Rte_{swc_name}_PortType;\n\n"
        
        # Add RTE API functions for each port
        for port in swc.ports:
            header += self._generate_port_api_declarations(swc, port)
        
        header += f'''
/* Initialization */
Std_ReturnType Rte_Init_{swc_name}(void);
Std_ReturnType Rte_Deinit_{swc_name}(void);

/* Port Management */
Rte_CommStatus Rte_Get_{swc_name}_PortStatus(Rte_{swc_name}_PortType port);
Std_ReturnType Rte_Start_{swc_name}(void);
Std_ReturnType Rte_Stop_{swc_name}(void);

#endif /* RTE_{swc_upper}_H */
'''
        return header
    
    def _generate_port_api_declarations(self, swc: ARXMLSoftwareComponent, port: ARXMLPort) -> str:
        """Generate API declarations for a port"""
        port_name = port.name
        swc_name = swc.name
        
        # Get interface
        iface_name = port.interface_ref.split('/')[-1] if port.interface_ref else "Unknown"
        iface = self.parser.port_interfaces.get(iface_name)
        
        if not iface:
            return f"/* Port {port_name}: interface not found */\n"
        
        api = f"/* Port: {port_name} ({'P-Port' if port.is_provided else 'R-Port'}) */\n"
        
        if iface.interface_type == PortInterfaceType.SENDER_RECEIVER:
            for de in iface.data_elements:
                type_name = de.type_ref.split('/')[-1] if de.type_ref else "uint32"
                
                if port.is_provided:
                    # P-Port: Write interface
                    api += f'''
/* Write interface for {de.name} */
Std_ReturnType Rte_Write_{swc_name}_{port_name}_{de.name}(
    const Rte_{type_name}* data
);
Std_ReturnType Rte_Write_{swc_name}_{port_name}_{de.name}_Direct(
    const Rte_{type_name}* data
);
Std_ReturnType Rte_Invalidate_{swc_name}_{port_name}_{de.name}(void);
'''
                else:
                    # R-Port: Read interface
                    api += f'''
/* Read interface for {de.name} */
Std_ReturnType Rte_Read_{swc_name}_{port_name}_{de.name}(
    Rte_{type_name}* data
);
Std_ReturnType Rte_Read_{swc_name}_{port_name}_{de.name}_Direct(
    Rte_{type_name}* data
);
Std_ReturnType Rte_Receive_{swc_name}_{port_name}_{de.name}(
    Rte_{type_name}* data,
    uint32_t timeoutMs
);
Std_ReturnType Rte_IsUpdated_{swc_name}_{port_name}_{de.name}(bool* isUpdated);
'''
        
        elif iface.interface_type == PortInterfaceType.CLIENT_SERVER:
            for op in iface.operations:
                api += f'''
/* Client-Server: {op.name} */
Std_ReturnType Rte_Call_{swc_name}_{port_name}_{op.name}(
'''
                # Add arguments
                for arg in op.arguments:
                    dir_prefix = "const " if arg['direction'] == 'IN' else ""
                    type_name = arg['type_ref'].split('/')[-1] if arg['type_ref'] else "uint32"
                    api += f"    {dir_prefix}Rte_{type_name}* {arg['name']},\n"
                
                api = api.rstrip(",\n") + "\n);\n"
        
        return api
    
    def _generate_swc_rte_source(self, swc: ARXMLSoftwareComponent) -> str:
        """Generate RTE source file for a specific SWC"""
        swc_name = swc.name
        
        source = f'''/**
 * @file Rte_{swc_name}.c
 * @brief RTE Implementation for {swc_name}
 * @generated {datetime.now().isoformat()}
 */

#include "Rte_{swc_name}.h"
#include "Rte_DdsAdapter.h"
#include <string.h>

/* Port Configuration */
'''
        
        # Generate port configurations
        for port in swc.ports:
            source += f'''
static const Rte_{swc_name}_PortInfo Rte_{swc_name}_{port.name}_Info = {{
    .portName = "{port.name}",
    .portId = Rte_{swc_name}_Port_{port.name},
    .isProvided = {("true" if port.is_provided else "false")},
    .interfaceRef = NULL  /* Resolved at runtime */
}};
'''
        
        source += f'''
/* DDS Topic Handles */
'''
        
        # Generate DDS topic handles for each sender-receiver port
        for port in swc.ports:
            iface_name = port.interface_ref.split('/')[-1] if port.interface_ref else ""
            iface = self.parser.port_interfaces.get(iface_name)
            
            if iface and iface.interface_type == PortInterfaceType.SENDER_RECEIVER:
                for de in iface.data_elements:
                    topic_var = f"Rte_{swc_name}_{port.name}_{de.name}_Topic"
                    source += f"static Rte_DdsTopicHandle {topic_var} = NULL;\n"
                    
                    if port.is_provided:
                        source += f"static Rte_DdsWriterHandle Rte_{swc_name}_{port.name}_{de.name}_Writer = NULL;\n"
                    else:
                        source += f"static Rte_DdsReaderHandle Rte_{swc_name}_{port.name}_{de.name}_Reader = NULL;\n"
        
        source += f'''
/* Port Status */
static Rte_CommStatus Rte_{swc_name}_PortStatus[Rte_{swc_name}_NumPorts];

/* Initialization */
Std_ReturnType Rte_Init_{swc_name}(void)
{{
    Std_ReturnType status = RTE_E_OK;
    
    /* Initialize all ports to disconnected */
    for (int i = 0; i < Rte_{swc_name}_NumPorts; i++) {{
        Rte_{swc_name}_PortStatus[i] = RTE_COMM_DISCONNECTED;
    }}
    
    /* Initialize DDS topics and endpoints */
'''
        
        # Add DDS initialization for each port
        for port in swc.ports:
            iface_name = port.interface_ref.split('/')[-1] if port.interface_ref else ""
            iface = self.parser.port_interfaces.get(iface_name)
            
            if iface and iface.interface_type == PortInterfaceType.SENDER_RECEIVER:
                for de in iface.data_elements:
                    topic_name = f"/{swc.parent_package}/{swc_name}/{port.name}/{de.name}"
                    type_name = de.type_ref.split('/')[-1] if de.type_ref else "Unknown"
                    
                    source += f'''
    /* Initialize {port.name}.{de.name} */
    {{
        Rte_DdsTopicConfig topicConfig = {{
            .domain_id = RTE_DDS_DOMAIN_ID,
            .topic_name = "{topic_name}",
            .type_name = "{type_name}",
            .qos_profile = "{self._get_qos_profile(de)}"
        }};
        
        Rte_{swc_name}_{port.name}_{de.name}_Topic = Rte_DdsAdapter_CreateTopic(&topicConfig);
        if (Rte_{swc_name}_{port.name}_{de.name}_Topic == NULL) {{
            status = RTE_E_DDS_ERROR;
        }}
'''
                    if port.is_provided:
                        source += f'''
        else {{
            Rte_{swc_name}_{port.name}_{de.name}_Writer = 
                Rte_DdsAdapter_CreateWriter(Rte_{swc_name}_{port.name}_{de.name}_Topic);
            if (Rte_{swc_name}_{port.name}_{de.name}_Writer != NULL) {{
                Rte_{swc_name}_PortStatus[Rte_{swc_name}_Port_{port.name}] = RTE_COMM_CONNECTED;
            }}
        }}
'''
                    else:
                        source += f'''
        else {{
            Rte_{swc_name}_{port.name}_{de.name}_Reader = 
                Rte_DdsAdapter_CreateReader(Rte_{swc_name}_{port.name}_{de.name}_Topic);
            if (Rte_{swc_name}_{port.name}_{de.name}_Reader != NULL) {{
                Rte_{swc_name}_PortStatus[Rte_{swc_name}_Port_{port.name}] = RTE_COMM_CONNECTED;
            }}
        }}
'''
                    source += "    }\n"
        
        source += f'''
    return status;
}}

Std_ReturnType Rte_Deinit_{swc_name}(void)
{{
    /* Cleanup DDS resources */
'''
        
        # Add cleanup code
        for port in swc.ports:
            iface_name = port.interface_ref.split('/')[-1] if port.interface_ref else ""
            iface = self.parser.port_interfaces.get(iface_name)
            
            if iface and iface.interface_type == PortInterfaceType.SENDER_RECEIVER:
                for de in iface.data_elements:
                    if port.is_provided:
                        source += f'''
    if (Rte_{swc_name}_{port.name}_{de.name}_Writer != NULL) {{
        Rte_DdsAdapter_DeleteWriter(Rte_{swc_name}_{port.name}_{de.name}_Writer);
    }}
'''
                    else:
                        source += f'''
    if (Rte_{swc_name}_{port.name}_{de.name}_Reader != NULL) {{
        Rte_DdsAdapter_DeleteReader(Rte_{swc_name}_{port.name}_{de.name}_Reader);
    }}
'''
                    source += f"    Rte_DdsAdapter_DeleteTopic(Rte_{swc_name}_{port.name}_{de.name}_Topic);\n"
        
        source += f'''
    return RTE_E_OK;
}}

/* Port Status */
Rte_CommStatus Rte_Get_{swc_name}_PortStatus(Rte_{swc_name}_PortType port)
{{
    if (port < Rte_{swc_name}_NumPorts) {{
        return Rte_{swc_name}_PortStatus[port];
    }}
    return RTE_COMM_ERROR;
}}

/* Start/Stop */
Std_ReturnType Rte_Start_{swc_name}(void)
{{
    /* Enable data flow */
    return RTE_E_OK;
}}

Std_ReturnType Rte_Stop_{swc_name}(void)
{{
    /* Disable data flow */
    return RTE_E_OK;
}}
'''
        
        # Add port-specific implementations
        for port in swc.ports:
            source += self._generate_port_api_implementations(swc, port)
        
        return source
    
    def _generate_port_api_implementations(self, swc: ARXMLSoftwareComponent, port: ARXMLPort) -> str:
        """Generate API implementations for a port"""
        swc_name = swc.name
        port_name = port.name
        
        iface_name = port.interface_ref.split('/')[-1] if port.interface_ref else ""
        iface = self.parser.port_interfaces.get(iface_name)
        
        if not iface or iface.interface_type != PortInterfaceType.SENDER_RECEIVER:
            return ""
        
        impl = f"\n/* === Port {port_name} Implementations === */\n"
        
        for de in iface.data_elements:
            type_name = de.type_ref.split('/')[-1] if de.type_ref else "uint32"
            
            if port.is_provided:
                # Write implementation
                impl += f'''
Std_ReturnType Rte_Write_{swc_name}_{port_name}_{de.name}(
    const Rte_{type_name}* data)
{{
    if (data == NULL) {{
        return RTE_E_INVALID;
    }}
    
    if (Rte_{swc_name}_{port_name}_{de.name}_Writer == NULL) {{
        return RTE_E_UNCONNECTED;
    }}
    
    return Rte_DdsAdapter_Write(
        Rte_{swc_name}_{port_name}_{de.name}_Writer,
        data,
        sizeof(Rte_{type_name})
    );
}}

Std_ReturnType Rte_Write_{swc_name}_{port_name}_{de.name}_Direct(
    const Rte_{type_name}* data)
{{
    /* Direct write without buffering */
    return Rte_Write_{swc_name}_{port_name}_{de.name}(data);
}}

Std_ReturnType Rte_Invalidate_{swc_name}_{port_name}_{de.name}(void)
{{
    /* Send invalidation signal */
    return RTE_E_OK;
}}
'''
            else:
                # Read implementation
                impl += f'''
Std_ReturnType Rte_Read_{swc_name}_{port_name}_{de.name}(
    Rte_{type_name}* data)
{{
    if (data == NULL) {{
        return RTE_E_INVALID;
    }}
    
    if (Rte_{swc_name}_{port_name}_{de.name}_Reader == NULL) {{
        return RTE_E_UNCONNECTED;
    }}
    
    return Rte_DdsAdapter_Read(
        Rte_{swc_name}_{port_name}_{de.name}_Reader,
        data,
        sizeof(Rte_{type_name})
    );
}}

Std_ReturnType Rte_Read_{swc_name}_{port_name}_{de.name}_Direct(
    Rte_{type_name}* data)
{{
    /* Direct read without buffering */
    return Rte_Read_{swc_name}_{port_name}_{de.name}(data);
}}

Std_ReturnType Rte_Receive_{swc_name}_{port_name}_{de.name}(
    Rte_{type_name}* data,
    uint32_t timeoutMs)
{{
    if (data == NULL) {{
        return RTE_E_INVALID;
    }}
    
    if (Rte_{swc_name}_{port_name}_{de.name}_Reader == NULL) {{
        return RTE_E_UNCONNECTED;
    }}
    
    return Rte_DdsAdapter_Receive(
        Rte_{swc_name}_{port_name}_{de.name}_Reader,
        data,
        sizeof(Rte_{type_name}),
        timeoutMs
    );
}}

Std_ReturnType Rte_IsUpdated_{swc_name}_{port_name}_{de.name}(bool* isUpdated)
{{
    if (isUpdated == NULL) {{
        return RTE_E_INVALID;
    }}
    
    if (Rte_{swc_name}_{port_name}_{de.name}_Reader == NULL) {{
        return RTE_E_UNCONNECTED;
    }}
    
    *isUpdated = Rte_DdsAdapter_IsDataAvailable(
        Rte_{swc_name}_{port_name}_{de.name}_Reader
    );
    
    return RTE_E_OK;
}}
'''
        
        return impl
    
    def _generate_dds_adapter_header(self) -> str:
        """Generate DDS adapter header"""
        return f'''/**
 * @file Rte_DdsAdapter.h
 * @brief RTE-DDS Adapter Interface
 * @generated {datetime.now().isoformat()}
 */

#ifndef RTE_DDS_ADAPTER_H
#define RTE_DDS_ADAPTER_H

#include "Rte_Type.h"

#ifdef __cplusplus
extern "C" {{
#endif

/* DDS Adapter API */
Rte_DdsTopicHandle Rte_DdsAdapter_CreateTopic(const Rte_DdsTopicConfig* config);
void Rte_DdsAdapter_DeleteTopic(Rte_DdsTopicHandle topic);

Rte_DdsWriterHandle Rte_DdsAdapter_CreateWriter(Rte_DdsTopicHandle topic);
void Rte_DdsAdapter_DeleteWriter(Rte_DdsWriterHandle writer);
Std_ReturnType Rte_DdsAdapter_Write(
    Rte_DdsWriterHandle writer,
    const void* data,
    uint32_t size
);

Rte_DdsReaderHandle Rte_DdsAdapter_CreateReader(Rte_DdsTopicHandle topic);
void Rte_DdsAdapter_DeleteReader(Rte_DdsReaderHandle reader);
Std_ReturnType Rte_DdsAdapter_Read(
    Rte_DdsReaderHandle reader,
    void* data,
    uint32_t size
);
Std_ReturnType Rte_DdsAdapter_Receive(
    Rte_DdsReaderHandle reader,
    void* data,
    uint32_t size,
    uint32_t timeoutMs
);
bool Rte_DdsAdapter_IsDataAvailable(Rte_DdsReaderHandle reader);

/* Participant Management */
Rte_DdsParticipantHandle Rte_DdsAdapter_CreateParticipant(uint32_t domainId);
void Rte_DdsAdapter_DeleteParticipant(Rte_DdsParticipantHandle participant);

#ifdef __cplusplus
}}
#endif

#endif /* RTE_DDS_ADAPTER_H */
'''
    
    def _generate_dds_adapter_source(self) -> str:
        """Generate DDS adapter implementation"""
        return f'''/**
 * @file Rte_DdsAdapter.c
 * @brief RTE-DDS Adapter Implementation
 * @generated {datetime.now().isoformat()}
 */

#include "Rte_DdsAdapter.h"
#include "Rte_Cfg.h"

#if defined(RTE_DDS_FASTDDS)
    #include <fastdds/dds/domain/DomainParticipantFactory.hpp>
    #include <fastdds/dds/domain/DomainParticipant.hpp>
    #include <fastdds/dds/topic/Topic.hpp>
    #include <fastdds/dds/publisher/Publisher.hpp>
    #include <fastdds/dds/publisher/DataWriter.hpp>
    #include <fastdds/dds/subscriber/Subscriber.hpp>
    #include <fastdds/dds/subscriber/DataReader.hpp>
#elif defined(RTE_DDS_CYCLONE)
    #include <dds/dds.h>
#endif

#include <stdlib.h>
#include <string.h>

/* DDS Adapter Implementation */

Rte_DdsTopicHandle Rte_DdsAdapter_CreateTopic(const Rte_DdsTopicConfig* config)
{{
    /* Platform-specific implementation */
    #if defined(RTE_DDS_FASTDDS)
        /* FastDDS implementation */
    #elif defined(RTE_DDS_CYCLONE)
        /* CycloneDDS implementation */
    #endif
    return NULL;
}}

void Rte_DdsAdapter_DeleteTopic(Rte_DdsTopicHandle topic)
{{
    if (topic != NULL) {{
        /* Cleanup implementation */
    }}
}}

Rte_DdsWriterHandle Rte_DdsAdapter_CreateWriter(Rte_DdsTopicHandle topic)
{{
    return NULL;
}}

void Rte_DdsAdapter_DeleteWriter(Rte_DdsWriterHandle writer)
{{
}}

Std_ReturnType Rte_DdsAdapter_Write(
    Rte_DdsWriterHandle writer,
    const void* data,
    uint32_t size)
{{
    return RTE_E_OK;
}}

Rte_DdsReaderHandle Rte_DdsAdapter_CreateReader(Rte_DdsTopicHandle topic)
{{
    return NULL;
}}

void Rte_DdsAdapter_DeleteReader(Rte_DdsReaderHandle reader)
{{
}}

Std_ReturnType Rte_DdsAdapter_Read(
    Rte_DdsReaderHandle reader,
    void* data,
    uint32_t size)
{{
    return RTE_E_OK;
}}

Std_ReturnType Rte_DdsAdapter_Receive(
    Rte_DdsReaderHandle reader,
    void* data,
    uint32_t size,
    uint32_t timeoutMs)
{{
    return RTE_E_OK;
}}

bool Rte_DdsAdapter_IsDataAvailable(Rte_DdsReaderHandle reader)
{{
    return false;
}}

Rte_DdsParticipantHandle Rte_DdsAdapter_CreateParticipant(uint32_t domainId)
{{
    return NULL;
}}

void Rte_DdsAdapter_DeleteParticipant(Rte_DdsParticipantHandle participant)
{{
}}
'''
    
    def _generate_rte_main(self) -> str:
        """Generate RTE main implementation"""
        return f'''/**
 * @file Rte_Main.c
 * @brief RTE Main Implementation
 * @generated {datetime.now().isoformat()}
 */

#include "Rte_Type.h"
#include "Rte_Cfg.h"
'''
    
    def _generate_cmake(self) -> str:
        """Generate CMakeLists.txt"""
        return f'''cmake_minimum_required(VERSION 3.10)
project(RTE_Generated)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source files
set(RTE_SOURCES
'''
    
    def _map_arxml_type_to_c(self, dt) -> str:
        """Map ARXML type to C type"""
        type_mapping = {
            'uint8': 'uint8_t',
            'uint16': 'uint16_t',
            'uint32': 'uint32_t',
            'sint8': 'int8_t',
            'sint16': 'int16_t',
            'sint32': 'int32_t',
            'float32': 'float',
            'float64': 'double',
            'boolean': 'bool'
        }
        return type_mapping.get(dt.name.lower(), 'uint32_t')
    
    def _get_qos_profile(self, de: ARXMLDataElement) -> str:
        """Get DDS QoS profile for data element"""
        if de.is_queued:
            return "reliable_transient_local"
        return "best_effort_volatile"
    
    def write_files(self, output_dir: str, files: Dict[str, str]):
        """Write generated files to disk"""
        os.makedirs(output_dir, exist_ok=True)
        
        for filename, content in files.items():
            filepath = os.path.join(output_dir, filename)
            with open(filepath, 'w') as f:
                f.write(content)
            print(f"Generated: {filepath}")


# Example usage
if __name__ == '__main__':
    config = RTEConfig(
        target_os="FreeRTOS",
        dds_implementation="FastDDS",
        memory_strategy="static",
        safety_level="ASIL_D"
    )
    
    generator = RTEGenerator(config)
    
    # Example would require actual ARXML file
    print("RTE Generator ready")