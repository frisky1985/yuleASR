## Xcp 模块 — 模块审查
- 审查时间: 2026-07-22
- 审查人: 小马 (质量架构师)
- 结论: 有条件通过
- 发现: P1 分类

### 审查范围
- 源文件: `src/bsw/services/xcp/`
- 测试文件: `tests/unit/autosar/services/test_xcp.c`, `tests/unit/xcp/test_xcp.c`
- 规范引用: ASAM XCP V1.5, AUTOSAR_SWS_Xcp

### 审查项

#### ✅ 1. XCP 协议实现
- `Xcp.c`: 核心协议引擎，处理 XCP 命令帧和响应帧
- `_xcp_cmd_std_impl.c`: 标准命令 (CONNECT, DISCONNECT, GET_STATUS, SYNCH, GET_COMM_MODE_INFO, GET_ID, GET_DAQ_CLOCK, GET_DAQ_RESOLUTION_INFO 等)
- `_xcp_cmd_daq_impl.c`: DAQ 命令 (SET_DAQ_LIST_MODE, WRITE_DAQ, SET_DAQ_LIST_RESOLUTION, ALLOC_DAQ, ALLOC_ODT, ALLOC_ODT_ENTRY 等)
- `_xcp_cmd_rest_impl.c`: REST 模式/校准命令 (SET_CAL_PAGE, GET_CAL_PAGE, COPY_CAL_PAGE, SET_SEED, UNLOCK 等)

#### ✅ 2. 传输层支持
- CAN 传输: XCP over CAN (通过 Xcp_Cfg.h 配置)
- 以太网传输: XCP over Ethernet (TCP/UDP)
- 驱动层抽象: Xcp_Cfg.h 中 `XCP_CTO_TRANSPORT_LAYER` 和 `XCP_DTO_TRANSPORT_LAYER` 配置

#### ✅ 3. DAQ 列表支持
- `Xcp_Cfg.h`: `XCP_MAX_DAQ_LIST_COUNT` 可配 (上限 8)
- DAQ 命令完整实现: ALLOC_DAQ, ALLOC_ODT, ALLOC_ODT_ENTRY, FREE_DAQ, SET_DAQ_LIST_MODE, WRITE_DAQ
- DAQ 事件/触发: 支持周期性、信号触发的 DAQ 列表

#### ✅ 4. 校准页面切换
- `_xcp_cmd_rest_impl.c` 中实现: SET_CAL_PAGE, GET_CAL_PAGE, COPY_CAL_PAGE
- 支持双页面校准 (工作页面 + 参考页面)

#### ✅ 5. 测试覆盖
| 测试文件 | 框架 | 状态 |
|---------|------|------|
| test_xcp.c (services) | unity | ❌ Stub 仅，TEST_IGNORE |
| test_xcp.c (unit/xcp) | 未确认 | 需要检查 |

#### ⚠️ 6. 发现项

| ID | 严重度 | 描述 | 位置 |
|----|--------|------|------|
| XCP-P1-001 | P1 | 所有单元测试仅为 stub，无实际测试覆盖 | tests/unit/autosar/services/test_xcp.c |
| XCP-P1-002 | P1 | DAQ ODT 条目超过 MAX_ODT_ENTRY_SIZE 时的缓冲区溢出边界未验证 | Xcp.c: DAQ allocation |
| XCP-P1-003 | P2 | XCP 种子/密钥算法硬编码，不支持外部可配置算法 | _xcp_cmd_rest_impl.c: SET_SEED/UNLOCK |
| XCP-P1-004 | P2 | 校准页面切换中的总线状态恢复处理缺失超时重试 | _xcp_cmd_rest_impl.c: COPY_CAL_PAGE |

### 总体评价
Xcp 模块的协议引擎实现完整，支持 CAN 和以太网传输层，DAQ 和校准功能齐全。但单元测试严重缺失（仅 stub），无法验证协议逻辑的正确性。需要补写 cmocka 测试覆盖核心命令路径。建议优先测试：CONNECT/SYNCH、DAQ 列表分配、校准页面切换。
