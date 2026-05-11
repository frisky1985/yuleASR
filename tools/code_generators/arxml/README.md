# yuleASR ARXML Generator

Vector Configurator风格的AUTOSAR ARXML配置生成工具

## 功能特性

- **MCAL配置生成**: 支持Mcu、Port、Can、Spi、Gpt等微控制器驱动
- **BSW配置生成**: 支持Com、PduR、CanIf、NvM等基础软件服务
- **AUTOSAR R4.0标准**: 生成符合AUTOSAR规范的ECUC配置文件
- **Vector风格API**: 类似Vector Configurator的用户体验

## 安装

```bash
cd /home/admin/yuleASR/tools/arxml-generator
chmod +x arxml-generator.py
```

## 快速开始

### 生成MCU配置

```bash
./arxml-generator.py mcu --clock 80000000 --periph-clock 40000000 -o Mcu.arxml
```

### 生成Port配置

```bash
./arxml-generator.py port --pins "PA0:OUT,PA1:IN,PA2:OUT,PB5:OUT" -o Port.arxml
```

### 生成CAN配置

```bash
./arxml-generator.py can --baudrate 500000 --controller 0 --tx-objects 4 --rx-objects 4 -o Can.arxml
```

### 生成COM配置

```bash
./arxml-generator.py com --ipdus 4 --signals 8 -o Com.arxml
```

### 生成NvM配置

```bash
./arxml-generator.py nvm --blocks 8 --crc NVM_CRC32 -o NvM.arxml
```

## 命令参考

### mcu - MCU驱动配置

| 参数 | 说明 | 默认值 |
|:-----|:-----|:-------|
| `--ecu` | ECU实例名称 | ECU0 |
| `--clock` | CPU时钟频率(Hz) | 80000000 |
| `--periph-clock` | 外设时钟频率(Hz) | 40000000 |
| `--dev-error-detect` | 启用开发错误检测 | True |
| `-o` | 输出文件路径 | 必需 |

### port - Port驱动配置

| 参数 | 说明 | 默认值 |
|:-----|:-----|:-------|
| `--ecu` | ECU实例名称 | ECU0 |
| `--pins` | 引脚配置 (格式: PA0:OUT,PA1:IN) | 必需 |
| `--dev-error-detect` | 启用开发错误检测 | True |
| `-o` | 输出文件路径 | 必需 |

### can - CAN驱动配置

| 参数 | 说明 | 默认值 |
|:-----|:-----|:-------|
| `--ecu` | ECU实例名称 | ECU0 |
| `--baudrate` | 波特率(bps) | 500000 |
| `--controller` | 控制器ID | 0 |
| `--tx-objects` | 发送HOH数量 | 2 |
| `--rx-objects` | 接收HOH数量 | 2 |
| `-o` | 输出文件路径 | 必需 |

### com - COM通信服务配置

| 参数 | 说明 | 默认值 |
|:-----|:-----|:-------|
| `--ecu` | ECU实例名称 | ECU0 |
| `--signals` | 信号数量 | 4 |
| `--ipdus` | IPDU数量 | 2 |
| `-o` | 输出文件路径 | 必需 |

### nvm - NvM NVRAM管理器配置

| 参数 | 说明 | 默认值 |
|:-----|:-----|:-------|
| `--ecu` | ECU实例名称 | ECU0 |
| `--blocks` | NVRAM块数量 | 4 |
| `--crc` | CRC类型 | NVM_CRC32 |
| `-o` | 输出文件路径 | 必需 |

## 程序化API

```python
from mcal_config_generator import create_mcu_config
from bsw_config_generator import create_com_config

# 生成MCU配置
mcu_gen = create_mcu_config("ECU0")
mcu_gen.add_general_config(dev_error_detect=True)
mcu_gen.add_clock_config(cpu_clock=80000000, peripheral_clock=40000000)
arxml_content = mcu_gen.to_arxml()

# 生成COM配置
com_gen = create_com_config("ECU0")
com_gen.add_general_config()
com_gen.add_ipdu_config("IPDU_0", pdu_id=0, length=8, direction="SEND")
com_gen.add_signal_config("Signal_0", ipdu_ref="IPDU_0", start_bit=0, bit_length=8)
arxml_content = com_gen.to_arxml()
```

## 项目结构

```
arxml-generator/
├── arxml-generator.py          # 主入口脚本
├── src/
│   ├── ecuc_config_model.py      # ECUC配置数据模型
│   ├── arxml_ecuc_generator.py   # ARXML生成器
│   ├── mcal_config_generator.py  # MCAL配置生成器
│   └── bsw_config_generator.py   # BSW配置生成器
├── examples/
│   ├── example_mcu_config.json   # MCU配置示例
│   └── example_can_config.json   # CAN配置示例
└── tests/
    └── test_generator.py         # 测试脚本
```

## 支持的模块

### MCAL模块
- ✅ Mcu - 微控制器驱动
- ✅ Port - 端口驱动
- ✅ Can - CAN驱动
- ✅ Spi - SPI驱动
- ✅ Gpt - 通用定时器
- 🔲 Adc - ADC驱动 (计划中)
- 🔲 Pwm - PWM驱动 (计划中)

### BSW服务
- ✅ Com - 通信服务
- ✅ PduR - PDU路由器
- ✅ CanIf - CAN接口
- ✅ NvM - NVRAM管理器
- 🔲 CanTp - CAN传输协议 (计划中)
- 🔲 Dcm - 诊断通信管理器 (计划中)
- 🔲 Dem - 诊断事件管理器 (计划中)

## 输出示例

生成的ARXML文件符合AUTOSAR R4.0标准：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<AUTOSAR xmlns="http://autosar.org/schema/r4.0">
  <AR-PACKAGES>
    <AR-PACKAGE>
      <SHORT-NAME>EcucModuleConfiguration</SHORT-NAME>
      <ELEMENTS>
        <ECUC-MODULE-CONFIGURATION-VALUES>
          <SHORT-NAME>Mcu</SHORT-NAME>
          <DEFINITION-REF DEST="ECUC-MODULE-DEF">/AUTOSAR/EcucDefs/Mcu</DEFINITION-REF>
          <CONTAINERS>
            <ECUC-CONTAINER-VALUE>
              <SHORT-NAME>McuGeneral</SHORT-NAME>
              <DEFINITION-REF DEST="ECUC-PARAM-CONF-CONTAINER-DEF">/AUTOSAR/EcucDefs/Mcu/McuGeneral</DEFINITION-REF>
              <PARAMETER-VALUES>
                <ECUC-NUMERICAL-PARAM-VALUE>
                  <DEFINITION-REF DEST="ECUC-BOOLEAN-PARAM-DEF">/AUTOSAR/EcucDefs/Mcu/McuGeneral/McuDevErrorDetect</DEFINITION-REF>
                  <VALUE>true</VALUE>
                </ECUC-NUMERICAL-PARAM-VALUE>
              </PARAMETER-VALUES>
            </ECUC-CONTAINER-VALUE>
          </CONTAINERS>
        </ECUC-MODULE-CONFIGURATION-VALUES>
      </ELEMENTS>
    </AR-PACKAGE>
  </AR-PACKAGES>
</AUTOSAR>
```

## 版本历史

| 版本 | 日期 | 说明 |
|:-----|:-----|:-----|
| 1.0.0 | 2026-05-09 | 初始版本，支持MCAL和BSW配置生成 |

## 参考文档

- [AUTOSAR_TPS_ECUConfiguration.pdf](https://www.autosar.org/standards/classic-platform)
- Vector Configurator用户手册
