# 数据流设计

## 数据流原则

1. **数据抽象**: 使用 PDU (Protocol Data Unit) 封装数据
2. **零拷贝**: 关键路径使用指针传递
3. **数据一致性**: 使用 E2E 保护关键数据

## PDU 数据结构

```c
typedef struct {
    uint8*  SduDataPtr;      /* 数据指针 */
    uint8*  MetaDataPtr;     /* 元数据指针 */
    PduLengthType SduLength; /* 数据长度 */
} PduInfoType;
```

## 主要数据流

### CAN 通信数据流

```
COM (Signal) → I-PDU → PDUR → L-PDU → CANIF → CAN 驱动
```

**转换过程**:
1. COM: Signal → I-PDU (信号组包)
2. PDUR: I-PDU → L-PDU (路由转发)
3. CANIF: L-PDU → CAN Frame
4. CAN: 发送物理消息

### 诊断数据流

```
DCM → UDS 请求 → PDUR → CANIF → CAN
```

### 安全相关数据流

```
原始数据 → SecOC (加签) → 传输 → SecOC (验签) → 目标数据
```

## 数据缓冲策略

### 静态缓冲

- 编译时确定大小
- 适用于固定大小数据

### 动态缓冲

- 运行时分配
- 适用于可变大小数据 (TP)

## 数据同步机制

1. **中断保护**: 关键操作禁用中断
2. **互斥锁**: 防止重入
3. **双缓冲**: 生产者-消费者模式
