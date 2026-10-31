#!/usr/bin/env python3
"""Migrate templates from config/templates/ to tools/yuleasr-configurator/templates/{layer}/."""

import shutil
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
SRC = REPO / "config" / "templates"
DST = REPO / "tools" / "yuleasr-configurator" / "templates"

MCAL = {
    "Mcu", "Port", "Dio", "Can", "Spi", "Adc", "Pwm", "gpt", "Wdg", "Icu", "Lin",
    "Eep", "Flash", "Fls", "I2c", "Uart", "Ocu", "RamTst", "Crypto", "Eth",
}

ECUAL = {
    "CanIf", "CanTp", "canNm", "CanTrcv",
    "LinIf", "linNm", "linSM", "linTp", "LinTrcv",
    "EthIf", "ethSm", "EthSwt", "EthTrcv",
    "FrIf", "FrTp", "IoHwAb", "MemIf", "Fee", "Ea",
    "J1939Tp", "IpduM", "SomeIpIf", "SomeIpSd", "Srp", "WdgIf",
}

SERVICES = {
    "Com", "comM", "PduR", "NvM", "Dcm", "Dem", "Det", "FiM", "E2E", "Crc",
    "Sd", "SomeIp", "SomeIpTp", "SomeIpXf",
    "SoAd", "doIP", "DoCan", "Dlt", "Xcp",
    "Csm", "CryIf", "KeyM", "SecOC", "StbM",
    "Mqtt", "LdCom", "Tm", "SwC", "SchM",
    # These modules are implemented under src/bsw/services in this repo.
    # AUTOSAR-canonical re-layering is tracked in
    # plans/production-readiness-plan.md (PR-1.2); moving an implementation
    # requires moving its template and updating this map.
    "CanM", "CanSM", "CanTSyn", "EthTSyn", "J1939Nm", "LinM", "LntM",
    "Nm", "udpNm", "TcpIp", "FlStSt", "RamSafety",
    "BswM", "EcuM", "ecuC",
}

OS = {"Os", "Os"}

INTEGRATION = set()

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
    stats = {"mcal": 0, "ecual": 0, "services": 0, "Os": 0, "integration": 0}
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
