/**
 * @file EcuM_Cfg.h
 * @brief ECU State Manager Configuration
 */

#ifndef ECUM_CFG_H
#define ECUM_CFG_H

/* Configuration */
#define ECUM_DEV_ERROR_DETECT               STD_ON
#define ECUM_VERSION_INFO_API               STD_ON
#define ECUM_MAIN_FUNCTION_PERIOD           10  /* ms */

/* Number of users */
#define ECUM_MAX_USERS                      32

/* Wakeup sources */
#define ECUM_WKSOURCE_POWER                 0x00000001
#define ECUM_WKSOURCE_RESET                 0x00000002
#define ECUM_WKSOURCE_INTERNAL_RESET        0x00000004
#define ECUM_WKSOURCE_INTERNAL_WDG          0x00000008
#define ECUM_WKSOURCE_EXTERNAL_WDG          0x00000010

#endif /* ECUM_CFG_H */
