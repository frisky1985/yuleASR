# dev-rte-generator Tasks

> **Change:** RTE Generator Tool  
> **Status:** In Progress  
> **Created:** 2026-04-24

---

## Task List

### Phase 1: Tool Development

- [x] Read existing RTE reference files (Rte_Type.h, Rte_Swc.h, Rte.h, Rte.c)
- [x] Read PduR_spec.md for specification format reference
- [x] Create `tools/rte_generator/rte_generator.py` - Python generator script
- [x] Create `tools/rte_generator/example_config.json` - Example SWC configuration
- [x] Create `tools/rte_generator/README.md` - Tool documentation

### Phase 2: Verification

- [x] Run generator with example config
- [x] Verify generated `Rte_SwcEngineCtrl.h` format and content
- [x] Verify generated `Rte_SwcEngineCtrl.c` format and content
- [x] Verify generated `Rte_SwcDisplay.h` format and content
- [x] Verify generated `Rte_SwcDisplay.c` format and content
- [x] Confirm struct types are defined in headers and referenced correctly
- [x] Confirm source files include their own generated headers

### Phase 3: Specification

- [x] Create `openspec/changes/dev-rte-generator/specs/RteGenerator_spec.md`
- [x] Include tool overview and responsibilities
- [x] Document generated API naming convention
- [x] Document JSON configuration schema
- [x] Document supported interface types
- [x] Document generated file structure
- [x] Write usage scenarios (at least 4)
- [x] Document integration with existing RTE layer

### Phase 4: Git Workflow

- [ ] Stage all changes (`git add -A`)
- [ ] Commit with descriptive message
- [ ] Push to origin/master (resolve conflicts if needed)

---

## Notes

- Generator uses only Python standard library (json, argparse, os, sys)
- Generated code follows AutoSAR 4.x naming conventions
- Compatible with existing Rte_Type.h types (uint16, uint8, Std_ReturnType, etc.)
- COM interfaces map to Com_SendSignal / Com_ReceiveSignal
- NVM interfaces map to NvM_ReadBlock / NvM_WriteBlock
- Client/Server stubs return E_OK with TODO placeholders
