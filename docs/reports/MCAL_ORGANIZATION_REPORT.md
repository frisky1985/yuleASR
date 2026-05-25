# yuleASR MCAL Layer Organization Report

## Summary
Successfully organized MCAL (Microcontroller Driver Layer) modules from `src/bsw/mcal/` to `src/autosar/mcal/` following AUTOSAR standard directory structure.

## Date
$(date)

## Directory Structure
Each module follows the standard AUTOSAR structure:
```
src/autosar/mcal/<module>/
├── include/
│   ├── <Module>.h          (API header file)
│   ├── <Module>_Cfg.h      (Configuration header)
│   └── <Module>_MemMap.h   (Memory mapping, if applicable)
└── src/
    ├── <Module>.c          (Main implementation)
    ├── <Module>_Irq.c      (Interrupt handlers, if applicable)
    └── <Module>_Lcfg.c     (Link-time configuration, if applicable)
```

## Organized Modules (21 total)

| Module | Headers | Sources | Notes |
|--------|---------|---------|-------|
| adc    | 2       | 1       | - |
| can    | 2       | 1       | - |
| crypto | 7       | 7       | Includes hardware-specific implementations |
| dio    | 2       | 1       | - |
| eep    | 2       | 2       | Includes Eep_Lcfg.c |
| eth    | 4       | 2       | Includes Eth_Irq.c |
| fee    | 2       | 2       | Includes Fee_Lcfg.c |
| flash  | 3       | 2       | Includes Flash_MemMap.h, Flash_Lcfg.c |
| fls    | 4       | 2       | Includes Fls_Hw.c, Fls_MemMap.h |
| gpt    | 2       | 1       | - |
| i2c    | 2       | 1       | - |
| icu    | 4       | 3       | Includes Icu_Irq.c, Icu_Lcfg.c |
| lin    | 19      | 13      | Master/Slave implementation with TP, UDS support |
| mcu    | 2       | 1       | - |
| ocu    | 4       | 2       | Includes Ocu_Irq.c |
| port   | 2       | 1       | - |
| pwm    | 2       | 1       | - |
| ramtst | 2       | 2       | Includes RamTst_Lcfg.c |
| spi    | 2       | 1       | Merged from spi/ and Spi/ directories |
| uart   | 2       | 1       | - |
| wdg    | 3       | 2       | Includes Wdg_Hw.c |

## File Count Summary
- **Total Modules**: 21
- **Total Header Files**: 74
- **Total Source Files**: 49
- **Grand Total**: 123 files

## Source to Destination Mapping

### Files copied from `src/bsw/mcal/` to `src/autosar/mcal/`:

1. **adc/**: Adc.h, Adc_Cfg.h, Adc.c
2. **can/**: Can.h, Can_Cfg.h, Can.c
3. **crypto/**: Crypto.h, Crypto_Cfg.h, Crypto_MemMap.h, Crypto_Types.h, Crypto_HwTrng.h, Crypto_S32K312_Hsm.h, SchM_Crypto.h, Crypto.c, Crypto_Cfg.c, Crypto_Aes.c, Crypto_Hsm.c, Crypto_HwTrng.c, Crypto_MbedTLS.c, Crypto_S32K312_Hsm.c
4. **dio/**: Dio.h, Dio_Cfg.h, Dio.c
5. **eep/**: Eep.h, Eep_Cfg.h, Eep.c, Eep_Lcfg.c
6. **eth/**: Eth.h, Eth_Cfg.h, Eth_Lcfg.h, Eth_Private.h, Eth.c, Eth_Irq.c
7. **fee/**: Fee.h, Fee_Cfg.h, Fee.c, Fee_Lcfg.c
8. **flash/**: Flash.h, Flash_Cfg.h, Flash_MemMap.h, Flash.c, Flash_Lcfg.c
9. **fls/**: Fls.h, Fls_Cfg.h, Fls_Hw.h, Fls_MemMap.h, Fls.c, Fls_Hw.c
10. **gpt/**: Gpt.h, Gpt_Cfg.h, Gpt.c
11. **I2c/** → **i2c/**: I2c.h, I2c_Cfg.h, I2c.c
12. **icu/**: Icu.h, Icu_Cfg.h, Icu_Lcfg.h, Icu_Private.h, Icu.c, Icu_Irq.c, Icu_Lcfg.c
13. **lin/**: Lin.h, Lin_Cfg.h, LinMaster.h, LinMaster_Cfg.h, LinMaster_Diagnostic.h, LinMaster_Hal.h, LinMaster_Schedule.h, LinMaster_Tp.h, LinMaster_Types.h, LinSlave.h, LinSlave_Cfg.h, LinSlave_CfgTable.h, LinSlave_Checksum.h, LinSlave_Hal.h, LinSlave_Pid.h, LinSlave_Tp.h, LinSlave_Types.h, LinSlave_Uds.h, Std_Types.h, Lin.c, LinMaster.c, LinMaster_Diagnostic.c, LinMaster_Hal.c, LinMaster_Schedule.c, LinMaster_Tp.c, LinSlave.c, LinSlave_CfgTable.c, LinSlave_Checksum.c, LinSlave_Hal.c, LinSlave_Pid.c, LinSlave_Tp.c, LinSlave_Uds.c
14. **mcu/**: Mcu.h, Mcu_Cfg.h, Mcu.c
15. **ocu/**: Ocu.h, Ocu_Cfg.h, Ocu_Lcfg.h, Ocu_Private.h, Ocu.c, Ocu_Irq.c
16. **port/**: Port.h, Port_Cfg.h, Port.c
17. **pwm/**: Pwm.h, Pwm_Cfg.h, Pwm.c
18. **ramtst/**: RamTst.h, RamTst_Cfg.h, RamTst.c, RamTst_Lcfg.c
19. **spi/ + Spi/** → **spi/**: Spi.h, Spi_Cfg.h, Spi.c
20. **Uart/** → **uart/**: Uart.h, Uart_Cfg.h, Uart.c
21. **wdg/**: Wdg.h, Wdg_Cfg.h, Wdg_Hw.h, Wdg.c, Wdg_Hw.c

## Notes
- All file contents preserved (byte-for-byte copy)
- No files were overwritten (all target files were empty/non-existent)
- Case normalization applied: I2c→i2c, Uart→uart
- Duplicate directories merged: spi/ and Spi/ combined into spi/
- LIN module is the most complex with Master/Slave implementation including TP and UDS support
