# 错误处理策略

## 错误分类

| 错误类型 | 说明 | 处理策略 |
|---------|------|----------|
| 开发错误 | 编程错误 | DET 报告 |
| 运行时错误 | 资源不足 | DEM 记录 |
| 通信错误 | 传输失败 | 重传机制 |
| 硬件错误 | 设备故障 | 看门狗复位 |

## DET (Default Error Tracer)

```c
#if (DET_ERROR_HOOK == STD_ON)
    Det_ReportError(        ModuleId,      /* 模块 ID */
        InstanceId,    /* 实例 ID */
        ApiId,         /* API ID */
        ErrorId        /* 错误 ID */
    );
#endif
```

## DEM (Diagnostic Event Manager)

```c
/* 报告诊断事件 */
Dem_SetEventStatus(    EventId,       /* 事件 ID */
    EventStatus    /* 状态: FAILED/PASSED */
);
```

## 错误恢复策略

### 分层恢复

1. **第一层**: 局部恢复 (重试)
2. **第二层**: 模块重启
3. **第三层**: ECU 重启

### 看门狗监控

```c
WdgM_CheckpointReached(    SEId,          /* 监控实体 ID */
    CheckpointId   /* 检查点 ID */
);
```
