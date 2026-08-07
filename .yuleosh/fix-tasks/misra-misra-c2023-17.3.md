# MISRA Fix Task: misra-c2023-17.3

> Generated: 2026-08-07T14:30:06.016661
> Severity: required
> Spec Ref: SWE-MISRA-S1

## Rule: Functions shall not call themselves indirectly

函数不应间接递归调用自身

## Violations

| # | File | Line | Col | Message |
|--:|:-----|:----|:----|:--------|
| 1 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/classic/rte_dds.c` | 178 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 2 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 158 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 3 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 479 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 4 | `/Users/stefan/.openclaw/workspace/yuleASR/src/autosar/adaptive/ara_com_dds.c` | 562 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 5 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/can/src/Can.c` | 125 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 6 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/can/src/Can.c` | 135 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 7 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/pwm/src/Pwm.c` | 131 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 8 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/gpt/src/Gpt.c` | 158 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 9 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/i2c/src/I2c.c` | 217 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 10 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/i2c/src/I2c.c` | 233 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 11 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/i2c/src/I2c.c` | 291 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 12 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/i2c/src/I2c.c` | 310 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 13 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/i2c/src/I2c.c` | 328 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 14 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/i2c/src/I2c.c` | 377 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 15 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/i2c/src/I2c.c` | 1191 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 16 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/spi/src/Spi.c` | 299 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 17 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/spi/src/Spi.c` | 311 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 18 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/spi/src/Spi.c` | 462 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 19 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/spi/src/Spi.c` | 476 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 20 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/adc/src/Adc.c` | 146 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 21 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/adc/src/Adc.c` | 227 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 22 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/mcal/mcu/src/Mcu.c` | 128 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 23 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 162 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 24 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/boot/test/test_boot_integration.c` | 169 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 25 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/dcm/src/dcm_transfer.c` | 652 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 26 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/dcm/src/dcm_transfer.c` | 851 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 27 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_write_impl.c` | 64 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 28 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_write_impl.c` | 129 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 29 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_write_impl.c` | 571 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 30 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_write_impl.c` | 629 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 31 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_read_impl.c` | 45 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 32 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_main_bulk_impl.c` | 99 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 33 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_main_bulk_impl.c` | 101 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 34 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_main_bulk_impl.c` | 125 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 35 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_main_bulk_impl.c` | 127 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 36 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_main_bulk_impl.c` | 477 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 37 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/nvm/src/_nvm_main_bulk_impl.c` | 526 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 38 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 26 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 39 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 33 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 40 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 63 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 41 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 110 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 42 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 121 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 43 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 172 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 44 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 205 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 45 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 244 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 46 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 275 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 47 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_keys_impl.c` | 354 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 48 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_key_exch_secret_impl.c` | 26 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 49 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_key_exch_secret_impl.c` | 85 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 50 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_key_ops_impl.c` | 20 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 51 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_key_ops_impl.c` | 34 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 52 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_key_ops_impl.c` | 76 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 53 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_key_ops_impl.c` | 324 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 54 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_key_ops_impl.c` | 383 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 55 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 21 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 56 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 100 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 57 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 106 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 58 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 179 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 59 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 185 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 60 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 232 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 61 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 238 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 62 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 308 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 63 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 314 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 64 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 384 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 65 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 390 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 66 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 461 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 67 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 467 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 68 | `/Users/stefan/.openclaw/workspace/yuleASR/src/bsw/services/csm/src/_csm_crypto_ops_impl.c` | 537 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 69 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 298 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |
| 70 | `/Users/stefan/.openclaw/workspace/yuleASR/src/common/log/dds_log.c` | 994 | 0 | A function shall not be declared implicitly [misra-c2012-17.3] |

## Fix Checklist

- [ ] Understand the violation context
- [ ] Apply fix to source code
- [ ] Re-run MISRA check to verify fix
- [ ] Update traceability matrix
- [ ] Document deviation if fix is not feasible

---
*Generated by yuleOSH MISRA fix-task generator*