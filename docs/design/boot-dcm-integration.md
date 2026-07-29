# DCM Integration — Secure Boot UDS

> **UDS 集成到 yuleASR SBL 中进行固件更新**

## 入口

SBL 中的 DCM 模块需要额外处理以下 UDS 服务以支持安全启动更新：

### 0x34 — RequestDownload

```c
/* 在 Dcm_Process_RequestDownload() 中增加: */
if (DFID == 0x01) {  /* SBL 更新 */
    target_slot = BOOT_SBL_ADDR;
    image_type  = BOOT_IMAGE_SBL;
} else if (DFID == 0x02) {  /* App 更新 */
    /* 先检查哪个 slot 当前空闲 */
    target_slot = BOOT_APP_SLOT_B_ADDR;  /* 或对端 slot */
    image_type  = BOOT_IMAGE_APP;
}
Boot_Update_Prepare(target_slot, image_type);
```

### 0x36 — TransferData

```c
/* 每个数据块到达时: */
Boot_Update_WriteBlock(data, block_offset, block_length);
```

### 0x37 — RequestTransferExit

```c
/* 传输完毕, 写 header + trailer, 验证签名: */
Boot_Update_Finalize(image_type, new_version);
/* 返回 NRC 0x00 表示成功, 否则 0x22 (条件不满足) */
```

### 0x31 — RoutineControl (checkIntegrity)

```c
RID = 0xFF01;  /* 自定义 Routine: 校验完整性 */
Boot_Result r = Boot_Loader_ResolveBootTarget();
/* 返回校验结果给 Tester */
```

### 0x11 — ECUReset

```c
/* 执行硬复位触发 PBL → SBL 重新选择 slot */
// 标准 ECUReset 流程
```

## Dcm_Callouts 钩子

在 `src/bsw/services/dcm/src/Dcm.c` 中需要增加的调用点:

| UDS 服务 | 调用点 | Boot API |
|----------|--------|----------|
| 0x34 | Dcm_Process_RequestDownload() | `Boot_Update_Prepare()` |
| 0x36 | Dcm_Process_TransferData() | `Boot_Update_WriteBlock()` |
| 0x37 | Dcm_Process_RequestTransferExit() | `Boot_Update_Finalize()` |
| 0x31 RID=0xFF01 | Dcm_Process_RoutineControl() | `Boot_Loader_ResolveBootTarget()` |
| 0x11 | EcuM + WdgM | `Boot_Loader_ConfirmBoot()` |

## 安全限制

- 仅 `0x10 0x02` (Programming Session) 允许固件写入
- 写入前需要 `0x27` (Security Access) 解锁
- 写入期间 WDG 喂狗间隔 < 100ms
- 验证失败立即擦除目标 slot
