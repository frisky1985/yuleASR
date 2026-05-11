# AUTOSAR 技术博客爬取资料

## 概述

本资料库包含从「孤星旅记」AUTOSAR专栋爬取的技术文章，聚焦AUTOSAR Classic平台的通信堆栈、诊断、网络管理与工具链。

## 来源信息

- **博客名称**: AUTOSAR 专栋 | 孤星旅记
- **原始URL**: https://binkyle.github.io/%E6%8A%80%E6%9C%AF%E7%AC%94%E8%AE%B0/Autosar/
- **平台**: VuePress 2.0 + VuePress Theme Hope
- **描述**: 聚焦 AUTOSAR Classic 平台，涵盖通信堆栈、诊断、网络管理与工具链
- **爬取时间**: 2025-05-11
- **总文章数**: 45篇

## 文章目录

### 系统概述
1. AUTOSAR Classic Platform 全景综述
2. AUTOSAR XML Schema Production Rules 全面解析与实践指南
3. Autosar解决方案概述

### 驱动与硬件抽象层 (MCAL)
4. Adc - ADC模块入门
5. AUTOSAR ADC 原理与实战指南
6. CAN - CAN模块入门
7. AUTOSAR CAN模块详解与实战配置
8. CAN Hardware Object分析
9. CAN总线休眠与唤醒机制解析
10. Can Trcv - CAN收发器驱动
11. AUTOSAR Classic 平台 CAN Transceiver Driver（CanTrcv）详解

### 通信服务 (COM Stack)
12. COM - 通信模块
13. AUTOSAR Com模块概述
14. AUTOSAR Com模块配置详解
15. COMM - 通信管理
16. AUTOSAR ComM模块详解分析

### 以太网通信
17. AVTP - 音频视频传输协议
18. AVTP 与 CAN over AVTP 在车载以太网中的协议详解与工程实践
19. DDS - 数据分发服务
20. AUTOSAR Classic平台DDS协议实现与模块设计详解
21. SOMEIP - 车载以太网通信协议
22. SOME/IP Service Discovery 原理与工程实践
23. So Ad - Socket适配器
24. AUTOSAR SoAd 模块原理与配置实践全景详解
25. Ld Com - 大数据通信
26. AUTOSAR LdCom 模块详解

### 操作系统 (OS)
27. AUTOSAR Classic OS 功能详解
28. AUTOSAR OS 自旋锁（Spinlock）原理与工程实践
29. AUTOSAR 服务发现 (Service Discovery) 配置指南

### 诊断与日志
30. DET - 默认错误追踪
31. AUTOSAR Default Error Tracer（DET）模块详解与最佳实践
32. DLT - 诊断日志与追踪
33. AUTOSAR DLT（Diagnostic Log and Trace）模块详解

### 基础软件与工具
34. BSWM - 基础软件模式管理
35. AUTOSAR BSWM模块详解

### 软件组件 (SWC)
36. SWC - 软件组件入门
37. AUTOSAR Port 的设计、连接原理与实践总结

### 工具链
38. Da Vinci Developer - Vector开发工具
39. 基本使用手册
40. Autosar davinci developer classic使用教程
41. 端口自动连线设计
42. davinci developer classic Port auto-connect
43. Vector CAST - 测试工具
44. VectorCAST 原理与实战指南

## 文件说明

| 文件 | 说明 |
|------|------|
| `summary.json` | 汇总信息，包含所有文章列表和URL |
| `autosar_articles.md` | Markdown格式的文章内容 |
| `README.md` | 本说明文件 |

## 使用建议

1. **快速定位**: 使用文章目录找到感兴趣的主题
2. **深度学习**: 阅读 `autosar_articles.md` 中的完整内容
3. **实践参考**: 结合项目实际应用相关模块

## 技术覆盖

- ✓ AUTOSAR Classic 平台架构
- ✓ MCAL层驱动开发
- ✓ 通信堆栈（CAN/ETH/DDS/SOMEIP）
- ✓ 基础软件管理
- ✓ 诊断与日志系统
- ✓ 工具链使用
- ✓ ARXML配置

## 注意事项

- 本资料仅用于学习研究，请尊重原作者版权
- 建议访问原网站获取最新内容
- 部分文章可能需要结合官方文档使用

## 爬取工具

使用自研的Web Scraper技能爬取：
```bash
python3 ~/.hermes/skills/web-scraper/scripts/deep_crawler.py \
  --url https://binkyle.github.io/技术笔记/Autosar/ \
  --depth 3 \
  --output-dir ./autosar_blog_crawled
```

---
*数据来源: 孤星旅记 (binkyle.github.io)*  
*爬取时间: 2025-05-11*
