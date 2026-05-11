# yuleASR 工程结构速查表

> 本文件记录yuleASR工程的标准目录结构，后续开发请严格遵循。

---

## 目录结构总览

```
yuleASR/
├── src/                      # 源代码
│   ├── autosar/             # AUTOSAR BSW 静态代码 (94模块)
│   │   ├── mcal/           # 微控制器驱动层 (21模块)
│   │   ├── ecual/          # ECU抽象层 (29模块)
│   │   ├── services/       # 服务层 (44模块)
│   │   └── common/         # 通用头文件
│   ├── application/        # 应用层 (ASW)
│   ├── middleware/         # 中间件 (DDS/RTE)
│   ├── platform/           # 平台代码 (S32K312)
│   └── diagnostics/        # 诊断模块 (DCM/DEM)
│
├── config/                 # 配置代码 (117文件)
│   ├── mcal/              # MCAL配置
│   ├── ecual/             # ECUAL配置
│   ├── services/          # Services配置
│   ├── templates/         # 配置模板
│   └── arxml/            # ARXML配置
│
├── tests/                  # 测试代码
│   ├── unit/              # 单元测试
│   │   ├── autosar/      # BSW模块测试
│   │   ├── middleware/   # 中间件测试
│   │   ├── diagnostics/  # 诊断测试
│   │   └── framework/    # 测试框架
│   ├── integration/       # 集成测试
│   └── resources/         # 测试资源
│
├── tools/                  # 工具链
│   ├── arxml/            # ARXML工具 (parser/generator/checker)
│   ├── can_config/       # CAN配置工具
│   ├── dtc_config/       # DTC配置工具
│   ├── code_generators/  # 代码生成器
│   └── analysis/         # 分析工具
│
├── third_party/            # 第三方代码
│   ├── crypto/           # 加密库
│   └── test_frameworks/  # 测试框架
│
├── docs/                   # 文档 (151文件)
│   ├── architecture/     # 架构文档
│   ├── api/             # API参考
│   ├── modules/         # 模块文档
│   ├── guides/          # 使用指南
│   ├── design/          # 设计文档
│   ├── specs/           # 规范文档
│   ├── reports/         # 项目报告
│   └── external/        # 外部资料
│
└── scripts/                # 脚本
    ├── build/           # 构建脚本
    ├── test/            # 测试脚本
    ├── deploy/          # 部署脚本
    └── analysis/        # 分析脚本
```

---

## 代码存放规则

### 1. AUTOSAR BSW 模块

```
src/autosar/<layer>/<module>/
  ├── include/
  │   ├── <Module>.h          # API头文件
  │   ├── <Module>_Cfg.h      # 配置头文件
  │   └── <Module>_MemMap.h   # 内存映射(可选)
  └── src/
      ├── <Module>.c          # 主实现
      ├── <Module>_Irq.c      # 中断处理(可选)
      └── <Module>_Lcfg.c     # 链接配置(可选)
```

**MCAL层模块** (21个):
```
adc, can, crypto, dio, eep, eth, fee, flash, fls, gpt,
i2c, icu, lin, mcu, ocu, port, pwm, ramtst, spi, uart, wdg
```

**ECUAL层模块** (29个):
```
canif, cantp, canNm, canSm, cantrcv, dlt, doIP, ea, ethif,
ethSm, ethtrcv, fee, fim, frif, frtp, iohwab, ipdum, j1939tp,
linif, linNm, linSM, linTp, lintrcv, memif, someipif, someipsd,
srp, wdgif, xcp
```

**Services层模块** (44个):
```
bswm, canm, cansm, cantsyn, com, comM, crc, cryif, csm, dcm,
dem, det, dlt, docan, doip, e2e, ecuC, ecum, ethsm, fim,
ipdum, j1939nm, keym, linm, linsm, lntm, mem, memif, mqtt,
nm, nvm, pdur, ramsafety, schm, secoc, soad, someip, someiptp,
someipxf, stbm, swc, udpNm, wdgm, xcp
```

### 2. 配置文件

```
config/<layer>/<Module>_Cfg.h     # 配置头文件
config/<layer>/<Module>_Lcfg.c    # 链接配置(可选)
```

示例:
- `config/mcal/Can_Cfg.h` → CAN驱动配置
- `config/ecual/CanIf_Cfg.h` → CAN接口配置
- `config/services/Com_Cfg.h` → COM服务配置

### 3. 测试代码

```
tests/unit/<category>/<module>/
  ├── test_<feature>.c
  └── stubs/                    # 桩函数(可选)

tests/integration/<type>/
  └── integration_test.c
```

### 4. 工具代码

```
tools/<tool_name>/
  ├── src/                      # 核心代码
  ├── gui/                      # Web GUI
  ├── gui_desktop/              # 桌面GUI
  ├── tests/                    # 测试
  └── examples/                 # 示例
```

### 5. 第三方库

```
third_party/<category>/<library>/
  ├── include/                  # 头文件
  ├── src/                      # 源代码
  └── README.md                 # 说明文档
```

### 6. 文档

```
docs/<category>/
  └── *.md                      # Markdown文档
```

---

## 快速命令

```bash
# 构建项目
./scripts/build/build_all.sh -p S32K312

# 运行测试
./scripts/test/run_tests.sh -t unit

# ARXML工具
./tools/arxml/arxml_tool.py parse system.arxml
./tools/arxml/arxml_tool.py generate config.json -o output/

# CAN配置
./tools/can_config/can-config-tool.py --dbc example.dbc
```

---

## 完整性检查

| 目录 | 状态 | 模块数/文件数 |
|------|------|--------------|
| src/autosar/mcal | ✅ | 21模块 |
| src/autosar/ecual | ✅ | 29模块 |
| src/autosar/services | ✅ | 44模块 |
| config/ | ✅ | 117文件 |
| tests/ | ✅ | 205文件 |
| tools/ | ✅ | 178文件 |
| docs/ | ✅ | 145文档 |
| third_party/ | ✅ | 1500文件 |

**整体完整性: ✅ 完整**

---

*更新时间: 2026-05-12*
*版本: v2.0*
