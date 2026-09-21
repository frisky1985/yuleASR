/**
 * @file Com_Cfg.h
 * @brief TEST-PRIVATE shadow of the production COM pre-compile configuration.
 *
 * WHY THIS FILE EXISTS (test-side workaround, production code untouched):
 *   Com_Init() in src/bsw/services/com/src/Com.c initializes signal states
 *   with
 *       for (i = 0U; i < COM_NUM_OF_SIGNALS; i++)   using a `uint8 i;`
 *   loop counter. The production Com_Cfg.h sets COM_NUM_OF_SIGNALS to 256U,
 *   so the uint8 counter wraps from 255 back to 0 and the loop NEVER
 *   TERMINATES -- a genuine production defect that hangs any process calling
 *   Com_Init().
 *
 *   Per project constraints src/ must not be modified, therefore com_test
 *   compiles the UNMODIFIED production Com.c / Com_Lcfg.c against this
 *   test-private configuration where COM_NUM_OF_SIGNALS (and the matching
 *   COM_MAX_SIGNALS table bound) is 255U, the largest value a uint8 loop
 *   counter can reach. All other parameters are identical to production.
 *
 *   This header deliberately reuses the production include guard COM_CFG_H:
 *   it is force-included (compiler -include option) into every com_test
 *   translation unit BEFORE any production header, so the production
 *   src/bsw/services/com/include/Com_Cfg.h (pulled in via Com.h, same
 *   directory quote-include) becomes a no-op. All TUs of com_test therefore
 *   see exactly one consistent COM configuration.
 *
 * @note AUTOSAR pre-compile time configuration differences are a legitimate
 *       test dimension; the SUT C code under test is byte-identical to
 *       production.
 */

#ifndef COM_CFG_H
#define COM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
*================================================================================================*/

/*==================================================================================================
*                                    General Configuration
*================================================================================================*/
#define COM_DEV_ERROR_DETECT    STD_ON
#define COM_VERSION_INFO_API    STD_ON

/*==================================================================================================
*                                    Module Configuration Counts
*================================================================================================*/
#define COM_NUM_OF_IPDUS    (64U)
/* TEST SHADOW VALUE: production uses (256U) which overflows the uint8 loop
 * counter in Com_Init(); 255U is the maximum safe value for a uint8 index. */
#define COM_NUM_OF_SIGNALS    (255U)
#define COM_NUM_OF_IPDU_GROUPS    (16U)
#define COM_NUM_IPDU_GROUPS    COM_NUM_OF_IPDU_GROUPS
#define COM_NUM_OF_SIGNAL_GROUPS    (16U)

/*==================================================================================================
*                                    Hardware Object Handles
*================================================================================================*/
#define COM_TX_MODE_DIRECT    (0U)
#define COM_TX_MODE_PERIODIC    (1U)
#define COM_TX_MODE_MIXED    (2U)

/*==================================================================================================
*                                    Other Configuration
*================================================================================================*/
/* TEST SHADOW VALUE: kept equal to COM_NUM_OF_SIGNALS, same reason as above. */
#define COM_MAX_SIGNALS    (255U)
#define COM_MAX_IPDUS    (64U)
#define COM_MAX_GROUPS    (16U)
#define COM_MAX_SIGNAL_LENGTH    (64U)
#define COM_LITTLE_ENDIAN    (0U)
#define COM_BIG_ENDIAN    (1U)
#define COM_MAX_IPDU_BUFFER_SIZE    (128U)
#define COM_MAX_IPDU_LENGTH    (64U)

#endif /* COM_CFG_H */

/*==================[end of file]===========================================*/
