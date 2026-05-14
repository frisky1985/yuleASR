/**
 * @file Eep_Cfg.h
 * @brief EEPROM Driver Configuration
 */

#ifndef EEP_CFG_H
#define EEP_CFG_H

#define EEP_DEV_ERROR_DETECT        STD_ON
#define EEP_VERSION_INFO_API        STD_ON
#define EEP_CANCEL_API              STD_ON

/* EEPROM Configuration */
#define EEP_BASE_ADDRESS            0x08080000U
#define EEP_SIZE                    0x00010000U  /* 64KB */
#define EEP_PAGE_SIZE               8U
#define EEP_WRITE_CYCLE_TIME        10U  /* ms */
#define EEP_ERASE_CYCLE_TIME        20U  /* ms */

#define EEP_JOB_CALL_CYCLE          10U  /* ms */

#endif
