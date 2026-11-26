#!/usr/bin/env python3
"""
本地脚本：从 NXP RTD 安装目录收集 S32K312 缺失的 IP 头文件并打包。

用法（在您的本地电脑上运行）：
  python collect_rtd_headers.py <RTD安装目录>

示例：
  python collect_rtd_headers.py "C:\\NXP\\SW32K3_S32M27x_RTD_R21-11_6.0.0"
  python collect_rtd_headers.py "D:\\S32DS\\S32K312_RTD"

输出：
  在当前目录生成 rtd_s32k312_headers.zip，然后通过 IDE 上传此 zip 文件。
"""
import os
import sys
import zipfile

# 需要收集的 40 个缺失文件
MISSING_FILES = [
    "S32K312_EIM.h", "S32K312_EMIOS.h", "S32K312_ERM.h",
    "S32K312_FCCU.h", "S32K312_FIRC.h", "S32K312_FLASH.h",
    "S32K312_FLEXCAN.h", "S32K312_FLEXIO.h", "S32K312_FXOSC.h",
    "S32K312_INTM.h", "S32K312_JDC.h", "S32K312_LCU.h",
    "S32K312_LPCMP.h", "S32K312_LPI2C.h", "S32K312_LPSPI.h",
    "S32K312_LPUART.h", "S32K312_MCM_CM7.h", "S32K312_MC_CGM.h",
    "S32K312_MC_ME.h", "S32K312_MC_RGM.h", "S32K312_MDM_AP.h",
    "S32K312_MSCM.h", "S32K312_MU.h", "S32K312_PFLASH.h",
    "S32K312_PIT.h", "S32K312_PLL.h", "S32K312_PMC.h",
    "S32K312_PRAMC.h", "S32K312_RTC.h", "S32K312_SDA_AP.h",
    "S32K312_SIRC.h", "S32K312_SIUL2.h", "S32K312_STCU.h",
    "S32K312_STM.h", "S32K312_SWT.h", "S32K312_SXOSC.h",
    "S32K312_NVIC.h", "S32K312_SCB.h", "S32K312_SYSTICK.h",
    "S32K312_MPU.h",
]


def find_files(root_dir):
    """在指定目录下递归搜索所有缺失的头文件"""
    found = {}
    missing_set = set(MISSING_FILES)

    for dirpath, _dirnames, filenames in os.walk(root_dir):
        for fn in filenames:
            if fn in missing_set:
                full_path = os.path.join(dirpath, fn)
                found[fn] = full_path
                missing_set.discard(fn)
                print(f"  [OK] {fn}")
                print(f"       -> {full_path}")

    return found, sorted(missing_set)


def main():
    if len(sys.argv) < 2:
        print("用法: python collect_rtd_headers.py <RTD安装目录>")
        print()
        print("请指定 NXP RTD 的安装目录，例如:")
        print('  python collect_rtd_headers.py "C:\\NXP\\SW32K3_S32M27x_RTD_R21-11_6.0.0"')
        sys.exit(1)

    root = sys.argv[1]
    if not os.path.isdir(root):
        print(f"错误: 目录不存在: {root}")
        sys.exit(1)

    print(f"搜索目录: {root}")
    print(f"需要找到 {len(MISSING_FILES)} 个头文件...")
    print()

    found, still_missing = find_files(root)

    print()
    print(f"找到: {len(found)}/{len(MISSING_FILES)}")

    if still_missing:
        print(f"\n以下 {len(still_missing)} 个文件未找到:")
        for f in still_missing:
            print(f"  [MISS] {f}")

    if not found:
        print("\n未找到任何文件，请检查 RTD 安装目录是否正确。")
        print("头文件通常位于: BaseNXP/header/ 或 Platform/BaseNXP/header/")
        sys.exit(1)

    # 打包
    zip_name = "rtd_s32k312_headers.zip"
    with zipfile.ZipFile(zip_name, "w", zipfile.ZIP_DEFLATED) as zf:
        for fn, full_path in sorted(found.items()):
            zf.write(full_path, fn)

    size_kb = os.path.getsize(zip_name) / 1024
    print(f"\n已生成: {zip_name} ({size_kb:.0f} KB)")
    print(f"包含 {len(found)} 个头文件")
    print()
    print("请通过 IDE 上传此 zip 文件到沙箱。")


if __name__ == "__main__":
    main()
