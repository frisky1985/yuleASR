---
name: vendor-register-integration
description: "Wire vendor register headers (e.g. NXP S32K3 RTD) into AUTOSAR BSW via a zero-churn MCAL compat layer. Invoke when vendor headers must be integrated into platform/MCAL builds."
---

# 厂商寄存器头文件接入（兼容层映射法）

**适用**：已有手写寄存器映射（如 `S32K312.h`）的 AUTOSAR Classic BSW 工程，现在拿到厂商官方寄存器头文件（如 NXP S32K3 RTD 的 `S32K312.h` + 53 个 IP 头），要在**不重写 MCAL 驱动**的前提下，让驱动使用厂商官方基址与寄存器宏，并能编译、可验证。

**核心思路**：厂商头文件（官方真相源） + 一张**兼容映射头**（把工程里旧的 `XXX_BASE` 宏映射到厂商 `IP_*` 宏） + **条件编译开关**（默认关，目标构建开）。这样 MCAL 源码零改动，普通宿主构建行为不变，目标构建切到官方定义。

> 纪律：先摸清依赖集再动代码；`third_party/` 只读不改；默认构建不切换行为；每一步都要有可复现的验证命令。

---

## 0. 前置：拿到并核对厂商头文件

1. 厂商头通常需账号/许可，**不要尝试绕过**；让用户本地解压后上传关键头（主聚合头 + 全部 IP 头 + `*_COMMON.h`）。
2. 大文件（>50MB 的 .exe/.zip）常被沙箱静默丢弃 → 提供本地收集脚本（见 §7）只打包所需 `.h`（通常 2–5MB）。
3. 放到 `third_party/<vendor>/<chip>/include/`，**先不要改内容**。

**命名冲突**：厂商主头常与工程主头同名（如都叫 `S32K312.h`）。将厂商主头重命名（如 `S32K312_RTD.h`），并同步它内部对自身的引用不涉及即可（聚合头一般靠 include guard，不受文件名影响）。

```bash
mkdir -p third_party/<vendor>/<chip>/include
cp /tmp/<extracted>/*.h third_party/<vendor>/<chip>/include/
mv third_party/<vendor>/<chip>/include/<Chip>.h third_party/<vendor>/<chip>/include/<Chip>_RTD.h
```

---

## 1. 侦察：算出真实依赖集（决定工作量）

不要凭感觉。精确算出**工程实际用到的手写宏**，这是兼容头要覆盖的最小集合。

```bash
HDR=src/platform/<chip>/include/<Chip>.h
# 1) 手写头定义的全部宏名
grep -oE '^#define +[A-Za-z0-9_]+' "$HDR" | awk '{print $2}' | sort -u > /tmp/hdr_defs.txt
# 2) 驱动侧使用到的所有标识符
grep -rhoE '\b[A-Za-z_][A-Za-z0-9_]*\b' src/bsw --include='*.c' --include='*.h' | sort -u > /tmp/bsw_ids.txt
# 3) 依赖集 = 交集
comm -12 /tmp/hdr_defs.txt /tmp/bsw_ids.txt > /tmp/used_from_hdr.txt
wc -l /tmp/used_from_hdr.txt
# 4) 测试侧也照做（若供应商宏会全局传播，测试同样走新分支）
grep -rhoE '\b[A-Za-z_][A-Za-z0-9_]*\b' tests --include='*.c' --include='*.h' | sort -u > /tmp/tests_ids.txt
comm -12 /tmp/hdr_defs.txt /tmp/tests_ids.txt > /tmp/used_by_tests.txt
cat /tmp/used_from_hdr.txt /tmp/used_by_tests.txt | sort -u   # 需兼容的总清单
```

**经验值（yuleASR/S32K312）**：手写头定义 382 个宏，驱动只用了 27 个基址 + 少量偏移，合计约 38 个。**绝大多数手写宏是死代码**，不必迁移。

同时摸清驱动访问寄存器的模式：多为「基址宏 + 自身偏移常量 + `REG32(base+off)`」，少数用 `#ifdef <Chip>` 分支（S32K312 分支可能从未被编译过——务必确认 `Chip` 宏是否只在某个子库定义）。

---

## 2. 分类：哪些宏能映射到厂商宏，哪些不能

对照厂商头里 `IP_*_BASE` 与结构体，把依赖集分两类：

| 类别 | 判定 | 处理 |
|:--|:--|:--|
| **厂商等效** | 厂商有同名/对应外设的 `IP_*` 基址 | 映射 `#define LEGACY_X_BASE (IP_X_BASE)` |
| **无等效（合成）** | 厂商无该外设，或驱动寄存器模型与真实 IP 不同 | 保留原值，**注释清楚"非厂商后端"** |

抓厂商基址宏与取值：

```bash
cd third_party/<vendor>/<chip>/include
grep -rhoE '^#define +IP_[A-Za-z0-9_]+_BASE\b' *.h | awk '{print $2}' | sort
grep -rhE '^#define +IP_CAN_0_BASE\b' *.h | head -1
```

**典型陷阱（S32K3 实例）**：
- 手写 CAN0=`0x40050000` 实为 i.MX 风格**错误值**；官方 `IP_CAN_0_BASE=0x40304000`。← 这正是迁移的价值
- `SIUL2` 基址恰好一致；但 `SIUL2_MSCR0_OFF` 手写 `0x20` 是陈旧值，官方 `MSCR` 在 `0x240`。
- **FTM** 在 S32K3 上不存在（真实 IP 为 eMIOS）→ 若 PWM 驱动仍建模 FTM，保留合成值。
- **WDOG** 在 S32K3 上实为 SWT → 保留合成值。
- **MCU** 聚合块实为 MC_ME/MC_CGM/PLL → 保留合成值。
- **HSM** 通过 MU/HSE 访问，非线性寄存器块 → 保留合成值。
- 旧 `SIUL2 GPIO 窗口`（PDOR/PDIR/PDDR @ +0x0/+0x10/+0x14）在 S32K3 不存在（真实为 GPDO/GPDI）→ 保留合成值。

---

## 3. 写兼容头（`<Chip>_LegacyCompat.h`）

放在**工程平台 include 目录**（不是 third_party）。分三段，注释必须说明每段性质：

1. **厂商等效段**：`#define LEGACY_X_BASE (IP_X_BASE)`。
2. **合成段**：保留历史值 + `NOTE: NOT vendor-backed because ...`。
3. **共享偏移/位常量**：能用 `offsetof(IP_Type, member)` 的优先用（顺带修正陈旧值），如
   `#define SIUL2_MSCR0_OFF ((uint32_t)offsetof(SIUL2_Type, MSCR))`。

**必须处理的冲突**：厂商把某些名字定义成**函数式宏**（如 `PIT_MCR_FRZ(x)` 位域辅助），而驱动当**值式宏**用。做法：

```c
#undef PIT_MCR_FRZ
#define PIT_MCR_FRZ (0x00000002U)   /* 恢复驱动的值式语义，注释说明 */
```

写完后用带 `-Wall` 的编译检查是否还有 `redefined` 告警；注意区分**本次引入**与**驱动自身原有**的重定义（后者用关闭厂商宏的方式复现确认）。

多芯片：把「芯片选择」单独放一个头（如 `Platform_ChipSelect.h`），用 `YULE_TARGET_CHIP_<CHIP>` 宏切分支，提供家族宏（S32K31X/32X/34X）与资源计数。

---

## 4. 接线：主头 + 构建开关

**主头**在厂商模式下包含聚合头 + 兼容头；否则保留手写定义（两条路径互斥）：

```c
#if defined(YULE_USE_VENDOR_RTD)
#include "<Chip>_RTD.h"
#include "<Chip>_LegacyCompat.h"
#else
/* 手写映射原样保留（宿主仿真） */
...
#endif
```

**CMake**（默认关，目标构建开；状态行必须**如实**反映是否真正启用）：

```cmake
option(YULE_USE_VENDOR_RTD "Use vendor official register headers (target build)" OFF)
set(YULE_TARGET_CHIP "<Chip>" CACHE STRING "Target chip")

set(_rtd_status "OFF")
if(YULE_USE_VENDOR_RTD)
  string(TOLOWER ${YULE_TARGET_CHIP} _chip_lower)
  set(_dir ${CMAKE_SOURCE_DIR}/third_party/<vendor>/${_chip_lower}/include)
  if(EXISTS ${_dir}/${YULE_TARGET_CHIP}_COMMON.h)
    target_include_directories(Platform PUBLIC ${_dir})
    target_compile_definitions(Platform PUBLIC YULE_USE_VENDOR_RTD YULE_TARGET_CHIP_${YULE_TARGET_CHIP})
    set(_rtd_status "ON (${YULE_TARGET_CHIP})")
  else()
    set(_rtd_status "OFF (headers missing)")
    message(WARNING "YULE_USE_VENDOR_RTD=ON but headers not found at ${_dir}")
  endif()
endif()
message(STATUS "Platform configured: ${YULE_TARGET_CHIP} (vendor: ${_rtd_status})")
```

**关键决策**：不要默认把厂商宏**全局**传播。目标构建才传播；否则宿主单元测试（多半编译的是 `#else` 通用分支）会被打成新分支，行为漂移。若确需全量编译 `<Chip>` 分支，用临时 `-D<Chip>` 语法校验（见 §5），不要改默认构建。

---

## 5. 验证（必须红绿可复现）

### 5.1 驱动目标分支语法编译（本方法的核心验证）

用现有 `compile_commands.json` 的每条命令，追加 `-D<Chip> -DYULE_USE_VENDOR_RTD -I<vendor include> -fsyntax-only` 逐个驱动编译：

```bash
cmake -S . -B /tmp/host -DBUILD_TESTING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
python3 - <<'PY'
import json,re,subprocess,os
cc=json.load(open('/tmp/host/compile_commands.json'))
by={}
for e in cc: by.setdefault(os.path.basename(e['file']),e)
drv=['Adc.c','Can.c','Dio.c','Gpt.c','I2c.c','Mcu.c','Port.c','Pwm.c','Uart.c','Wdg.c']  # 按实际
extra=' -DS32K312 -DYULE_USE_VENDOR_RTD -I/ABS/third_party/<vendor>/<chip>/include -Wall -fsyntax-only'
for n in drv:
    e=by[n]; cmd=re.sub(r'\s-o\s+\S+','',e['command']).replace(' -c ',' ')
    r=subprocess.run(cmd+extra,shell=True,cwd=e['directory'],capture_output=True,text=True)
    errs=[l for l in r.stderr.splitlines() if 'error:' in l]
    print(('OK  ' if r.returncode==0 else 'FAIL'),n,r.returncode,len(errs))
    for l in errs[:3]: print('   ',l[:160])
PY
```

### 5.2 数值断言（证明映射到官方值、偏移被修正）

写一次性小程序（放 `/tmp`，不入库）：`#include "<Chip>.h"`，用 `CK(S32K312_CAN0_BASE==0x40304000UL,...)`、`CK(SIUL2_MSCR0_OFF==offsetof(SIUL2_Type,MSCR),...)` 断言，gcc 编译后运行，期望 `RESULT: PASS`。

### 5.3 厂商头自包含校验

单独 `gcc -std=c99 -Wall -Wextra -I<vendor include> -c` 一个只含聚合头的 TU，确认头文件本身无缺依赖（如 CMSIS）。

### 5.4 两种模式全量构建 + 串行测试

```bash
# 厂商模式
cmake -S . -B /tmp/rtd -DBUILD_TESTING=ON -DYULE_USE_VENDOR_RTD=ON -DYULE_TARGET_CHIP=<Chip>
cmake --build /tmp/rtd -j 6 && ctest --test-dir /tmp/rtd -j 1
# 宿主回归（默认关）
cmake -S . -B /tmp/host -DBUILD_TESTING=ON
cmake --build /tmp/host -j 6 && ctest --test-dir /tmp/host -j 1
```

两者都必须 100% 通过，且改动文件复扫无新增告警。

---

## 6. 交付与汇报

- 分支 `feat/...`；提交信息写清：`third_party` 新增数量、兼容头分类（等效/合成）、`#undef` 冲突处理、验证命令与结果。
- 汇报用结构化中文：**可映射外设 / 合成外设（需后续真 IP 重写）/ 冲突处理 / 验证矩阵（两模式构建+测试）/ 未覆盖风险**。
- 明确风险：驱动 `<Chip>` 分支若从未被单测覆盖，本次仅证明"可编译 + 基址正确"，**不等于功能正确**；FTM/WDOG/Dio-GPIO 等合成模型需按真实 IP 重写。

---

## 7. 大文件获取：本地收集脚本模板

```python
# collect_vendor_headers.py  —— 在用户本地运行，生成小 zip 再上传
import os,sys,zipfile
NEEDED=[...]                       # 缺失的 IP 头文件名列表
def main(root):
    found={}; miss=set(NEEDED)
    for dp,_,fs in os.walk(root):
        for f in fs:
            if f in miss:
                found[f]=os.path.join(dp,f); miss.discard(f)
    with zipfile.ZipFile('vendor_headers.zip','w',zipfile.ZIP_DEFLATED) as z:
        for f,p in sorted(found.items()): z.write(p,f)
    print('found',len(found),'missing',sorted(miss))
main(sys.argv[1])
```

---

## 常见坑速查

| 坑 | 解决 |
|:---|:---|
| 厂商 .exe/.zip 上传后沙箱看不到 | 多半超大小限制；让用户本地解压后只传 `.h`，或用小 zip |
| 厂商头与工程头同名冲突 | 重命名厂商主头为 `<Chip>_RTD.h` |
| 厂商函数式宏与驱动值式宏冲突 | `#undef` 后按驱动语义重定义并注释 |
| 手写基址是错的（i.MX 风格） | 属正常；映射到官方 `IP_*` 即为修正，务必数值断言证明 |
| `<Chip>` 分支其实从未被单测编译 | 用 `-D<Chip>` 语法校验 + offsetof 断言补足；报告如实说明"仅可编译" |
| 厂商宏全局传播导致测试行为漂移 | 默认不全局传播；仅目标构建开启 |
| `/tmp` 被沙箱清理 | 构建目录与 `compile_commands.json` 随时可一键重建 |
| `third_party` 被误改 | 只读；一切适配写在工程侧兼容头 |
