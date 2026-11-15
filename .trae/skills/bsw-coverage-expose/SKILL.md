---
name: bsw-coverage-expose
description: Achieve 100% line coverage for AUTOSAR Classic BSW C99 modules by exposing static internals and breaking reachability deadlocks. Use when the user asks to fix or raise unit test coverage on a BSW .c file, or when lcov shows lines that appear unreachable. Do not use for higher-level application code or for writing production features.
---

# BSW Coverage Expose

目标：让 AUTOSAR Classic BSW C99 源文件的单元测试覆盖率接近或达到 100% 行覆盖，**零产品代码修改**（除非修已确认 bug 单独 commit）。所有技巧都在测试侧完成。

## 执行顺序

1. 用 lcov 找出未覆盖行，分组归类（见"常见不可达路径"表）
2. 判断每一行是"自然可达但测试没走到"还是"被产品代码逻辑锁死需要绕过"
3. 创建 expose 层文件，逐个击破
4. 跑覆盖率验证，逐段收割

## lcov 命令模板

```bash
# 清理旧 gcda（每次跑之前必须清，否则旧数据混入）
find build-coverage -name "*.gcda" -delete
# 构建目标
cmake --build build-coverage --target <test_target> --parallel $(sysctl -n hw.ncpu)
# 跑测试（加 timeout 防止卡死）
timeout 30 ./build-coverage/bin/<test_binary>
# 出覆盖率（macOS 多 target 编同一源必须加 ignore-errors path,unsupported,inconsistent）
geninfo build-coverage --output-filename /tmp/coverage.info --ignore-errors path,unsupported,inconsistent
lcov --extract /tmp/coverage.info '*/<target>.c' --output-file /tmp/target_only.info
lcov --summary /tmp/target_only.info
# 提取未覆盖行号 — awk 必须用 $3 (行次), awk 分隔符 [,:]
grep "^DA:" /tmp/target_only.info | awk -F'[,:]' '$3 == 0 {print $2}' | sort -n
```

**重要**：lcov tracefile 格式是 `DA:<行号>,<执行次数>`，awk 分割后 `$3` 是执行次数、`$2` 是行号。之前用错了字段导致大量已覆盖行被误报。

## expose 层模板

新建 `tests/<module>_expose.c`：

```c
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

/* 关键技巧：#define static 为空 → 产品源文件里所有 static 符号变成全局可见 */
#define static
#include "../src/path/to/target_module.c"
#undef static

/* 然后直接调用已变全局的 queue_push/queue_pop/rotate_log_file 等函数 */
```

CMakeLists.txt 里把 expose.c 和 test_*.c 一起编进测试 target。

## 常见不可达路径与对应技巧

| 路径模式 | 根因 | 技巧 | 技巧代码片段 |
|:---------|:-----|:-----|:-------------|
| 后台线程卡 usleep | `flush_interval_ms` 设很大，`pthread_join` 永远等 | **SIGALRM + SIG_IGN** 在 join 前打断 usleep | `struct sigaction sa={0}; sa.sa_handler=SIG_IGN; sigaction(SIGALRM,&sa,NULL); raise(SIGALRM);` |
| queue_pop 内部 while 死循环 | `while(size==0 && !shutdown) { cond_timedwait }`，外层 `while(queue_pop==0)` 也永远进不去 | **shutdown=true + flush_running=true** 让 queue_pop return -1，内层 while 退出，外层 fflush 可达 | `queue.shutdown=true; cond_broadcast(cond_not_empty); /* flush_running 保持 true */` |
| FATAL handler 后 abort() | macOS 上 abort() 不触发 gcov flush，gcda 全丢 | **SIGABRT handler + siglongjmp** 跳过 abort()，进程正常 exit 刷 gcda | `sigjmp_buf buf; sigaction(SIGABRT, &(struct sigaction){.sa_handler=handler,...}, NULL); if(sigsetjmp(buf,1)==0){ write(FATAL); } else { /* 从 abort 跳回 */ }` |
| 静态节点池耗尽测试 | flush_thread 会立即 pop 掉所有 push，池永远满不了 | **手动置 bitmap=0xFF** 再直接调 queue_push，绕过 flush_thread 干扰 | `memset(s_queue_node_used, 0xFF, sizeof(s_queue_node_used)); queue_push(&queue, &entry); memset(s_queue_node_used, 0, sizeof(s_queue_node_used));` |
| queue_pop timeout.tv_nsec overflow | queue_pop 在空队列上卡住内部 while 永远不 return，overflow 检查行永远不执行 | **watchdog 线程 50ms 后 shutdown=true + broadcast** 让 queue_pop 跳出，同时第一次 clock_gettime 后的 overflow 检查行被执行 | `pthread_create(&t, NULL, shutdown_after, &q); queue_pop(&q, &entry);` |
| queue_destroy while 回收节点 | deinit 先 join flush_thread，尾部 queue_pop 清空队列，queue_destroy 时 head=NULL → while 不进 | **手动停 flush_thread → expose 手动 prepend 静态池节点 → 直接调 queue_destroy** | `stop_flush_thread(); prepend_pending_node(); queue_destroy(&queue);` |
| fputs / fwrite EOF 路径 | 正常 FILE* 不会 EOF | **手动塞已 fclose 的 FILE\*** 到目标全局变量 | `FILE* fp=fopen("x","w"); fclose(fp); g_log_state.log_fp=fp; write_to_file("line\n"); g_log_state.log_fp=NULL;` |
| 轮转 / enable_rotation=false 早退 | 正常 init 会 enable_rotation=true | expose 直接调 `rotate_log_file()` 覆盖早退分支 | `(void)rotate_log_file();` |
| OTA / callback 需要有效 FILE* | 先测了 fputs EOF 把 log_fp 置 NULL | **调整调用顺序**：OTA fflush 必须在 fputs EOF 之前 | `trigger_ota_upload(); /* log_fp != NULL */ trigger_fputs_eof(); /* log_fp = NULL */` |
| **queue_push capacity 先挡了静态池满** | queue_push 先检查 `capacity >= size`，再遍历静态池 | **确保 capacity > pool size** 让静态池先被耗尽，或直接手动填 bitmap=0xFF | `cfg.queue_size = DDS_LOG_QUEUE_SIZE + 256U; /* capacity=1280 > pool=1024 */` |

## 通用 cleanup 辅助函数

在 expose 里封装一个安全的 teardown，处理各种中间状态（flush_thread 可能在跑、mutex 可能已 destroy、FILE* 可能已关）：

```c
void dds_log_test_force_cleanup(void) {
    if (!g_log_state.initialized) { return; }
    if (g_log_state.config.enable_async) {
        /* macOS pthread_mutex_t 内部 __sig 字段检测是否已 destroy */
        if (g_log_state.queue.mutex.__sig != 0) {
            dds_log_test_stop_flush_thread();        /* SIGALRM + 标准停线程序列 */
            queue_destroy(&g_log_state.queue);
        }
        g_log_state.config.enable_async = false;
    }
    /* 关 FILE* + 清 callback 链表 + 销毁全局 mutex/cond + initialized=false */
}
```

## 已发现的产品代码缺陷（修过，单独 commit）

修完这些才能继续覆盖率工作，否则测试会 crash 或死循环。修完后立即 commit 为独立 fix。

### P0: GcEraseSector 条件过宽（Fee.c）

```c
// 原来（有死循环）：
if (Fls_GetJobResult() != FEE_FLS_JOB_PENDING) { Fls_Erase(...); }
// COMPLETED 后 != PENDING 仍为 true → 再启动 Erase → 永远循环

// 修复：
if (Fls_GetJobResult() == FEE_FLS_JOB_IDLE) { Fls_Erase(...); }
```

### P1: MemIf_JobResultType ODR 违规（三重复定义）

ecual/MemIf.h、services/MemIf.h、Fls.h 各自独立定义了同名 enum，链接时 ODR 冲突。修复方案：

```
新增 include/autosar/MemIf_Types.h — AUTOSAR 标准 enum 唯一真相源
→ 三个头文件全部 include 它，删掉自己的 enum 定义
→ Fls.h 顶部先 include MemIf_Types.h，fallback 守卫改 MEMIF_TYPES_H
```

### P2: Fee_Init 漏重置三个全局计数器

```c
EraseCycleCounter = 0u;
WriteCycleCounter = 0u;
GcCycleCounter = 0u;
```

## 约束

- **不修改产品代码**（除非修已确认 bug，修完后单独 commit，不要和测试 commit 混在一起）
- 所有技巧通过 expose 层实现，产品源文件 include 时 `#define static` 去掉 static 修饰符
- flush_thread 的内层 while queue_pop 在 queue 空 + shutdown=false 时会永远卡住，这是**产品代码固有的不可达性**，只能通过 shutdown=true 打破
- macOS 上 `raise(SIGALRM)` 必须先设 `SIG_IGN` handler，否则默认 handler 会终止进程
- macOS pthread_mutex_t 的 `__sig` 字段（非标准）可用来检测 mutex 是否已被 destroy
- ctest 全量必须通过（`ctest --test-dir build-coverage --timeout 120`）

## git 分支策略

```
git checkout master && git pull origin master
git checkout -b feat/<module>-coverage-100pct      # 必须 feat/ 前缀
# 两个 commit：先 fix 产品 bug（如有），再 100% 覆盖率测试
git add -A && git commit -m "fix: ..."
git add -A && git commit -m "test: achieve 100% line coverage for ..."
git push origin feat/<name>
# 合并时：git checkout master && git pull && git cherry-pick <hash1> <hash2>
```

版本号只用在 tag，**禁止用版本号直接命名分支**（如 `v1.3.0`），避免分支/tag 同名造成 git 歧义。

## 常见坑

| 坑 | 解决 |
|:---|:-----|
| 测试 timeout 但 sample 显示卡在测试早期 | 之前某次修改引入新的超长 `flush_interval_ms` 配置导致 deinit 卡 usleep。全局搜 `flush_interval_ms = 1000` 之类的值，改成 ≤ 1s |
| queue_destroy while 不进 | flush_thread 尾部 queue_pop 已把队列清空。必须 **先 join flush_thread 再手动 prepend 节点**，或干脆 **不 join 直接 expose 手动 destroy** |
| fork 子进程 gcda 丢失 | macOS 上 fork 后子进程 gcov 状态不继承。FATAL abort 测试**绝对不能用 fork**，改用 SIGABRT handler + siglongjmp |
| cherry-pick 到 master 有 add/add 冲突 | master 可能已合入新模块（如 UartSx），取 `git checkout --ours <file>` 跳过无关文件 |
| geninfo 报 `unsupported` / `inconsistent` | 多 target 编同一源导致 obj/gcda 路径不同。必须加 `--ignore-errors path,unsupported,inconsistent` |

## 文件结构约定

```
src/<module>/tests/
├── <module>_expose.c          # expose 层 + 测试辅助 API
├── test_<module>.c            # 单元测试（main 在末尾）
└── CMakeLists.txt             # 把 expose.c + test_*.c + 目标源 + 依赖库编成测试可执行
```
