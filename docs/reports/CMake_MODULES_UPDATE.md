# CMake构建配置更新摘要

## 更新时间
2026-04-29

## 新增模块
本次更新向CMake构建系统中添加了4个新模块:

### 1. Eth驱动 (MCAL层)
- 位置: `src/bsw/mcal/eth/`
- 源文件:
  - `Eth.c` - 主驱动实现
  - `Eth_Irq.c` - 中断处理
- 头文件路径: `src/bsw/mcal/eth/include/`

### 2. Icu驱动 (MCAL层)
- 位置: `src/bsw/mcal/icu/`
- 源文件:
  - `Icu.c` - 主驱动实现
  - `Icu_Irq.c` - 中断处理
  - `Icu_Lcfg.c` - 链接时配置
- 头文件路径: `src/bsw/mcal/icu/include/`

### 3. Ocu驱动 (MCAL层)
- 位置: `src/bsw/mcal/ocu/`
- 源文件:
  - `Ocu.c` - 主驱动实现
  - `Ocu_Irq.c` - 中断处理
- 头文件路径: `src/bsw/mcal/ocu/include/`

### 4. FrTp协议 (ECUAL层)
- 位置: `src/bsw/ecual/frtp/`
- 源文件:
  - `FrTp.c` - 主协议实现
  - `FrTp_Lcfg.c` - 链接时配置
  - `FrTp_PrivUtil.c` - 私有工具函数
  - `FrTp_Rx.c` - 接收处理
  - `FrTp_Tx.c` - 发送处理
  - `FrTp_TxSm.c` - 发送状态机
- 头文件路径: `src/bsw/ecual/frtp/include/`

## 修改的文件

### 1. tools/build/CMakeLists.txt
- 新增4个头文件包含路径
- 新增11个源文件到BSW_SOURCES列表
- 更新了yule_bsw静态库的配置

### 2. tests/CMakeLists.txt
- 新增4个头文件包含路径到测试配置
- 新增11个源文件到MCAL_SOURCES和ECUAL_SOURCES
- 确保测试可以编译新模块

## 依赖关系

```
新模块依赖关系:
┌─────────────────────────────────────────────┐
│  FrTp (FlexRay Transport Protocol)           │
│  └── 依赖: FrIf (FlexRay Interface)         │
├─────────────────────────────────────────────┤
│  Eth (Ethernet Driver)                       │
│  └── 依赖: MCU, PORT                        │
├─────────────────────────────────────────────┤
│  Icu (Input Capture Unit)                    │
│  └── 依赖: MCU                              │
├─────────────────────────────────────────────┤
│  Ocu (Output Compare Unit)                   │
│  └── 依赖: MCU, GPT                         │
└─────────────────────────────────────────────┘
```

## 源文件统计

| 模块 | 源文件数 | 头文件数 | 总代码行数(约) |
|------|---------|---------|---------------|
| Eth  | 2       | 4       | ~350          |
| Icu  | 3       | 4       | ~550          |
| Ocu  | 2       | 4       | ~400          |
| FrTp | 6       | 4       | ~1000         |
| **总计** | **13** | **16** | **~2300** |

## 编译配置

所有新模块使用以下编译选项:
```cmake
target_compile_options(yule_bsw PRIVATE
    -Wall
    -Wextra
    -O2
)
```

## 验证步骤

1. 确保所有源文件存在:
```bash
ls src/bsw/mcal/eth/src/*.c
ls src/bsw/mcal/icu/src/*.c
ls src/bsw/mcal/ocu/src/*.c
ls src/bsw/ecual/frtp/src/*.c
```

2. 确保所有头文件存在:
```bash
ls src/bsw/mcal/eth/include/*.h
ls src/bsw/mcal/icu/include/*.h
ls src/bsw/mcal/ocu/include/*.h
ls src/bsw/ecual/frtp/include/*.h
```

3. 运行CMake配置 (需要安装CMake):
```bash
python3 tools/build/build.py configure
```

4. 构建项目:
```bash
python3 tools/build/build.py build
```

## 注意事项

1. 所有新模块都遵循AutoSAR标准规范
2. 模块头文件已正确配置包含路径
3. 源文件按层次(MCAL/ECUAL)正确分组
4. 依赖关系已通过include_directories正确配置
5. 测试配置已同步更新

## 后续工作

- [ ] 为新模块创建单元测试
- [ ] 添加模块间集成测试
- [ ] 运行静态代码分析
- [ ] 更新API文档
