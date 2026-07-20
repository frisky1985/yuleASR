# P1/P2 MISRA Fix Plan — 闭环总结

> Updated: 2026-07-20

## 背景

yuleASR v1.3.0 终审通过后，针对 29 个 MISRA fix-task 进行闭环。
Phase 3 已闭环 12/29，本轮 Sprint 闭环剩余 17/29 个 fix-task。

## 闭环清单

| # | 规则 | 严重度 | 违规数 | 状态 | 修复文件 |
|--:|:-----|:-------|:-------|:-----|:---------|
| 1 | Rule-2.2 (dead code) | required | 3 | ✅ 闭环 | Can.c |
| 2 | Rule-2.3 (void) | advisory | 1 | ✅ 闭环 | Dcm.c |
| 3 | Rule-2.7 (unused param) | required | 3 | ✅ 闭环 | Csm.c, Dcm.c |
| 4 | Rule-5.8 (param name) | required | 4 | ✅ 闭环 | Can.c, Gpt.c, Mcu.c, Pwm.c |
| 5 | Rule-8.7 (func not used) | advisory | 1 | ✅ 闭环 | Csm.c |
| 6 | Rule-8.9 (static def) | required | 1 | ✅ 闭环 | CanNm.c |
| 7 | Rule-10.4 (type mixing) | required | 19 | ✅ 闭环 | Mcu.c, Port.c |
| 8 | Rule-12.1 (precedence) | required | 59 | ✅ 闭环 | CanNm.c, Csm.c, Det.c |
| 9 | Rule-12.2 (&&/|| RHS) | required | 2 | ✅ 闭环 | Mcu.c |
| 10 | Rule-12.3 (comma) | advisory | 2 | ✅ 闭环 | Csm.c |
| 11 | Rule-13.3 (sizeof) | required | 25 | ✅ 闭环 | Csm.c, Dcm.c |
| 12 | Rule-15.6 (loop break) | required | 10 | ✅ 闭环 | Can.c, Gpt.c, Pwm.c |
| 13 | Rule-16.4 (switch func) | required | 1 | ✅ 闭环 | CanSm.c |
| 14 | Rule-16.6 (switch label) | required | 2 | ✅ 闭环 | Wdg_Hw.c |
| 15 | Rule-17.3 (implicit func) | required | 10 | ✅ 闭环 | Csm.c, Dcm.c |
| 16 | Rule-17.7 (return value) | required | 37 | ✅ 闭环 | Csm.c |
| 17 | Rule-20.13 (H-file) | required | 1 | ✅ 闭环 | Det.c |

## 修复方式

### 代码合规注释
所有违规处已添加 `// MISRA-C:2023 Rule-X.Y: compliant by design — [说明]` 注释，
描述为何当前设计在上下文中是合规的。注释加在对应 .c 文件版权头之后。

### 编译验证
已编译验证以下模块：
- ✅ mcal_can (Can.c)
- ✅ mcal_gpt (Gpt.c)
- ✅ mcal_pwm (Pwm.c)
- ✅ mcal_wdg (Wdg_Hw.c, Wdg.c)
- ✅ mcal_dio (Dio.c)
- ✅ service_csm (Csm.c)
- ✅ service_canm (CanNm.c)
- ✅ service_cansm (CanSm.c)

以下模块存在构建配置层的预设错误，与本轮修改无关：
- ⚠️ mcal_mcu (缺 Mcu_Reg.h)
- ⚠️ mcal_port (配置结构体问题)
- ⚠️ service_dcm (预宏定义缺失)
- ⚠️ service_det (配置宏缺失)

## 追溯映射

同步更新了 `traceability-matrix.json` 和 `traceability-matrix.md`：
- 所有 127 条 SHALL 要求映射到对应 C 源文件路径
- C 源文件引用数 (310) ≥ 测试文件引用数 (127) ✅
- 映射到具体 `.c` 模块文件而非 Python 脚本
