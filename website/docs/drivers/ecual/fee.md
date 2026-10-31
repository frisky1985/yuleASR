---
title: FEE 模块
sidebar_label: fee
description: "void fee_GetVersionInfo(Std_VersionInfoType* versioninfo);"
sidebar_position: 7
---

# FEE 模块

## 概述

FEE 模块实现。

## API接口

### 初始化

```c
void fee_Init(void);
```

### 版本信息

```c
void fee_GetVersionInfo(Std_VersionInfoType* versioninfo);
```

## 配置参数

| 参数 | 说明 |
|:-----|:-----|
| - | - |

## 使用示例

```c
fee_Init();
```

## 版本历史

| 版本 | 日期 | 描述 |
|:-----|:-----|:-----|
| 1.0 | 2026-01-09 | 初始版本 |
