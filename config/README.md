# yuleASR 配置管理指南

本目录存放项目所有配置文件，采用双层配置管理策略。

## 目录结构

```
config/
├── input/                    # 配置输入 (手动编辑)
│   ├── mcal/                 # MCAL层配置 (ADC, CAN, SPI...)
│   ├── ecual/                # ECUAL层配置 (CanIf, EthIf...)
│   ├── services/             # Services层配置 (DCM, DEM, COM...)
│   └── arxml/                # ARXML配置源文件
├── generated/                # 工具生成的配置 (勿手动编辑)
│   ├── mcal/                 # MCAL生成配置
│   ├── ecual/                # ECUAL生成配置
│   ├── services/             # Services生成配置
│   └── rte/                   # RTE生成配置
├── templates/                # 配置模板
├── tools/                    # 工具配置
└── README.md                # 本文件
```

## 配置管理规范

### 1. 输入配置 (config/input/)

这些配置由开发人员手动编辑：

- **MCAL配置**: 驱动级配置 (ADC通道、CAN波特率等)
- **ECUAL配置**: 抽象层配置 (CanIf映射、PduR路由等)
- **Services配置**: 服务层配置 (COM信号、DTC定义等)
- **ARXML配置**: AUTOSAR XML配置源文件

编辑后需要运行配置生成工具更新 generated/ 目录。

### 2. 生成配置 (config/generated/)

这些配置由工具自动生成，**不要手动修改**：

- 生成工具: `tools/arxml-tool/`, `tools/code_generators/`
- 输出格式: `<Module>_Cfg.h`, `<Module>_Lcfg.c`
- 引用方式: `#include "config/generated/services/Dcm_Cfg.h"`

### 3. 配置模板 (config/templates/)

包含配置文件模板，用于创建新模块配置：

- 标准模板: `Can_Cfg.h`, `Com_Cfg.h` 等
- 自定义模板: 项目特定配置模板

### 4. 工具配置 (config/tools/)

各配置工具的输入参数：

- `nvm_config.json` - NvM配置工具输入
- `dtc_config.json` - DTC配置工具输入
- `cantp_config.json` - CanTp配置工具输入
- `rte_config.json` - RTE生成工具输入

## 工作流程

```
1. 编辑输入配置
   → 修改 config/input/services/Dcm_Cfg.h

2. 运行配置生成工具
   → ./tools/code_generators/generate_config.py

3. 生成配置更新
   → config/generated/services/Dcm_Cfg.h (自动更新)

4. 编译代码
   → src/bsw/services/dcm/src/Dcm.c 引用生成配置
```

## 注意事项

1. 不要直接修改 `config/generated/` 下的文件
2. 配置变更后需要重新生成才能生效
3. 重要配置变更需要记录在版本控制中

## 相关工具

- ARXML配置生成: `tools/arxml-tool/`
- 代码生成器: `tools/code_generators/`
- DTC配置工具: `tools/dtc_config/`
- NvM配置工具: `tools/nvm_configurator/`
