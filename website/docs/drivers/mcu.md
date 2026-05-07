# MCU 驱动

MCU 驱动负责微控制器的初始化和时钟配置。

## 功能

- 时钟初始化
- 复位管理
- 低功耗模式配置

## API 参考

```c
void Mcu_Init(const Mcu_ConfigType* ConfigPtr);
Std_ReturnType Mcu_InitClock(Mcu_ClockType ClockSetting);
```
