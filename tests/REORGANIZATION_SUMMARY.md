# yuleASR 测试代码整理报告

## 整理时间
2026-05-12

## 整理范围
将分散在各模块的测试文件整合到统一的 tests/ 目录结构中

## 源位置与目标位置对照

| 序号 | 源位置 | 目标位置 | 文件数量 |
|------|--------|----------|----------|
| 1 | src/bsw/mcal/*/test/* | tests/unit/autosar/mcal/ | 2 |
| 2 | src/bsw/services/*/test/* | tests/unit/autosar/services/ | 11 |
| 3 | src/bsw/integration/tests/* | tests/integration/bsw/ | 3 |
| 4 | src/micro-dds/tests/unity/* | tests/unit/middleware/ | 11 |
| 5 | src/diagnostics/dcm/test_*.c | tests/unit/diagnostics/ | 2 |
| 6 | (Unity框架) | tests/unit/framework/ | 2 |

## 整理后的测试目录结构

tests/
├── unit/                          # 单元测试
│   ├── autosar/                   # AUTOSAR模块测试
│   │   ├── mcal/                 # MCAL测试 (2个文件)
│   │   │   ├── Crypto_Test.c
│   │   │   └── test_linslave.c
│   │   ├── ecual/                # ECUAL测试 (已存在)
│   │   └── services/             # Services测试 (11个文件)
│   │       ├── CryIf_Test.c
│   │       ├── Csm_Test.c
│   │       ├── Det_Stub.c/.h
│   │       ├── Mem_Test.c
│   │       ├── RamSafety_test.c
│   │       ├── SecOC_Test.c
│   │       ├── WdgM_Test.c
│   │       ├── test_keym.c
│   │       └── mqtt/               # MQTT测试
│   │           ├── build.sh
│   │           ├── CMakeLists.txt
│   │           ├── README.md
│   │           ├── stubs/
│   │           └── test_mqtt_tls.c
│   ├── middleware/                # 中间件测试 (11个文件)
│   │   ├── test_buffer_pool.c
│   │   ├── test_cdr.c
│   │   ├── test_domain.c
│   │   ├── test_publisher.c
│   │   ├── test_qos.c
│   │   ├── test_reader.c
│   │   ├── test_subscriber.c
│   │   ├── test_topic.c
│   │   ├── test_writer.c
│   │   ├── unity.c
│   │   └── unity.h
│   ├── diagnostics/               # 诊断模块测试 (2个文件)
│   │   ├── test_io_control.c
│   │   └── test_write_data_by_identifier.c
│   └── framework/                 # 测试框架 (2个文件)
│       ├── unity.c
│       └── unity.h
├── integration/                   # 集成测试
│   ├── bsw/                       # BSW集成测试 (4个文件)
│   │   ├── integration_test.c
│   │   ├── integration_test_cfg.h
│   │   ├── test_ecum_bswm_integration.c (已存在)
│   │   └── test_runner.c
│   └── system/                    # 系统测试 (已存在)
└── resources/                     # 测试资源
    ├── arxml/                     # ARXML测试数据 (空)
    ├── dbc/                       # DBC测试数据 (空)
    └── csv/                       # CSV测试数据 (空)

## 移动的文件详情

### 1. MCAL测试 (tests/unit/autosar/mcal/)
- Crypto_Test.c (从 src/bsw/mcal/crypto/test/)
- test_linslave.c (从 src/bsw/mcal/lin/test/)

### 2. Services测试 (tests/unit/autosar/services/)
- CryIf_Test.c (从 src/bsw/services/cryif/test/)
- Csm_Test.c (从 src/bsw/services/csm/test/)
- Det_Stub.c/.h (从 src/bsw/services/keym/test/)
- test_keym.c (从 src/bsw/services/keym/test/)
- Mem_Test.c (从 src/bsw/services/mem/test/)
- RamSafety_test.c (从 src/bsw/services/ramsafety/test/)
- SecOC_Test.c (从 src/bsw/services/secoc/test/)
- WdgM_Test.c (从 src/bsw/services/wdgm/test/)
- mqtt/ 目录 (从 src/bsw/services/mqtt/test/)

### 3. 诊断测试 (tests/unit/diagnostics/)
- test_io_control.c (从 src/diagnostics/dcm/)
- test_write_data_by_identifier.c (从 src/diagnostics/dcm/)

### 4. 中间件测试 (tests/unit/middleware/)
- test_buffer_pool.c
- test_cdr.c
- test_domain.c
- test_publisher.c
- test_qos.c
- test_reader.c
- test_subscriber.c
- test_topic.c
- test_writer.c
- unity.c/.h (从 src/micro-dds/tests/unity/)

### 5. 测试框架 (tests/unit/framework/)
- unity.c/.h (Unity测试框架核心)

### 6. BSW集成测试 (tests/integration/bsw/)
- integration_test.c (从 src/bsw/integration/tests/)
- integration_test_cfg.h (从 src/bsw/integration/tests/)
- test_runner.c (从 src/bsw/integration/tests/)

## 备注

1. 所有测试文件内容保持不变，仅移动到新位置
2. Unity框架文件同时存在于 tests/unit/framework/ 和 tests/unit/middleware/
3. tests/resources/ 下的子目录为预留结构，目前为空
4. 原始测试文件在源位置保留，未删除

## 统计

- 新增目录: 10个
- 移动文件总数: 31个
  - .c 文件: 24个
  - .h 文件: 5个
  - 其他: 2个
