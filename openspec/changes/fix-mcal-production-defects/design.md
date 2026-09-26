# 设计文档：fix-mcal-production-defects

## 决策记录

### AD1: Crypto GetVersionInfo SID = 0x02

AUTOSAR SWS Crypto R22-11 服务 ID 表：`Init=0x00, DeInit=0x01, GetVersionInfo=0x02, ProcessJob=0x03`。现有 `Crypto.h` 缺少 `CRYPTO_SID_GETVERSIONINFO` 定义，导致 `Crypto.c` 误用 `CRYPTO_SID_DEINIT`。补定义并替换使用点。SID 值与 AUTOSAR 标准一致，非自定义。

### AD2: Mcu 哨兵值选择

- `Mcu_ClockType` 为 `uint32`（`Mcu.h:86`）
- 合法配置索引受 `Mcu_InitClock` 的 `ClockSetting >= NumClockConfigs` 校验约束，实际配置数 ≤ 个位数
- 哨兵取 `0xFFFFFFFFU`（`MCU_CLOCK_INVALID`），与任何合法索引无交集
- 备选方案：增加独立 `boolean clockConfigured` 标志。否决——改动面更大且 `currentClock` 本身即状态载体，哨兵更内聚

### AD3: Spi 分频循环修复方式

问题根因：`for (preDiv = 0; cond && !found; preDiv++)` 中，内层 `break` 后外层仍执行 `preDiv++` 再退出，导致写寄存器的 `preDiv` = 匹配值 + 1。

修复采用独立捕获变量（非 `preDiv--` 事后补偿）：

```c
uint32 bestPre  = 0U;
uint32 bestPost = 0U;
boolean found   = FALSE;
for (preDiv = 0; (preDiv < 16U) && (found == FALSE); preDiv++) {
    for (postDiv = 0; postDiv < 16U; postDiv++) {
        if (((1UL << preDiv) * (postDiv + 1UL)) >= tempDiv) {
            bestPre = preDiv; bestPost = postDiv; found = TRUE; break;
        }
    }
}
if (found == FALSE) { bestPre = 15U; bestPost = 15U; }  /* 回退最大分频 */
```

理由：事后 `--` 依赖循环副作用语义，可读性差且易再次引入 off-by-one；捕获变量语义直白。`1u` 改 `1UL` 消除 MISRA 整数提升隐患。无匹配组合（tempDiv > 2^15×16 理论极值）时回退最大分频，行为确定。

### AD4: Spi GetVersionInfo SID

`Spi_Cfg.h:26` 已定义 `SPI_SERVICE_ID_GETVERSIONINFO (9U)`，Spi.c 改用该宏替换字面量 `0x02U`。

## P0-4: ECUAL 决策记录 (2026-09-25)

### AD5: LinIf 状态激活修复

`LinIf_Init` 末尾仅写 `configPtr`，从未将状态机从 `LINIF_UNINIT` 翻转到 `LINIF_INIT`，导致所有受 E_UNINIT 守卫的 API 在合法初始化后仍全部拒绝——整车 LIN 通信在 SOP 台架首测即暴露。修复为 Init 末尾同时写入 `configPtr` 与 `state = LINIF_INIT`，并删除过期的 "[MISRA Advisory] Redundant" 注释（该注释为缺陷的成因注脚）。测试 `Init_ValidConfig_ShouldActivateModule` 以 `LinIf_Transmit` 返回 E_OK 直接证明激活。

### AD6: LinIf_Config 链接可见性

`LinIf_Lcfg.c` 原将配置对象声明为 `static`，测试与集成代码无法引用。AUTOSAR 链接时配置（LCFG）契约要求配置对象外部可见。改为 `const LinIf_ConfigType LinIf_Config`（去 static），并在 `LinIf.h` 配置 typedef 之后增加 `extern` 声明，使包含头文件者即可链接。

### AD7: CanIf.h 头文件自洽与回调原型

CanIf 作为 CAN 驱动（Can）与上层（PduR/CanSM）之间的双向接口，其头文件缺失驱动侧入口回调的原型声明：`CanIf_TxConfirmation`、`CanIf_ControllerBusOff`、`CanIf_RxIndication`。同时 `CanIf.h` 未包含 `Can.h`，而 `CanIf_RxIndication` 原型需要 `Can_HwType`，任何包含 CanIf.h 的 TU 若未提前包含 Can.h 即编译失败。修复：CanIf.h 增加 `#include "Can.h"` 与三个原型。选择改头文件而非各 TU 前置包含——自洽头是 AUTOSAR 模块头文件的最低契约。

### AD8: CanIf 测试配置策略

CanIf 测试未链接生产 `CanIf_Lcfg.c`，原因：该文件使用已废弃的类型名（`CanIf_HohCfgType`、`CanIf_TxPduCfgType` 等，与当前 `CanIf_Cfg.h` 的 `CanIf_TxPduConfigType` 不一致）且未定义头文件 `extern` 的 `CanIf_Config` 对象，本身已不可编译。测试改用测试内静态有效配置（1 控制器 + 4 TxPdu + 4 RxPdu，字段数与 `CanIf_*ConfigType` 精确对齐）。生产 Lcfg.c 的重建移交配置生成器任务，已记入提案"不包含内容"。

## 测试修正策略

### P0-1 (MCAL)

| 测试 | 变更 |
|:-----|:-----|
| `test_crypto.c` GetVersionInfo_NullPtr | 断言 `CRYPTO_SID_GETVERSIONINFO`，删除 Quirk 注释 |
| `test_mcu.c` Setup | 删除 `testClocks[1]` 复制 hack，`NumClockConfigs 2→1` |
| `test_mcu.c` DistributePllClock_AfterInitClock | `Mcu_InitClock(1U)→(0U)`，成为索引 0 回归用例 |
| `test_mcu.c` DistributePllClock_NoClockSelected | 行为不变（未配置时钟 → NOT_LOCKED），注释改写为正确语义 |
| `test_spi.c` Init_ValidConfig PERIODREG | 0x94 → 0x93，注释改写为黄金值推导 |
| `test_spi.c` Init_ValidConfig | 新增 ECSPI2 (8Mbit/s) PERIODREG = 0x90 断言 |
| `test_spi.c` GetVersionInfo_NullPtr | 断言 `SPI_SERVICE_ID_GETVERSIONINFO`，删除 Quirk 注释 |

### P0-4 (ECUAL 实化)

| 套件 | 策略 |
|:-----|:-----|
| `test_cantp.c`（14 用例） | 桩 `CanIf_Transmit`（捕获 PDU id + 帧字节）、`PduR_RxIndication`/`PduR_TxConfirmation`（计数）、`Det_ReportError`（计数）；链接生产 `CanTp_Lcfg.c`；关键用例：SF 黄金路径、N_Bs 超时复位通道、双 Init 配置替换 |
| `test_linif.c`（13 用例） | 链接生产 `LinIf.c` + `LinIf_Lcfg.c`；关键用例：`Init_ValidConfig` 断言 Transmit E_OK（证明 AD5 激活修复）、版本黄金值（0x0055/0x27）、DeInit 去激活 |
| `test_canif.c`（17 用例） | 桩 `Can_Write`/`Can_SetControllerMode`（计数）+ PduR 回调；测试内静态配置（AD8）；关键用例：控制器 STOPPED→Transmit E_NOT_OK、STARTED+ONLINE→Transmit E_OK、RxIndication 匹配通知 PduR；`setUp()` 先 `CanIf_DeInit()` 归一化跨用例静态状态 |

共通陷阱：C 结构体位置初始化器字段数必须与被初始化类型字段数精确一致——`CanIf_RxPduConfigType` 8 字段少写 1 个会导致尾部字段静默清零（本次 RxIndication 用例失败的根因）。

## 影响面分析

- `currentClock`：全仓仅 `Mcu.c` 内部 3 处使用（init/InitClock/DistributePllClock），无跨模块耦合
- `PERIODREG`：全仓断言点仅 `test_spi.c:279` 一处
- `CRYPTO_SID_*`：仅 crypto 模块内部使用
- `LinIf_State`：仅 LinIf.c 内部静态状态，激活修复不改变 API 签名与配置布局
- `CanIf.h` 新增 `#include "Can.h"`：Can.h 为 MCAL 头，无循环包含风险（Can.h 不包含 CanIf.h）
- 三个新 CMake 目标（cantp_test/linif_test/canif_test）仅影响测试构建，不改变产物 BSW 库
