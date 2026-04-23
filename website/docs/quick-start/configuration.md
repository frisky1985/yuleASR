---
sidebar_position: 4
---

# 配置指南

学习如何配置 YuleTech AutoSAR BSW Platform。

## 配置文件

所有配置都位于 `configs/` 目录下的 JSON 文件中。

### 系统配置 (system.json)

```json
{
  "target": {
    "mcu": "STM32F407",
    "oscillator": 8000000
  },
  "modules": {
    "Can": true,
    "Spi": true,
    "Dio": true
  }
}
```

### 驱动配置

每个驱动都有自己的配置文件，如 `Mcu.json`、`Can.json` 等。
