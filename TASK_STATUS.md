# 项目状态与待办

> 最后更新: 2026-05-26

---

## 🏆 yuleASR — AutoSAR BSW 平台

### 状态总览

| 维度 | 状态 |
|:-----|:----:|
| BSW 模块 | 96 个 (MCAL 21 + ECUAL 29 + Services 46) ✅ |
| ASW 组件 | 8 个 ✅ |
| RTE | 6 个源文件, 20 个 CS API ✅ |
| OS | 3 个源文件 ✅ |
| 总代码量 | ~214K 行 C (146K C + 71K H) |
| 源文件数 | 621 个 |
| 单元测试 | 262 个文件, 70K 行 |
| API 文档注释 | 64K 行, 覆盖 312 个文件 |

### 已完成优化

#### P0 — 商用就绪
- ✅ CMake 构建修复 (零错误配置)
- ✅ CONTRIBUTING.md (542行), CODE_OF_CONDUCT.md, SECURITY.md (203行)
- ✅ 版权头覆盖 100% (621/621)
- ✅ Dockerfile (5阶段多架构)
- ✅ SBOM (SPDX 2.3, 12包23依赖)
- ✅ License 合规 (8个第三方补充 + check-licenses.sh)

#### P1 — 代码质量
- ✅ TODO 清除: 77 个 → 0
- ✅ 硬编码审计: 35 处配置化
- ✅ NvM 测试: 5/6 启用
- ✅ ComM 状态机: 6状态 + 唤醒处理
- ✅ Csm 密钥: KeyGenerate/Derive/Exchange
- ✅ Dem 时间戳: 全局tick + 5处TODO替换

#### P2 — 工程化
- ✅ RTE CS Operations: 20 个 API, ASW 调度集成
- ✅ CI/CD: 7 个 workflow (6+1 release)
- ✅ Release 自动化: tag v* 自动发布
- ✅ Docusaurus 文档站: 128 篇在线文档

### CI/CD

| Workflow | 状态 |
|:---------|:----:|
| `ci.yml` | ✅ 构建 + 测试 + 静态分析 |
| `deploy-docs.yml` | ✅ Docusaurus GH Pages 部署 |
| `docs.yml` | ✅ Doxygen + mdBook |
| `hil-tests.yml` | ✅ HIL 测试 |
| `integration-tests.yml` | ✅ E2E 集成测试 |
| `misra-check.yml` | ✅ MISRA C:2012 合规 |
| `release.yml` | ✅ tag 触发自动发布 |

---

## 🚀 yuleDKCS — CCC Digital Key 示例项目

> **仓库**: github.com/frisky1985/yuleDKCS
> **状态**: v2.0.0 已发布 ✅
> **总代码量**: ~277K 行, 890 文件

### Phase 1 — 项目重构 ✅
- yuleasr_dependency.md: 平台关系文档
- scripts/setup.sh: 一键环境脚本
- scripts/build.sh: 构建脚本
- embedded/include/yuleasr_bsw.h: BSW 集成头文件
- CMake 三通路检测: find_package/subdir/手动
- examples/ccc_reference/: 完整 CCC 参考示例 (7文件)

### Phase 2 — 嵌入式核心 ✅
- CCC R2.0 协议栈: HKDF-SHA256, AES-GCM, ECDSA
- SE050 SCP03: 安全通道完整实现
- DW3000 UWB: DS-TWR 测距驱动
- TODO 清零: 53 个 → 0

### Phase 3 — 三端联调 ✅
- 后端 19 个 TODO 清理
- E2E 测试: 钥匙生命周期 + 多协议 + Docker 环境
- API 参考文档: 314 行 → 1307 行
- iOS SDK: 覆盖率 21% → 62% (12 个 API 添加)
- Android SDK: 覆盖率 44% → 64% (5 个 API 添加)
- 9 项对齐问题全部修复

### Phase 4 — 文档与发布 ✅
- OTA 功能完整实现 (路由注册 + 移动端 SDK)
- MkDocs Material 文档站 (43 文件)
- CHANGELOG + GitHub Pages 部署配置
- **v2.0.0 版本标签已发布** ✅

### 嵌入式层模块

| 模块 | 状态 | 说明 |
|:-----|:----:|:-----|
| CCC 协议栈 | ✅ | R2.0, 23 测试用例 |
| ICCE 协议 | ✅ | 中国标准数字钥匙 |
| ICCOA 协议 | ✅ | 行业组织标准 |
| SE050 + SCP03 | ✅ | 安全芯片集成 |
| mbedTLS | ✅ | 加密库 |
| 国密 SM2/SM3/SM4 | ✅ | 中国商用密码 |
| DW3000 UWB | ✅ | 测距驱动 |
| KW47 BLE | ✅ | 蓝牙驱动 |

---

## 📋 项目关系

```
yuleDKCS (示例应用, v2.0.0)
  └── 依赖 → yuleASR (AutoSAR BSW平台)
               ├── OS/RTE/CSM/NvM/DCM/Dem
               ├── MCAL 21 + ECUAL 29 + Services 46
               └── 商用就绪: Docker/SBOM/CI/CD/Release
```
