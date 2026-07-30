# remaining-errors.md — yuleASR v1.3.0 未修复的编译器错误

> 文档记录经本次修复后仍存在的编译器错误。
> 修复 commit: `8323821` on `v1.3.0`

## 概览

本次修复解决了约 60-70% 的编译器错误。
剩余约 40 条错误行（并行编译统计为 46 条），主要分为以下类别：

---

## 1. TcpIp 套接字函数缺失 (架构级)

**涉及文件:** `Mqtt.c`, `Mqtt_Tls.c`, `Mqtt_CertMgr.c`

**错误:**
```
call to undeclared function 'TcpIp_SocketCreate'
call to undeclared function 'TcpIp_SocketClose'
call to undeclared function 'TcpIp_IsConnected'
```

**根本原因:** TcpIp 模块（`src/bsw/services/tcpip/`）存在但不导出这些套接字函数。
Mqtt 模块依赖 TcpIp 的 TCP 连接API，但该 API 尚未实现/声明。

**修复方式:** 需要实现 TcpIp 的 Socket* API 或创建完整的 TcpIp 存根模块。

---

## 2. Mcal 中断控制函数缺失 (架构级)

**涉及文件:** `NvM.c`

**错误:**
```
call to undeclared function 'Mcal_EnableAllInterrupts'
call to undeclared function 'Mcal_DisableAllInterrupts'
```

**根本原因:** MCAL 层未提供中断控制函数。
NvM 模块在临界区操作中调用这些函数。

**修复方式:** 在 `Mcal.h` 中添加中断控制函数声明，或在 MCAL 层实现。

---

## 3. NVM 配置宏缺失

**涉及文件:** `NvM.c`, `NvM_EccHandler.c`

**错误:**
```
use of undeclared identifier 'NVM_CFG_MAX_BLOCK_ID'
```

**根本原因:** `NVM_CFG_MAX_BLOCK_ID` 宏未在 `NvM_Cfg.h` 或配置文件中定义。

**修复方式:** 在 `config/input/services/NvM_Cfg.h` 或 `NvM_Cfg.h` 中添加 `NVM_CFG_MAX_BLOCK_ID` 定义。

---

## 4. SecOC 类型定义缺失

**涉及文件:** `SecOc.c`, `SecOc_Lcfg.c`

**错误:**
```
use of undeclared identifier 'SECOC_MAX_PDUS'
use of undeclared identifier 'SECOC_FRESHNESS_LENGTH_4'
use of undeclared identifier 'SECOC_FRESHNESS_LENGTH_3'
use of undeclared identifier 'SECOC_AUTH_LENGTH_8'
use of undeclared identifier 'SECOC_AUTH_LENGTH_4'
unknown type name 'SecOC_SecurityProfileType'
```

**根本原因:** SecOC 模块缺少配置宏和枚举类型定义。

**修复方式:** 在 SecOC 的配置头文件中添加缺少的宏和类型定义。

---

## 5. Mqtt 循环依赖未完全解决

**涉及文件:** `Mqtt.c`, `Mqtt.h`, `Mqtt_Tls.h`

**错误:**
```
static declaration of 'Mqtt_ConfigPtr' follows non-static declaration
```

**根本原因:** Mqtt.c 中 `Mqtt_ConfigPtr` 声明与 Mqtt.h 中声明冲突。

**修复方式:** 统一 Mqtt_ConfigPtr 的声明方式（static vs extern）。

---

## 6. MemIf 函数签名不匹配

**涉及文件:** `MemIf.c`, `MemIf_Lcfg.c`

**错误:**
```
conflicting types for 'MemIf_Read'
static declaration of 'MemIf_Devices' follows non-static declaration
```

**根本原因:** MemIf.h 的函数声明与 MemIf.c 的实现签名不完全匹配。
`MemIf_Devices` 数组的声明方式不一致。

**修复方式:** 对齐 MemIf.h 和 MemIf.c 的函数签名。

---

## 7. SomeIp 函数签名不匹配

**涉及文件:** `SomeIp.c`

**错误:**
```
conflicting types for 'SomeIp_ExtractIds'
call to undeclared function 'SomeIp_ExtractIds'
```

**根本原因:** SomeIp.h 导出了 `SomeIp_CreateMessageId` 等函数，但 SomeIp.c 中调用的 `SomeIp_ExtractIds` 可能签名不同或被当作宏定义。

**修复方式:** 在 SomeIp.h 中声明 `SomeIp_ExtractIds` 或对齐签名。

---

## 8. NvM 功能函数缺失

**涉及文件:** `NvM_EccHandler.c`

**错误:**
```
call to undeclared function 'NvM_GetRedundantBlockAddress'
```

**根本原因:** NvM 的冗余块地址功能函数未声明。

**修复方式:** 在 NvM.h 中添加函数声明或提供存根。

---

## 相关文件索引

| 模块 | 文件 | 错误类型 |
|------|------|----------|
| TcpIp | `Mqtt.c, Mqtt_Tls.c` | 函数未声明 |
| Mcal | `NvM.c` | 中断控制函数缺失 |
| NvM | `NvM.c, NvM_EccHandler.c` | 配置宏缺失、函数未声明 |
| SecOC | `SecOc.c, SecOc_Lcfg.c` | 类型/宏定义缺失 |
| Mqtt | `Mqtt.c` | 静态声明冲突 |
| MemIf | `MemIf.c, MemIf_Lcfg.c` | 类型/宏定义缺失 |
| SomeIp | `SomeIp.c` | 函数签名不匹配 |

---

## 下次修复建议

1. **TcpIp 套接字存根** — 为 `TcpIp_SocketCreate/Close/IsConnected` 创建存根实现
2. **MCAL 中断控制** — 添加 `Mcal_EnableAllInterrupts/DisableAllInterrupts` 声明
3. **NvM 配置** — 定义 `NVM_CFG_MAX_BLOCK_ID`
4. **SecOC 类型** — 补充 SecOC 配置头和类型定义
5. **MemIf/SomeIp 对齐** — 对齐头文件声明与实现签名
