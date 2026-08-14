/** @file mcal_stubs.h — Missing MCAL defines for coverage testing
 *  NOTE: This is -include'd before any headers, so no external types available.
 *  Use only basic C types (unsigned char, unsigned int).
 */
#ifndef MCAL_STUBS_H
#define MCAL_STUBS_H

/* Missing SPI defines */
#ifndef SPI_INSTANCE_ID
#define SPI_INSTANCE_ID 0U
#endif

/* Missing Port defines */
#ifndef PORT_E_ALREADY_INITIALIZED
#define PORT_E_ALREADY_INITIALIZED 0x11U
#endif

/* PduR ConfigType forward declaration (uint8 not available yet) */
#ifndef PduR_ConfigType
struct PduR_ConfigType_s { unsigned char dummy; };
typedef struct PduR_ConfigType_s PduR_ConfigType;
#endif

/* Enable optional APIs - use raw 1 for STD_ON (defined later) */
#ifndef GPT_WAKEUP_FUNCTIONALITY_API
#define GPT_WAKEUP_FUNCTIONALITY_API 1
#endif
#ifndef ICU_WAKEUP_FUNCTIONALITY_API
#define ICU_WAKEUP_FUNCTIONALITY_API 1
#endif

#endif /* MCAL_STUBS_H */
