/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Csm_MemMap.h
 * @brief CSM模块内存映射头文件
 * 
 * 定义代码段和数据段的内存映射
 * 
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef CSM_MEMMAP_H
#define CSM_MEMMAP_H

/*==================================================================================================
*                                       内存映射定义
==================================================================================================*/

#ifdef CSM_START_SEC_CODE
    #undef CSM_START_SEC_CODE
    #pragma section ".text.Csm" ax
#endif

#ifdef CSM_STOP_SEC_CODE
    #undef CSM_STOP_SEC_CODE
    #pragma section
#endif

#ifdef CSM_START_SEC_VAR_INIT_UNSPECIFIED
    #undef CSM_START_SEC_VAR_INIT_UNSPECIFIED
    #pragma section ".data.Csm" aw
#endif

#ifdef CSM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef CSM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #pragma section
#endif

#ifdef CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section ".bss.Csm" aw
#endif

#ifdef CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section
#endif

#ifdef CSM_START_SEC_CONST_UNSPECIFIED
    #undef CSM_START_SEC_CONST_UNSPECIFIED
    #pragma section ".rodata.Csm" a
#endif

#ifdef CSM_STOP_SEC_CONST_UNSPECIFIED
    #undef CSM_STOP_SEC_CONST_UNSPECIFIED
    #pragma section
#endif

#ifdef CSM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef CSM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section ".rodata.Csm.Config" a
#endif

#ifdef CSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef CSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section
#endif

#endif /* CSM_MEMMAP_H */
