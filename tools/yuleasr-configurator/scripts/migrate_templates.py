#!/usr/bin/env python3
"""Migrate templates from config/templates/ to tools/yuleasr-configurator/templates/{layer}/."""

import shutil
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
SRC = REPO / "config" / "templates"
DST = REPO / "tools" / "yuleasr-configurator" / "templates"

MCAL = {
    "mcu", "port", "dio", "can", "spi", "adc", "pwm", "gpt", "wdg", "icu", "lin",
    "eep", "flash", "fls", "flstst", "i2c", "uart", "ocu", "ramsafety", "ramtst",
}

ECUAL = {
    "canif", "cantp", "canm", "cansm", "canNm", "cantrcv", "cantsyn",
    "linif", "linm", "linNm", "linSM", "linTp", "lintrcv", "lntm",
    "ethif", "ethSm", "ethswt", "ethtrcv", "ethtsyn", "eth",
    "frif", "frtp", "iohwab", "memif", "fee", "ea",
    "nm", "udpNm", "j1939nm", "j1939tp", "ipdum", "tcpip",
}

SERVICES = {
    "com", "comM", "pdur", "nvm", "dcm", "dem", "det", "fim", "e2e", "crc",
    "sd", "someip", "someipif", "someipsd", "someiptp", "someipxf",
    "soad", "doIP", "docan", "dlt", "xcp", "wdgif",
    "csm", "cryif", "crypto", "keym", "srp", "SecOC", "stbm",
    "mqtt", "ldcom", "tm", "swc", "schm",
}

OS = {"os", "Os"}

INTEGRATION = {"bswm", "BswM", "ecum", "ecuC"}

LAYER_MAP = {}
for mod in MCAL:
    LAYER_MAP[mod.lower()] = "mcal"
for mod in ECUAL:
    LAYER_MAP[mod.lower()] = "ecual"
for mod in SERVICES:
    LAYER_MAP[mod.lower()] = "services"
for mod in OS:
    LAYER_MAP[mod.lower()] = "os"
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
