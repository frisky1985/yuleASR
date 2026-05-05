# Crypto 模块集成指南

## 概述
本文档描述如何将 Crypto 硬件加密驱动模块集成到 yuleASR 项目中。

## 目录结构
```
src/bsw/mcal/crypto/
├── include/
│   ├── Crypto.h
│   ├── Crypto_Cfg.h
│   ├── Crypto_MemMap.h
│   └── SchM_Crypto.h
├── src/
│   ├── Crypto.c
│   └── Crypto_Cfg.c
├── test/
│   └── Crypto_Test.c
├── README.md
└── INTEGRATION.md  (本文件)
```

## 依赖模块

Crypto 模块依赖以下模块:

| 模块 | 描述 | 必需 |
|------|------|------|
| Std_Types.h | AUTOSAR 标准类型定义 | 是 |
| Det.h | 开发错误跟踪 | 否* |
| SchM_Crypto.h | 调度管理器 | 否* |

*当 CRYPTO_DEV_ERROR_DETECT 或 CRYPTO_ASYNC_OPERATION_SUPPORT 为 STD_ON 时必需

## 集成步骤

### 1. 添加文件到构建系统

在您的 Makefile 或 CMakeLists.txt 中添加:

```makefile
# 源文件
CRYPTO_SRC = \
    $(MCAL_DIR)/crypto/src/Crypto.c \
    $(MCAL_DIR)/crypto/src/Crypto_Cfg.c

# 头文件路径
CRYPTO_INC = \
    -I$(MCAL_DIR)/crypto/include

# 添加到总源文件列表
BSW_SRC += $(CRYPTO_SRC)
BSW_INC += $(CRYPTO_INC)
```

### 2. 配置选项调整

编辑 `Crypto_Cfg.h` 根据您的硬件平台进行配置:

```c
/* 硬件加速支持 */
#define CRYPTO_AES_HW_SUPPORT       STD_ON    /* 如果 MCU 有 AES 硬件加速器 */
#define CRYPTO_SHA256_HW_SUPPORT    STD_ON    /* 如果 MCU 有 SHA-256 硬件加速器 */
#define CRYPTO_RSA_HW_SUPPORT       STD_OFF   /* 如果 MCU 没有 RSA 硬件加速器 */
```

### 3. 硬件适配

根据您的 MCU 编写硬件适配层:

#### STM32 平台示例
```c
/* 在 Crypto.c 中修改以下函数 */

static Std_ReturnType Crypto_HwInitialize(void)
{
    /* 使能 CRYP 和 HASH 时钟 */
    __HAL_RCC_CRYP_CLK_ENABLE();
    __HAL_RCC_HASH_CLK_ENABLE();
    
    /* 初始化 CRYP 外设 */
    hpcd_cryp.Instance = CRYP;
    HAL_CRYP_Init(&hpcd_cryp);
    
    /* 初始化 HASH 外设 */
    hpcd_hash.Instance = HASH;
    HAL_HASH_Init(&hpcd_hash);
    
    return E_OK;
}
```

#### NXP 平台示例 (i.MX RT)
```c
static Std_ReturnType Crypto_HwInitialize(void)
{
    /* 初始化 DCP (Data Co-Processor) */
    DCP_Init(DCP);
    
    /* 启用 AES 引擎 */
    DCP_AES_SetKey(DCP, key, keySize);
    
    return E_OK;
}
```

### 4. 中断配置

如果使用异步操作，需要配置中断:

```c
/* 在 MCU 的启动代码中 */
void Crypto_IRQHandler(void)
{
    /* 处理 Crypto 中断 */
    if (CRYPTO_HW_STATUS & CRYPTO_INT_DONE) {
        /* 清除中断标志 */
        CRYPTO_HW_STATUS = CRYPTO_INT_DONE;
        
        /* 调用驱动处理 */
        Crypto_MainFunction();
    }
}

/* 在启动代码中使能中断 */
NVIC_EnableIRQ(CRYPTO_IRQn);
```

### 5. 调度器集成

实现 SchM_Crypto.h 中宣告的函数:

```c
/* SchM_Crypto.c */
#include "Os.h"

void SchM_Enter_Crypto_CRYPTO_EXCLUSIVE_AREA_0(void)
{
    SuspendAllInterrupts();
}

void SchM_Exit_Crypto_CRYPTO_EXCLUSIVE_AREA_0(void)
{
    ResumeAllInterrupts();
}

/* 其他 exclusive area 函数... */
```

### 6. 与 CryIf 集成

如果您的项目使用了 CryIf (Crypto Interface)需要将 Crypto 驱动与 CryIf 连接:

```c
/* 在 CryIf_Cfg.c 中 */
const CryIf_ChannelConfigType CryIf_ChannelConfigs[] = {
    {
        .channelId = 0,
        .priority = 1,
        .primitive = CRYIF_CRYPTOPRIMITIVE_ENCRYPT,
        .algorithmFamily = CRYIF_ALGOFAM_AES,
        .algorithmMode = CRYIF_ALGOMODE_CBC,
        .callbackActive = TRUE
    }
};

const CryIf_ConfigType CryIf_Config = {
    .channelConfigs = CryIf_ChannelConfigs,
    .numChannels = 1,
    .keyConfigs = CryIf_KeyConfigs,
    .numKeys = CRYIF_NUM_KEYS
};
```

## 内存配置

### RAM 需求
| 项目 | 大小 (字节) | 描述 |
|------|------------|------|
| 驱动对象 | ~1KB | 4 个驱动对象 × 约 256 字节 |
| 通道状态 | ~1KB | 8 个通道 × 约 128 字节 |
| 密钥存储 | ~26KB | 16 个密钥 × 5 个元素 × 32 字节 |
| 作业队列 | ~2KB | 16 个作业 × 约 128 字节 |
| **总计** | **~30KB** | |

### ROM 需求
| 项目 | 大小 (字节) | 描述 |
|------|------------|------|
| 代码 | ~20-50KB | 取决于启用的算法 |
| 配置数据 | ~2KB | 驱动对象、通道、密钥配置 |
| **总计** | **~25-55KB** | |

## 配置示例

### 最小配置 (仅 AES)
```c
#define CRYPTO_NUM_DRIVER_OBJECTS    1
#define CRYPTO_NUM_CHANNELS          1
#define CRYPTO_NUM_KEYS              4
#define CRYPTO_MAX_JOB_QUEUE_SIZE    4

#define CRYPTO_SHA256_HW_SUPPORT     STD_OFF
#define CRYPTO_HMAC_HW_SUPPORT       STD_OFF
#define CRYPTO_RSA_HW_SUPPORT        STD_OFF
#define CRYPTO_TRNG_HW_SUPPORT       STD_OFF
```

### 完整配置
```c
#define CRYPTO_NUM_DRIVER_OBJECTS    4
#define CRYPTO_NUM_CHANNELS          8
#define CRYPTO_NUM_KEYS              16
#define CRYPTO_MAX_JOB_QUEUE_SIZE    16

#define CRYPTO_AES_HW_SUPPORT        STD_ON
#define CRYPTO_SHA256_HW_SUPPORT     STD_ON
#define CRYPTO_HMAC_HW_SUPPORT       STD_ON
#define CRYPTO_RSA_HW_SUPPORT        STD_ON
#define CRYPTO_TRNG_HW_SUPPORT       STD_ON
#define CRYPTO_ASYNC_OPERATION_SUPPORT STD_ON
```

## 测试验证

运行测试用例验证集成:

```bash
# 编译测试代码
gcc -o crypto_test \
    -DCRYPTO_TEST_STANDALONE \
    -I../../common \
    -I../../services/cryif/include \
    -I./include \
    ./test/Crypto_Test.c \
    ./src/Crypto.c \
    ./src/Crypto_Cfg.c

# 运行测试
./crypto_test
```

## 故障排除

### 常见问题

1. **初始化失败**
   - 检查硬件时钟是否使能
   - 验证配置指针是否有效

2. **加密失败**
   - 确保密钥已设置并标记为有效
   - 检查 IV 长度是否正确
   - 验证输出缓冲区足够大

3. **异步操作无响应**
   - 检查中断是否正确配置
   - 确保 MainFunction 被定期调用
   - 验证回调函数是否注册

4. **内存访问错误**
   - 检查内存映射配置
   - 验证缓冲区对齐要求

## 支持与联系

如遇到问题，请参阅:
- 技术文档: `README.md`
- AUTOSAR 规范: AUTOSAR_SWS_CryptoDriver.pdf
- 项目文档: yuleASR 技术文档
