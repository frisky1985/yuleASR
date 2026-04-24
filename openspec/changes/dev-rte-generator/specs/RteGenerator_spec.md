# RTE Generator Tool Specification

> **Tool:** RTE Generator  
> **Layer:** RTE (Runtime Environment)  
> **Standard:** AutoSAR Classic Platform 4.x  
> **Platform:** NXP i.MX8M Mini  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Tool Overview

The RTE Generator is a Python-based code generation tool that automatically produces RTE interface C code from a JSON configuration file describing Software Components (SWCs), their ports, interfaces, and data elements.

The tool bridges the gap between high-level SWC architectural descriptions and low-level RTE implementation code, ensuring consistent naming conventions, buffer management, and service layer integration across the BSW stack.

### Key Responsibilities
- Parse JSON SWC/Port/Interface/DataElement configurations
- Generate `Rte_SwcName.h` - Public RTE interface headers per SWC
- Generate `Rte_SwcName.c` - Implementation files with static buffers and function bodies
- Map SenderReceiver interfaces to COM signal APIs (`Com_SendSignal` / `Com_ReceiveSignal`)
- Map NvBlock interfaces to NVM APIs (`NvM_ReadBlock` / `NvM_WriteBlock`)
- Generate Client/Server call and result stubs following AutoSAR CS interface patterns
- Generate ModeSwitch APIs (`Rte_Switch` / `Rte_Mode`)

---

## 2. Generated API Naming Convention

All generated code follows AutoSAR 4.x naming standards:

### SenderReceiver Interfaces

| Direction | API Pattern | Example |
|-----------|-------------|---------|
| Provided (Sender) | `Rte_Write_<Swc>_<Port>_<DE>(const <Type>* data)` | `Rte_Write_SwcEngineCtrl_PpEngineSpeed_EngineSpeed` |
| Required (Receiver) | `Rte_Read_<Swc>_<Port>_<DE>(<Type>* data)` | `Rte_Read_SwcDisplay_RpEngineSpeed_EngineSpeed` |
| Provided | `Rte_Send_<Swc>_<Port>(const <Type>* data)` | `Rte_Send_SwcEngineCtrl_PpEngineSpeed` |
| Required | `Rte_Receive_<Swc>_<Port>(<Type>* data)` | `Rte_Receive_SwcDisplay_RpEngineSpeed` |

### NvBlock Interfaces

| Operation | API Pattern | Example |
|-----------|-------------|---------|
| Read | `Rte_Read_<Swc>_<Port>_<DE>(<Type>* data)` | `Rte_Read_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig` |
| Write | `Rte_Write_<Swc>_<Port>_<DE>(const <Type>* data)` | `Rte_Write_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig` |

### ClientServer Interfaces

| Port Role | API Pattern | Example |
|-----------|-------------|---------|
| Client (Required) | `Rte_Call_<Swc>_<Port>_<Op>(<args>)` | `Rte_Call_SwcEngineCtrl_PpDiagServices_ReadDTC` |
| Server (Provided) | `Rte_Server_<Swc>_<Port>_<Op>(<args>)` | `Rte_Server_SwcDisplay_PpDisplayServices_ShowWarning` |
| Server (Provided) | `Rte_Result_<Swc>_<Port>_<Op>(<args>)` | `Rte_Result_SwcDisplay_PpDisplayServices_ShowWarning` |

### ModeSwitch Interfaces

| Operation | API Pattern | Example |
|-----------|-------------|---------|
| Switch | `Rte_Switch_<Swc>_<Port>_<ModeGroup>(uint32 mode)` | `Rte_Switch_SwcEngineCtrl_PpMode_OpMode` |
| Read | `Rte_Mode_<Swc>_<Port>_<ModeGroup>(uint32* mode)` | `Rte_Mode_SwcEngineCtrl_PpMode_OpMode` |

---

## 3. Configuration JSON Schema

### Top-Level Object

```json
{
  "softwareComponents": [ /* array of SWC objects */ ]
}
```

### Software Component Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | SWC name (PascalCase), used in filenames and API prefixes |
| `description` | string | No | Human-readable component description |
| `ports` | array of Port | Yes | List of component ports |

### Port Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Port name (PascalCase with Pp/Rp prefix convention) |
| `direction` | string | Yes | `"Provided"` (P-Port) or `"Required"` (R-Port) |
| `interfaceType` | string | Yes | `"SenderReceiver"`, `"NvBlock"`, `"ClientServer"`, `"ModeSwitch"` |
| `dataElements` | array of DataElement | Conditionally | Required for `SenderReceiver` and `NvBlock` |
| `operations` | array of Operation | Conditionally | Required for `ClientServer` |
| `modeGroup` | string | Conditionally | Required for `ModeSwitch` |

### Data Element Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Data element name (PascalCase) |
| `type` | string or StructType | Yes | C primitive type name or inline struct definition |
| `comSignalId` | number | No | COM signal ID (required for SenderReceiver) |
| `nvmBlockId` | number | No | NVM block ID (required for NvBlock) |

### Struct Type Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `kind` | string | Yes | Must be `"struct"` |
| `fields` | array of Field | Yes | Struct member fields |

### Field Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Field name (camelCase) |
| `type` | string | Yes | C primitive type name |

### Operation Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Operation name (PascalCase) |
| `arguments` | array of Argument | Yes | Operation arguments |

### Argument Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Argument name (camelCase) |
| `type` | string | Yes | C primitive type name |
| `direction` | string | Yes | `"in"` or `"out"` |

---

## 4. Supported Interface Types

### 4.1 SenderReceiver

- Maps data elements to COM signal APIs
- Provided ports generate `Rte_Write` and `Rte_Send` APIs calling `Com_SendSignal`
- Required ports generate `Rte_Read` and `Rte_Receive` APIs calling `Com_ReceiveSignal`
- Each data element gets a static local buffer and an `isUpdated` flag

### 4.2 NvBlock

- Maps data elements to NVM block APIs
- Generates `Rte_Read` calling `NvM_ReadBlock`
- Generates `Rte_Write` calling `NvM_WriteBlock`
- Supports primitive types and inline struct definitions
- Each data element gets a static local buffer and an `isValid` flag

### 4.3 ClientServer

- Required (Client) ports generate `Rte_Call_<Op>` functions with argument marshaling stubs
- Provided (Server) ports generate `Rte_Server_<Op>` runnable skeletons and `Rte_Result_<Op>` async result APIs
- Arguments support `in` (by value) and `out` (by pointer) directions

### 4.4 ModeSwitch

- Generates `Rte_Switch_<Port>_<ModeGroup>` to initiate mode transitions
- Generates `Rte_Mode_<Port>_<ModeGroup>` to query current mode
- Returns `Rte_StatusType` for RTE-level error handling

---

## 5. Generated File Structure

For each SWC named `<SwcName>`, the tool produces:

### `Rte_<SwcName>.h`
- File header with auto-generation warning
- Include files (`Rte.h`, `Rte_Type.h`, service headers as needed)
- Version information macros
- Local type definitions (e.g., inline structs for NvBlock data elements)
- Function prototypes inside `RTE_START_SEC_CODE` / `RTE_STOP_SEC_CODE` MemMap sections
- Convenience macros mapping `RTE_READ/WRITE/SEND/RECEIVE/CALL/RESULT/SWITCH/MODE` to generated functions

### `Rte_<SwcName>.c`
- File header with project metadata
- Include files (including the generated header)
- COM signal ID defines (e.g., `RTE_COMSIGNAL_...`)
- NVM block ID defines (e.g., `RTE_NVMBLOCK_...`)
- Static local buffers for each data element
- Static validity/update flags
- Global function implementations with null-pointer checks
- MemMap section markers for code and variables

---

## 6. Scenarios

### Scenario 1: Generate Engine Control SWC

**Description:** Generate RTE interface code for `SwcEngineCtrl` with sender ports for engine speed and throttle position, an NV block for engine configuration, and a diagnostic client port.

**Configuration:** See `example_config.json` -> `SwcEngineCtrl`

**Generated Files:**
- `Rte_SwcEngineCtrl.h` with prototypes for:
  - `Rte_Write_SwcEngineCtrl_PpEngineSpeed_EngineSpeed`
  - `Rte_Send_SwcEngineCtrl_PpEngineSpeed`
  - `Rte_Write_SwcEngineCtrl_PpThrottlePos_ThrottlePosition`
  - `Rte_Read_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig` (struct type)
  - `Rte_Write_SwcEngineCtrl_PpNvMEngineConfig_EngineConfig` (struct type)
  - `Rte_Call_SwcEngineCtrl_PpDiagServices_ReadDTC`
- `Rte_SwcEngineCtrl.c` with implementations mapping to `Com_SendSignal`, `NvM_ReadBlock`/`NvM_WriteBlock`, and client call stubs.

**Expected Result:** Files compile against existing `Rte_Type.h`, `Com.h`, and `NvM.h`.

### Scenario 2: Generate Display SWC

**Description:** Generate RTE interface code for `SwcDisplay` with receiver ports for engine speed and vehicle speed, and a server port for display warnings.

**Configuration:** See `example_config.json` -> `SwcDisplay`

**Generated Files:**
- `Rte_SwcDisplay.h` with prototypes for:
  - `Rte_Read_SwcDisplay_RpEngineSpeed_EngineSpeed`
  - `Rte_Receive_SwcDisplay_RpEngineSpeed`
  - `Rte_Read_SwcDisplay_RpVehicleSpeed_VehicleSpeed`
  - `Rte_Server_SwcDisplay_PpDisplayServices_ShowWarning`
  - `Rte_Result_SwcDisplay_PpDisplayServices_ShowWarning`
- `Rte_SwcDisplay.c` with implementations mapping to `Com_ReceiveSignal` and server runnable skeletons.

**Expected Result:** Files compile against existing `Rte_Type.h` and `Com.h`.

### Scenario 3: Modify Configuration and Regenerate

**Description:** A developer adds a new data element `EngineTemp` (uint8) to `SwcEngineCtrl.PpEngineSpeed`, updates `example_config.json`, and re-runs the generator.

**Flow:**
1. Edit `example_config.json` to add `{ "name": "EngineTemp", "type": "uint8", "comSignalId": 102 }`
2. Run `python rte_generator.py --config example_config.json --output src/bsw/rte/generated/`
3. Generator overwrites `Rte_SwcEngineCtrl.h` and `Rte_SwcEngineCtrl.c`
4. New `Rte_Write_SwcEngineCtrl_PpEngineSpeed_EngineTemp` and `Rte_Send_SwcEngineCtrl_PpEngineSpeed_EngineTemp` APIs appear

**Expected Result:** Regenerated files reflect the new data element without breaking existing APIs.

### Scenario 4: Struct Type in NvBlock

**Description:** An NvBlock data element uses a structured type rather than a primitive.

**Flow:**
1. Define a `struct` inline in the JSON `type` field
2. Generator emits a `typedef struct { ... } Rte_<Swc>_<Port>_<DE>Type;` in the header
3. Generator uses this type for buffer declarations and API signatures in both header and source

**Expected Result:** Struct type is visible to both SWC implementation code and RTE implementation code.

---

## 7. Integration with Existing RTE Layer

### Header Dependencies

Generated headers include:
- `Rte.h` - Core RTE API and macros
- `Rte_Type.h` - Common RTE types (`Rte_StatusType`, `Std_ReturnType`, etc.)
- `Com.h` - When any SenderReceiver port is present
- `NvM.h` - When any NvBlock port is present

### Source Dependencies

Generated sources include:
- Their own generated header (`Rte_<SwcName>.h`)
- `Com.h` / `NvM.h` as needed for implementation
- `MemMap.h` for section placement

### MemMap Sections

Generated code uses the following MemMap sections:
- `RTE_START_SEC_VAR_CLEARED_UNSPECIFIED` / `RTE_STOP_SEC_VAR_CLEARED_UNSPECIFIED` for static buffers
- `RTE_START_SEC_CODE` / `RTE_STOP_SEC_CODE` for function implementations

### Buffer Management

- Each SenderReceiver data element gets a `STATIC <Type> Rte_Buffer_<Swc>_<Port>_<DE>` and `STATIC boolean Rte_IsUpdated_<Swc>_<Port>_<DE>`
- Each NvBlock data element gets a `STATIC <Type> Rte_Buffer_<Swc>_<Port>_<DE>` and `STATIC boolean Rte_Buffer_<Swc>_<Port>_<DE>_IsValid`
- Buffers are updated on successful write/send operations

### Error Handling

- All generated functions check for `NULL_PTR` before dereferencing data pointers
- SenderReceiver write operations check `Com_SendSignal` return value
- SenderReceiver read operations check `Com_ReceiveSignal` return value against `COM_SERVICE_OK`
- NvBlock operations propagate `NvM_ReadBlock` / `NvM_WriteBlock` return values
- Client/Server stubs return `E_OK` with TODO comments for manual implementation

---

## 8. Command Line Interface

```bash
python tools/rte_generator/rte_generator.py \
    --config <path_to_config.json> \
    --output <output_directory>
```

| Argument | Required | Description |
|----------|----------|-------------|
| `--config` | Yes | Path to JSON configuration file |
| `--output` | Yes | Output directory for generated `.h` and `.c` files |

---

## 9. Constraints and Limitations

- Python 3.x only, standard library only (`json`, `argparse`, `os`, `sys`)
- No support for queued sender-receiver communication (queue length configuration)
- No support for inter-runnable variables (IRV) or per-instance memory (PIM) generation
- Client/Server operations generate synchronous stubs only; async semantics require manual implementation
- ModeSwitch generates placeholder implementations
- Inline struct types are supported for NvBlock data elements only

---

## 10. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-04-24 | YuleTech | Initial RTE Generator specification |
