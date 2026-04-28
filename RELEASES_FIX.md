# 版本发布修复计划

## 当前问题
- GitHub Releases 页面只显示 v0.1.0
- v0.2.0、v0.3.0、v0.4.0 有tag但没有release说明
- 版本分散在main和master两个分支

## 建议操作

### 1. 为现有tag创建releases

#### v0.2.0 (master分支)
```bash
# 在GitHub上创建release
curl -X POST \
  -H "Authorization: token $GITHUB_TOKEN" \
  -H "Accept: application/vnd.github.v3+json" \
  https://api.github.com/repos/frisky1985/yuleASR/releases \
  -d '{
    "tag_name": "v0.2.0",
    "name": "Release v0.2.0 - 工程质量体系全面升级",
    "body": "## 主要改进\n\n### 工程基础设施\n- 添加完整的测试框架\n- 优化CI门禁流程\n- 完善开源合规检查\n- 重构项目文档结构\n\n### 质量保障\n- 代码覆盖率提升\n- 自动化测试增强\n- 静态分析工具集成",
    "draft": false,
    "prerelease": false
  }'
```

#### v0.3.0 (master分支)
```bash
curl -X POST \
  -H "Authorization: token $GITHUB_TOKEN" \
  -H "Accept: application/vnd.github.v3+json" \
  https://api.github.com/repos/frisky1985/yuleASR/releases \
  -d '{
    "tag_name": "v0.3.0",
    "name": "Release v0.3.0 - AutoSAR DLT诊断日志模块",
    "body": "## 主要功能\n\n### DLT模块 (Diagnostic and Logging Trace)\n- 符合AutoSAR 4.x标准\n- 完整DLT协议实现\n- 诊断日志和跟踪功能\n- 配置工具支持\n\n### 技术规格\n- 遵循AutoSAR R21-11规范\n- 支持多种传输通道\n- 完整的单元测试覆盖",
    "draft": false,
    "prerelease": false
  }'
```

### 2. main分支发布 v0.5.0
基于当前main分支的最新整合成果发布新版本。

## 版本规划建议

### 当前版本分布
```
main 分支:  v0.1.0 -----> 建议发布 v0.5.0 (整合成果)
                  \
master 分支:      v0.2.0 -> v0.3.0 -> v0.4.0
```

### 推荐方案
1. **保持main分支为主开发线**
2. **发布 v0.5.0** 包含最新整合成果
3. **归档master分支** 或合并到main
4. **统一使用main分支**进行后续开发

## v0.5.0 发布内容建议

### 版本亮点
- 整合master分支的Harness工程体系
- 增强测试框架和静态分析工具
- 补充简化版RTE生成器
- 保留完整的BSW/DDS/安全栈实现

### 包含模块
- Classic AUTOSAR BSW (EthIf, SoAd, PduR, DCM, DEM)
- DDS中间件 (DDS-XRCE, RTPS, QoS)
- 功能安全 (RAM ECC, SafeRAM, MPU)
- 诊断栈 (UDS, DLT)
- 安全通信 (SecOC, CSM, CryIf, KeyM)
- OTA更新管理
- FreeRTOS多架构支持
