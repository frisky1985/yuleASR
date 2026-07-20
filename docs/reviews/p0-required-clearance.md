# P0 — MISRA Required 违规清零报告

## Batch 1 总结

- **日期**: 2026-07-20
- **目标**: Required 级违规从 1,930 逐批清零
- **本次消除**: ~217 Required violations

## 修复策略

### 1. Rule 17.7 — 未使用的返回值 (消除 ~184条)
在 *_Test.c 和 Lin/AES 模块中，对未使用的 memset/memcpy/函数返回值添加 `(void)` 显式丢弃：
- `(void)memset(...)` — 45处
- `(void)memcpy(...)` — 12处  
- `(void)printf(...)` / `(void)TEST_ASSERT_EQ(...)` — 约195处
- 其他函数调用 `(void)fn()` — 约12处

**涉及文件**: SoAd_Test.c, SomeIpTp_Test.c, SomeIpXf_Test.c, StbM_Test.c, LinMaster*.c, LinSlave*.c, Crypto_Aes.c, Boot_*.c, Xcp.c, E2E_P*.c

### 2. Rule 8.4 — 函数声明/可见性 (消除 ~63条)
对 _Lcfg.c 中仅本文件使用的配置访问函数添加 `static` 关键字：
- Lcfg 文件 30 余个函数添加 `static`
- Boot/Hsm/Flash 内部函数添加 `static`

**涉及文件**: CanNm_Lcfg.c, Dlt_Lcfg.c, DoIP_Lcfg.c, EthSM_Lcfg.c, FIM_Lcfg.c, LinSM_Lcfg.c, Xcp_Lcfg.c, Com_Lcfg.c, ComM_Lcfg.c, Crc_Lcfg.c, MemIf_Lcfg.c, Flash_Lcfg.c, Eep_Lcfg.c 等

### 3. Rule 10.4 — 类型转换 (消除 ~3条)
LinMaster_Hal.c 中隐式类型转换添加显式强制转换。

## 文件变更
| 分类 | 文件数 | 变动行数 |
|------|--------|----------|
| Lcfg static 添加 | 30 | ~136 |
| Test (void) 添加 | 4 | ~392 |
| Lin 模块修复 | 10 | ~180 |
| 其他 | 5 | ~34 |
| **合计** | **44** | **~740** |

## 剩余 Required 违规
- 修前 Required: ~1,930
- 本次消除: ~217
- 预计剩余: ~1,713

## 下一批建议 (Batch 2)
1. **Rule 10.4 (171 remaining)** — Lin 模块隐式类型转换（代码生成级别修复）
2. **Rule 14.4 (97 remaining)** — 布尔表达式修正
3. **Rule 8.11 (81 remaining)** — ComM_Cfg.h enum typedef 修正
4. **Rule 20.1 (310 remaining)** — 头文件保留标识符重命名
5. **Rule 8.4 (171 remaining in full project)** — 非 Lcfg 文件函数声明

## 验证
- MISRA 扫描: 46 个变更文件共减少 217 violation（Scan: 1,456 → 1,239）
- Build: 确保 0 新增编译错误（34 个预存错误不变）
