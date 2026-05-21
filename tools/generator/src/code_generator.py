#!/usr/bin/env python3
"""
YuleTech BSW Code Generator v2.0

基于模块化配置注册表 + Jinja2 模板生成 AutoSAR BSW 配置头文件。
支持所有注册的 30 个 BSW 模块。

用法:
    python code_generator.py <config.json> [output_dir]
    python code_generator.py --auto           # 使用默认配置生成所有模块
"""

import json
import os
import sys
from pathlib import Path
from typing import Dict, Any, Optional, List, Tuple
from jinja2 import Environment, FileSystemLoader, Undefined
from datetime import datetime


# 模板目录相对于本文件的位置
TEMPLATES_DIR = Path(__file__).resolve().parent.parent / "templates"

# 默认输出目录
DEFAULT_OUTPUT = Path("./generated")


# ======================================================================
# 默认配置 —— 覆盖所有 30 个 BSW 模块
# ======================================================================
DEFAULT_CONFIG: Dict[str, Dict[str, Any]] = {
    # ---- MCAL ----
    "Port": {
        "PortPinMode": 0, "PortPinDirection": 0, "PortPinLevelValue": 0,
        "PortPinOutputType": 0, "PortVersionInfoApi": False,
        "PortMajorVersion": 4, "PortMinorVersion": 4, "PortRevisionVersion": 0,
        "PortDevErrorDetect": True,
    },
    "Dio": {
        "DioChannelGroupCount": 1, "DioVersionInfoApi": False,
        "DioFlipChannelApi": False, "DioMaskedWritePortApi": False,
        "DioMajorVersion": 4, "DioMinorVersion": 4, "DioRevisionVersion": 0,
        "DioDevErrorDetect": True,
    },
    "Can": {
        "CanControllerCount": 2, "CanBaudrate": 500000, "CanVersionInfoApi": False,
        "CanSetBaudrateApi": False, "CanMainFunctionPeriod": 0.01,
        "CanMajorVersion": 4, "CanMinorVersion": 4, "CanRevisionVersion": 0,
        "CanDevErrorDetect": True,
    },
    "Lin": {
        "LinChannelCount": 1, "LinBaudrate": 19200, "LinVersionInfoApi": False,
        "LinWakeupSupport": True,
        "LinMajorVersion": 4, "LinMinorVersion": 4, "LinRevisionVersion": 0,
        "LinDevErrorDetect": True,
    },
    "Spi": {
        "SpiChannelCount": 1, "SpiJobCount": 1, "SpiSequenceCount": 1,
        "SpiVersionInfoApi": False,
        "SpiMajorVersion": 4, "SpiMinorVersion": 4, "SpiRevisionVersion": 0,
        "SpiDevErrorDetect": True,
    },
    "Gpt": {
        "GptChannelCount": 4, "GptPrescaler": 1, "GptVersionInfoApi": False,
        "GptWakeupFunctionality": False,
        "GptMajorVersion": 4, "GptMinorVersion": 4, "GptRevisionVersion": 0,
        "GptDevErrorDetect": True,
    },
    "Mcu": {
        "McuClockSettingCount": 1, "McuRamSectors": 1, "McuDefaultSpeed": 16000000,
        "McuVersionInfoApi": False,
        "McuMajorVersion": 4, "McuMinorVersion": 4, "McuRevisionVersion": 0,
        "McuDevErrorDetect": True,
    },
    "Adc": {
        "AdcChannelCount": 4, "AdcResolution": 12, "AdcGroupCount": 1,
        "AdcVersionInfoApi": False,
        "AdcMajorVersion": 4, "AdcMinorVersion": 4, "AdcRevisionVersion": 0,
        "AdcDevErrorDetect": True,
    },
    "Icu": {
        "IcuChannelCount": 4, "IcuVersionInfoApi": False, "IcuSignalMeasurement": False,
        "IcuMajorVersion": 4, "IcuMinorVersion": 4, "IcuRevisionVersion": 0,
        "IcuDevErrorDetect": True,
    },
    "Pwm": {
        "PwmChannelCount": 4, "PwmPeriod": 1000, "PwmVersionInfoApi": False,
        "PwmMajorVersion": 4, "PwmMinorVersion": 4, "PwmRevisionVersion": 0,
        "PwmDevErrorDetect": True,
    },
    "Fls": {
        "FlsSectorCount": 8, "FlsPageSize": 256, "FlsVersionInfoApi": False,
        "FlsMajorVersion": 4, "FlsMinorVersion": 4, "FlsRevisionVersion": 0,
        "FlsDevErrorDetect": True,
    },
    "Crc": {
        "CrcComputationWidth": 32, "CrcVersionInfoApi": False, "CrcHwUnitCount": 1,
        "CrcMajorVersion": 4, "CrcMinorVersion": 4, "CrcRevisionVersion": 0,
        "CrcDevErrorDetect": True,
    },
    # ---- ECUAL ----
    "CanIf": {
        "CanIfMaxRxPduCount": 16, "CanIfMaxTxPduCount": 16,
        "CanIfVersionInfoApi": False, "CanIfPublicCanConfigSet": 0,
        "CanIfDevErrorDetect": True,
        "CanIfMajorVersion": 4, "CanIfMinorVersion": 4, "CanIfRevisionVersion": 0,
    },
    "CanTp": {
        "CanTpMaxSduCount": 4, "CanTpChannelCount": 1,
        "CanTpRxBufferSize": 256, "CanTpTxBufferSize": 256,
        "CanTpVersionInfoApi": False,
        "CanTpMajorVersion": 4, "CanTpMinorVersion": 4, "CanTpRevisionVersion": 0,
        "CanTpDevErrorDetect": True,
    },
    "EthIf": {
        "EthIfControllerCount": 1, "EthIfRxBufferSize": 2048,
        "EthIfTxBufferSize": 2048, "EthIfVersionInfoApi": False,
        "EthIfDevErrorDetect": True,
        "EthIfMajorVersion": 4, "EthIfMinorVersion": 4, "EthIfRevisionVersion": 0,
    },
    "IoHwAb": {
        "IoHwAbChannelCount": 8, "IoHwAbPortPinCount": 16,
        "IoHwAbVersionInfoApi": False, "IoHwAbDevErrorDetect": True,
        "IoHwAbMajorVersion": 4, "IoHwAbMinorVersion": 4, "IoHwAbRevisionVersion": 0,
    },
    "MemIf": {
        "MemIfJobCount": 1, "MemIfVersionInfoApi": False, "MemIfDevErrorDetect": True,
        "MemIfMajorVersion": 4, "MemIfMinorVersion": 4, "MemIfRevisionVersion": 0,
    },
    "Fee": {
        "FeeBlockCount": 16, "FeePageSize": 256, "FeeVersionInfoApi": False,
        "FeeDevErrorDetect": True,
        "FeeMajorVersion": 4, "FeeMinorVersion": 4, "FeeRevisionVersion": 0,
    },
    "Ea": {
        "EaBlockCount": 8, "EaPageSize": 128, "EaVersionInfoApi": False,
        "EaDevErrorDetect": True,
        "EaMajorVersion": 4, "EaMinorVersion": 4, "EaRevisionVersion": 0,
    },
    "FrIf": {
        "FrIfChannelCount": 1, "FrIfControllerCount": 1,
        "FrIfVersionInfoApi": False, "FrIfDevErrorDetect": True,
        "FrIfMajorVersion": 4, "FrIfMinorVersion": 4, "FrIfRevisionVersion": 0,
    },
    "LinIf": {
        "LinIfChannelCount": 1, "LinIfScheduleTableCount": 2,
        "LinIfVersionInfoApi": False, "LinIfDevErrorDetect": True,
        "LinIfMajorVersion": 4, "LinIfMinorVersion": 4, "LinIfRevisionVersion": 0,
    },
    # ---- Services ----
    "Com": {
        "ComMaxPduCount": 32, "ComMaxSignalCount": 128, "ComVersionInfoApi": False,
        "ComDevErrorDetect": True, "ComTimeBasedTxMode": False,
        "ComMajorVersion": 4, "ComMinorVersion": 4, "ComRevisionVersion": 0,
    },
    "PduR": {
        "PduRMaxRoutingTableCount": 32, "PduRVersionInfoApi": False,
        "PduRDevErrorDetect": True,
        "PduRMajorVersion": 4, "PduRMinorVersion": 4, "PduRRevisionVersion": 0,
    },
    "NvM": {
        "NvMBlockCount": 32, "NvMRomBlockCount": 32, "NvMRamBlockCount": 32,
        "NvMVersionInfoApi": False, "NvMDevErrorDetect": True,
        "NvMMajorVersion": 4, "NvMMinorVersion": 4, "NvMRevisionVersion": 0,
    },
    "Dcm": {
        "DcmMaxPduCount": 8, "DcmMaxSidCount": 32, "DcmVersionInfoApi": False,
        "DcmDevErrorDetect": True, "DcmDspUdsOnCan": True,
        "DcmMajorVersion": 4, "DcmMinorVersion": 4, "DcmRevisionVersion": 0,
    },
    "Dem": {
        "DemMaxEventCount": 64, "DemMaxDebounceCount": 16,
        "DemOperationCycleCount": 4, "DemVersionInfoApi": False,
        "DemDevErrorDetect": True,
        "DemMajorVersion": 4, "DemMinorVersion": 4, "DemRevisionVersion": 0,
    },
    "EcuM": {
        "EcuMMaxWakeupSources": 4, "EcuMShutdownTarget": 0,
        "EcuMVersionInfoApi": False, "EcuMDevErrorDetect": True,
        "EcuMMajorVersion": 4, "EcuMMinorVersion": 4, "EcuMRevisionVersion": 0,
    },
    "BswM": {
        "BswMMaxModeCount": 8, "BswMMaxLogicExpressionCount": 16,
        "BswMVersionInfoApi": False, "BswMDevErrorDetect": True,
        "BswMMajorVersion": 4, "BswMMinorVersion": 4, "BswMRevisionVersion": 0,
    },
    "WdgM": {
        "WdgMMaxSupervisedEntities": 4, "WdgMMaxAliveSupervisionCycles": 8,
        "WdgMMaxDeadlineSupervisionCycles": 8,
        "WdgMVersionInfoApi": False, "WdgMDevErrorDetect": True,
        "WdgMMajorVersion": 4, "WdgMMinorVersion": 4, "WdgMRevisionVersion": 0,
    },
    "WdgIf": {
        "WdgIfMaxConfiguredTriggers": 1, "WdgIfVersionInfoApi": False,
        "WdgIfDevErrorDetect": True,
        "WdgIfMajorVersion": 4, "WdgIfMinorVersion": 4, "WdgIfRevisionVersion": 0,
    },
}


# ======================================================================
# 模板→模块名映射工具
# ======================================================================
KNOWN_MODULES = {
    "wdgif": "WdgIf", "bswm": "BswM", "dem": "Dem",
    "ecum": "EcuM", "wdgm": "WdgM", "mcu": "Mcu",
    "can": "Can", "canif": "CanIf", "cantp": "CanTp",
    "ethif": "EthIf", "iohwab": "IoHwAb", "memif": "MemIf",
    "fee": "Fee", "ea": "Ea", "frif": "FrIf", "linif": "LinIf",
    "com": "Com", "pdur": "PduR", "nvm": "NvM", "dcm": "Dcm",
    "port": "Port", "dio": "Dio", "lin": "Lin", "spi": "Spi",
    "gpt": "Gpt", "adc": "Adc", "icu": "Icu", "pwm": "Pwm",
    "fls": "Fls", "crc": "Crc",
}

def module_from_template(tpl_rel_path: str) -> str:
    """从模板相对路径推断模块名"""
    stem = Path(tpl_rel_path.replace('\\', '/')).stem  # wdgif_cfg.h
    base = Path(stem).stem          # wdgif_cfg
    name = base.replace("_cfg", "").replace("_Cfg", "").replace("_", "")
    lower = name.lower()
    return KNOWN_MODULES.get(lower, name.capitalize())


# ======================================================================
# 代码生成器
# ======================================================================
class CodeGenerator:
    """模板驱动的代码生成器"""

    def __init__(self, config_path: Optional[str] = None, output_dir: str = "./generated"):
        self.config_path = Path(config_path) if config_path else None
        self.output_path = Path(output_dir)
        self.templates_dir = TEMPLATES_DIR
        self.config_data: Dict[str, Dict[str, Any]] = {}
        self.generated = 0
        self.skipped = 0

    # --------------------------------------------------------------
    def load_config(self) -> bool:
        """加载配置：JSON → 默认值回退"""
        if self.config_path and self.config_path.exists():
            try:
                with open(self.config_path, 'r', encoding='utf-8') as f:
                    raw = json.load(f)
                modules_raw = raw.get('modules', raw)
                self.config_data = {}
                for mod_name, mod_cfg in modules_raw.items():
                    defaults = DEFAULT_CONFIG.get(mod_name, {})
                    merged = dict(defaults)
                    for k, v in mod_cfg.items():
                        if k not in ('name', 'enabled'):
                            merged[k] = v
                    self.config_data[mod_name] = merged
                print(f"[OK] Loaded config: {self.config_path} ({len(self.config_data)} modules)")
                return True
            except Exception as e:
                print(f"[WARN] Config load failed: {e}. Using defaults.")
        self.config_data = dict(DEFAULT_CONFIG)
        print(f"[OK] Using default config for {len(self.config_data)} modules")
        return True

    def get_module_config(self, name: str) -> Dict[str, Any]:
        """合并默认 + 覆盖 + 注入生成日期"""
        merged = dict(DEFAULT_CONFIG.get(name, {}))
        merged.update(self.config_data.get(name, {}))
        merged['generation_date'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        return merged

    # --------------------------------------------------------------
    def discover_templates(self) -> List[Tuple[str, str]]:
        """扫描 templates/ 目录，返回 [(tpl_rel_path, module_name)]"""
        results = []
        if not self.templates_dir.exists():
            print(f"[WARN] Templates dir not found: {self.templates_dir}")
            return results
        for root, dirs, files in os.walk(self.templates_dir):
            for f in sorted(files):
                if f.endswith('.j2'):
                    full = Path(root) / f
                    rel = str(full.relative_to(self.templates_dir)).replace(os.sep, '/')
                    mod = module_from_template(rel)
                    results.append((rel, mod))
        return results

    # --------------------------------------------------------------
    def generate_one(self, tpl_rel: str, module: str) -> bool:
        """生成单个模块头文件"""
        try:
            cfg = self.get_module_config(module)
            if not cfg:
                print(f"  [SKIP] {module}: no config")
                self.skipped += 1
                return True

            env = Environment(
                loader=FileSystemLoader(str(self.templates_dir)),
                undefined=Undefined
            )
            tmpl = env.get_template(tpl_rel)
            content = tmpl.render(module=cfg)

            out_name = Path(tpl_rel).stem  # wdgif_cfg.h
            out_file = self.output_path / out_name
            out_file.parent.mkdir(parents=True, exist_ok=True)
            with open(out_file, 'w', encoding='utf-8') as f:
                f.write(content)

            print(f"  [OK] {module:10s} -> {out_file}")
            self.generated += 1
            return True
        except Exception as e:
            print(f"  [FAIL] {module}: {e}")
            return False

    def generate_all(self) -> bool:
        """扫描模板并全部生成"""
        print("=" * 55)
        print("  YuleTech BSW Code Generator v2.0")
        print("  Template-driven | 30 modules supported")
        print("=" * 55)
        if not self.load_config():
            return False

        tpls = self.discover_templates()
        if not tpls:
            print("[FAIL] No templates found!")
            return False

        print(f"\nFound {len(tpls)} templates:\n")
        for rel, mod in tpls:
            print(f"  [{mod:10s}] {rel}")

        print(f"\n{'─' * 55}")
        print(f"Output -> {self.output_path.resolve()}\n")

        ok = True
        for rel, mod in tpls:
            if not self.generate_one(rel, mod):
                ok = False

        print(f"\n{'─' * 55}")
        print(f"  Generated: {self.generated} | Skipped: {self.skipped}")
        print(f"  Output:    {self.output_path.resolve()}")
        print(f"  Status:    {'[OK]' if ok else '[WARN]'}")
        print(f"{'─' * 55}")
        return ok


def main():
    """CLI 入口"""
    args = sys.argv[1:]

    if not args or '--help' in args or '-h' in args:
        print("Usage:")
        print("  python code_generator.py <config.json> [output_dir]")
        print("  python code_generator.py --auto          [output_dir]")
        print("  python code_generator.py --list")
        return 1

    if args[0] == '--list':
        gen = CodeGenerator()
        for rel, mod in gen.discover_templates():
            print(f"  {mod:12s} <- {rel}")
        return 0

    if args[0] == '--auto':
        output = args[1] if len(args) > 1 else "./generated"
        gen = CodeGenerator(output_dir=output)
    else:
        cfg = args[0]
        output = args[1] if len(args) > 1 else "./generated"
        gen = CodeGenerator(config_path=cfg, output_dir=output)

    return 0 if gen.generate_all() else 1


if __name__ == "__main__":
    exit(main())
