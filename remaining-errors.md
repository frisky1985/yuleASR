# remaining-errors.md — yuleASR v1.3.0 编译错误跟踪

> 本文件记录 yuleASR v1.3.0 分支编译错误的修复进度。
> 修复 commit: `8323821`、`cb28c1e`（前两轮）+ 本轮（见 git log）。

## 状态：✅ 已全部修复（编译全绿）

`cmake --build build -j4` 与 `make -C build -j1 -k` 均 **0 error**，全部目标 100% 构建成功。

### 本轮修复统计

- 修复前（本轮起点）：563 条 error 行 / 474 条唯一错误
- 修复后：**0 条 error**

### 修复明细（按类别）

| 类别 | 修复内容 |
|------|----------|
| CMake 包含路径 | mcal/ecual/services/boot/platform/os 各层补齐缺失 include dir（platform s32k312、mcal can/spi/fls/lin/uart、third_party mbedtls/aes_modes/hash、include/autosar、config/input 等） |
| 新增存根头文件 | FreeRTOSConfig.h / projdefs.h / portmacro.h、Reg_Macros.h、Lockstep.h、PduR_DoIP.h、PduR_LinTp.h、Eth_GeneralTypes.h、SchM_DoIP/Fee/SecOC/Uart.h、EthTrcv_MemMap.h、Fee_MemMap.h、FiM_MemMap.h、RamSafety_MemMap.h、Platform_Lockstep_MemMap.h、mbedtls/*.h（bignum/ecp/ecdsa/ecdh/aes/gcm/sha256/md/hkdf/ctr_drbg/entropy/x509/x509_crt/oid/asn1/ssl/pk/platform/debug）、CryptoStack_Types.h、Dma.h |
| 删除遮蔽性 stub | include/autosar 下 SomeIp.h / SomeIpTp.h / SomeIpXf.h / LinTp.h / RamSafety.h 与模块头文件冲突，删除让模块头生效 |
| 共享类型补全 | ComStack_Types.h（NetworkHandleType、TP_STMIN/BS/BC、BUFREQ_E_OVFL、RetryInfoType 扩展）、Eth_GeneralTypes.h、Crypto_Types.h（消除 typedef 重定义、补 ConfigType 字段） |
| 截断文件补全 | NvM.c / StbM.c / SecOc.c / SomeIpTp.c / SomeIpXf.c 被先前提交截断（缺函数尾部），已按 API 语义补全 |
| ARM asm 兼容 | __asm("dsb")/cpsid/msr msp 等在 aarch64 宿主上改用条件编译（dsb sy / daifset 等） |
| 各模块代码修复 | Mqtt（TcpIp_Socket* 包装、CheckTimeout、枚举/宏冲突、TLS 配置前向声明）、MemIf（签名对齐）、SecOC（配置头宏+Lcfg 重写）、NvM（NVM_CFG_MAX_BLOCK_ID、GetBlockAddress/GetRedundantBlockAddress）、SomeIp/SomeIpTp/SomeIpXf、CanNm/LinNm（NetworkHandleType、ComM_ECNM_*）、CanTp/FrTp、CanTrcv（配置类型重写）、Icu（类型+配置重写）、Fee/Fls/Flash（配置宏+类型）、EthSM/EthIf、DoIP（SoAd 接口）、Xcp（配置类型）、StbM、RamSafety、Platform、RTE/ASW（Rte_Read/Write/端口 API、Rte_GetTime 实现、ABS/memset 等） |

### 遗留说明（非编译阻塞）

1. **集成测试** `tests/integration/` 失败：`test_evidence_pipeline.py` 等依赖 `.yuleosh/evidence-bundle` 证据文件（CI 流水线产物），本地未生成，与本轮编译修复无关。单元测试 `tests/unit/test_yuleasr_monitor.py` 6/6 通过。
2. **平台适配 TODO**（已在代码中标注）：
   - mbedtls 为声明级 stub，运行时需链接真实 mbedTLS 库；
   - Mcal_EnableAllInterrupts/DisableAllInterrupts/ResetSystem、TcpIp socket 包装、Uart/Dma 时间源、Platform_Fccu_NonFaultyFault、Lockstep_* 等为宿主可用实现/占位，量产需接 MCAL/OS 真实实现；
   - Flash 寄存器定义为 STM32F4 风格默认值，需按 S32K312 实际寄存器基址调整。
3. **仓库大小写冲突**：repo 存在 DoIP/DoIp、SecOC/SecOc、CanNm/canNm 等大小写重复路径，在大小写不敏感文件系统（macOS APFS）上工作树表现为"已修改"，实际内容与 HEAD 一致（详见本轮工作区审查结论）。
