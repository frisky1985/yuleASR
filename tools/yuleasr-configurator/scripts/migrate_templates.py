#!/usr/bin/env python3
"""Migrate templates from config/templates/ to tools/yuleasr-configurator/templates/{layer}/."""

import shutil
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
SRC = REPO / "config" / "templates"
DST = REPO / "tools" / "yuleasr-configurator" / "templates"

MCAL = {
    "adc", "can", "crypto", "dio", "eep", "eth", "flash", "fls", "gpt", "i2c",
    "icu", "lin", "mcu", "ocu", "port", "pwm", "ramtst", "spi", "uart", "wdg",
}

ECUAL = {
    "canif", "cannm", "cantp", "cantrcv", "ea", "ethif", "ethsm", "ethswt",
    "ethtrcv", "fee", "frif", "frtp", "iohwab", "ipdum", "j1939tp", "linif",
    "linnm", "linsm", "lintp", "lintrcv", "memif", "someipif", "someipsd",
    "srp", "wdgif",
}

SERVICES = {
    "bswm", "canm", "cansm", "cantsyn", "com", "comm", "crc", "cryif", "csm",
    "dcm", "dem", "det", "dlt", "docan", "doip", "e2e", "ecuc", "ecum",
    "ethtsyn", "fim", "flstst", "j1939nm", "keym", "ldcom", "linm", "lntm",
    "mem", "mqtt", "nm", "nvm", "pdur", "ramsafety", "schm", "sd", "secoc",
    "soad", "someip", "someiptp", "someipxf", "stbm", "swc", "tcpip", "tm",
    "udpnm", "wdgm", "xcp",
}

OS = {"os"}

INTEGRATION = set()  # BswM/EcuM/EcuC 已归入 services; 保留空集合兼容 classify

LAYER_MAP = {}
for mod in MCAL:
    LAYER_MAP[mod.lower()] = "mcal"
for mod in ECUAL:
    LAYER_MAP[mod.lower()] = "ecual"
for mod in SERVICES:
    LAYER_MAP[mod.lower()] = "services"
for mod in OS:
    LAYER_MAP[mod.lower()] = "Os"
for mod in INTEGRATION:
    LAYER_MAP[mod.lower()] = "integration"


def classify(template_name: str) -> str:
    mod = template_name.split("_Cfg")[0]
    return LAYER_MAP.get(mod.lower(), "services")


def main():
    templates = sorted(SRC.glob("*.j2"))
    stats = {"mcal": 0, "ecual": 0, "services": 0, "os": 0, "integration": 0}
    unmapped = []

    for tpl in templates:
        layer = classify(tpl.name)
        dst_file = DST / layer / tpl.name
        shutil.copy2(tpl, dst_file)
        stats[layer] += 1

    total = sum(stats.values())
    print(f"Migrated {total} templates:")
    for layer, count in stats.items():
        print(f"  {layer:12s}: {count}")

    if unmapped:
        print(f"\nUnmapped ({len(unmapped)}):")
        for name in unmapped:
            print(f"  {name}")


if __name__ == "__main__":
    main()
