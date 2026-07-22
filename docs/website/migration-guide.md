# 从 EasyXMen 迁移至 yuleASR

本指南帮助 EasyXMen 用户快速迁移到 yuleASR 平台。

## 迁移概览

| 维度 | EasyXMen | yuleASR | 迁移难度 |
|:-----|:---------|:--------|:--------:|
| 许可模式 | 商业许可 | MIT 开源 | — |
| AUTOSAR 版本 | 4.x | 4.x / R20-11 | 低 |
| MCAL | 专有实现 | 开源实现，API 兼容 | 中 |
| ECU 配置 | 专有格式 | ARXML + 工具链 | 中 |
| 诊断协议栈 | UDS | DCM/DEM + UDS | 低 |
| 网络管理 | CAN NM | CanNm + UDP Nm + J1939Nm | 低 |
| 操作系统 | 内建 OS | FreeRTOS 适配 | 中 |
| DDS 中间件 | 需集成 | 内建 | 易 |

## 步骤一：评估现有配置

```bash
# 导出 EasyXMen 的 ARXML 配置
# 假设已有 system.arxml
cd yuleASR
python3 tools/arxml/arxml_tool.py check system.arxml
```

## 步骤二：配置迁移

### CAN 配置

```bash
# 将 EasyXMen 的 DBC 文件转换为 yuleASR 格式
python3 tools/can_config/can-config-tool.py \
  --dbc path/to/easyxmen/your_can.dbc \
  --output config/can/
```

### DTC 配置

```bash
# 准备 DTC CSV（格式兼容）
python3 tools/dtc_config/dtc-tool.sh \
  --input path/to/easyxmen/dtc_config.csv
```

> 注：yuleASR 的 DTC CSV 格式与 EasyXMen 基本兼容，只需调整列头映射。

### UDS 诊断

```bash
python3 tools/uds_configurator/uds-tool.sh \
  --input path/to/easyxmen/uds_config.json
```

## 步骤三：MCAL 移植注意事项

EasyXMen 的 MCAL 通常为预编译库，yuleASR 提供开源的 MCAL 源码：

- **GPIO / Port**: API 基本兼容，重新生成 `Port_Cfg.h`
- **CAN**: 检查 CAN 控制器基址与时钟配置
- **ADC / PWM / GPT**: 配置项需对照芯片参考手册调整
- **Flash / EEPROM**: 确认分区布局一致

使用 yuleASR 配置工具生成：

```bash
python3 tools/arxml/arxml_tool.py generate \
  --cfg config/your_platform.json \
  --output generated/
```

## 步骤四：应用层适配

yuleASR 的 RTE 接口遵循 AUTOSAR 标准，兼容 EasyXMen 的 SWC 接口：

```c
// EasyXMen 风格
void MySwc_Runnable(void)
{
    /* 原有逻辑无需修改 */
    uint8 signal;
    Rte_Read_RpSignal_Port(&signal);
}

// yuleASR — API 兼容
void MySwc_Runnable(void)
{
    uint8 signal;
    Rte_Read_RpSignal_Port(&signal); // 同样可用
}
```

## 步骤五：验证

```bash
# 运行单元测试
make test

# 运行集成测试
ctest --output-on-failure

# 生成报告
python3 tools/analysis/report_summary.py
```

## 常见问题

**Q: 迁移后性能会下降吗？**
A: yuleASR 的 MCAL 源码级实现允许更优的编译器优化，实测在 S32K312 上性能持平或略优。

**Q: 已有 BSW 配置能否复用？**
A: ARXML 格式兼容 AUTOSAR 4.x 标准，可直接导入。专有格式需通过工具链转换。

**Q: 技术支持渠道？**
A: 通过 GitHub Issues 提交 · 社区讨论 · 予乐科技提供商业支持选项。

## 迁移检查清单

- [ ] 评估现有模块使用情况
- [ ] 导出 ARXML 配置
- [ ] 转换 CAN/DTC/UDS 配置
- [ ] 生成 MCAL 配置头文件
- [ ] 移植应用层 SWC
- [ ] 运行单元测试
- [ ] 运行集成测试
- [ ] 进行硬件在环 (HIL) 验证
