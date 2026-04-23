# DoIp and DoCan Module Development Tasks

> **Change:** dev-doip-docan-module  
> **Date:** 2026-04-24  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## Task List

- [x] Create OpenSpec change directory `openspec/changes/dev-doip-docan-module/`
- [x] Write DoIp module specification (`specs/DoIp_spec.md`)
- [x] Write DoCan module specification (`specs/DoCan_spec.md`)
- [x] Implement DoIp public header (`src/bsw/services/doip/include/DoIp.h`)
- [x] Implement DoIp config header (`src/bsw/services/doip/include/DoIp_Cfg.h`)
- [x] Implement DoIp core source (`src/bsw/services/doip/src/DoIp.c`)
- [x] Implement DoIp link-time config (`src/bsw/services/doip/src/DoIp_Lcfg.c`)
- [x] Implement DoCan public header (`src/bsw/services/docan/include/DoCan.h`)
- [x] Implement DoCan config header (`src/bsw/services/docan/include/DoCan_Cfg.h`)
- [x] Implement DoCan core source (`src/bsw/services/docan/src/DoCan.c`)
- [x] Implement DoCan link-time config (`src/bsw/services/docan/src/DoCan_Lcfg.c`)
- [x] Update MemMap.h with DOIP and DOCAN memory partitions
- [x] Create DoIp unit tests (`src/bsw/services/doip/src/DoIp_test.c`)
- [x] Create DoCan unit tests (`src/bsw/services/docan/src/DoCan_test.c`)
- [ ] Compile and run unit tests (pending CI)
- [ ] Archive change to `openspec/archived/` (post-review)

---

## Design Decisions

1. **DoIp Protocol Version:** Implemented DoIP 2.x (Protocol Version 0x02) as specified in ISO 13400-2.
2. **Connection Management:** Supports up to 4 simultaneous connections with configurable inactivity and alive-check timeouts.
3. **Routing Activation:** Supports default (0x00), WWH-OBD (0x01), and Central Security (0xE0) activation types.
4. **DoCan PDU Mapping:** Clean bidirectional mapping between DCM PDU IDs and CanTp N-SDU IDs for both physical and functional addressing.
5. **DET Integration:** Both modules report development errors via `Det_ReportError` when `DEV_ERROR_DETECT` is enabled.
6. **MemMap Integration:** All code, config, and variable sections use project-standard MemMap.h macros.

---

## Testing Summary

### DoIp Tests
- `test_doip_init_valid_config` - Verify successful initialization
- `test_doip_init_null_config` - Verify DET error on NULL config
- `test_doip_activate_routing_success` - Verify routing activation logic
- `test_doip_iftransmit_encapsulates_header` - Verify DoIP generic header construction
- `test_doip_ifrxindication_routes_to_dcm` - Verify RX parsing and DCM routing
- `test_doip_iftransmit_uninit` - Verify uninit DET error
- `test_doip_close_connection_success` - Verify connection close
- `test_doip_soadtxconfirmation_routes_to_dcm` - Verify TX confirmation routing
- `test_doip_getversioninfo` - Verify version info API

### DoCan Tests
- `test_docan_init_valid_config` - Verify successful initialization
- `test_docan_init_null_config` - Verify DET error on NULL config
- `test_docan_transmit_calls_cantp` - Verify TX forwarding to CanTp
- `test_docan_rxindication_routes_to_dcm` - Verify RX forwarding to DCM
- `test_docan_txconfirmation_notifies_dcm` - Verify TX confirmation to DCM
- `test_docan_transmit_uninit` - Verify uninit DET error
- `test_docan_getversioninfo` - Verify version info API
- `test_docan_transmit_functional_addressing` - Verify functional addressing path
- `test_docan_rxindication_uninit` - Verify uninit DET error on RX

---

## Files Changed

| File | Status |
|------|--------|
| `openspec/changes/dev-doip-docan-module/specs/DoIp_spec.md` | Added |
| `openspec/changes/dev-doip-docan-module/specs/DoCan_spec.md` | Added |
| `openspec/changes/dev-doip-docan-module/tasks.md` | Added |
| `src/bsw/services/doip/include/DoIp.h` | Added |
| `src/bsw/services/doip/include/DoIp_Cfg.h` | Added |
| `src/bsw/services/doip/src/DoIp.c` | Added |
| `src/bsw/services/doip/src/DoIp_Lcfg.c` | Added |
| `src/bsw/services/doip/src/DoIp_test.c` | Added |
| `src/bsw/services/docan/include/DoCan.h` | Added |
| `src/bsw/services/docan/include/DoCan_Cfg.h` | Added |
| `src/bsw/services/docan/src/DoCan.c` | Added |
| `src/bsw/services/docan/src/DoCan_Lcfg.c` | Added |
| `src/bsw/services/docan/src/DoCan_test.c` | Added |
| `src/bsw/general/inc/MemMap.h` | Modified (appended DOIP/DOCAN macros) |
