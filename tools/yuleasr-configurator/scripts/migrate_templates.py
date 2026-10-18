#!/usr/bin/env python3
"""Migrate templates from config/templates/ to tools/yuleasr-configurator/templates/{layer}/."""

import shutil
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
SRC = REPO / "config" / "templates"
DST = REPO / "tools" / "yuleasr-configurator" / "templates"

MCAL = {
    "Mcu", "Port", "Dio", "Can", "Spi", "Adc", "Pwm", "gpt", "Wdg", "Icu", "Lin",
    "Eep", "Flash", "Fls", "FlStSt", "I2c", "Uart", "Ocu", "RamSafety", "RamTst",
}

ECUAL = {
    "CanIf", "CanTp", "CanM", "CanSM", "canNm", "CanTrcv", "CanTSyn",
    "LinIf", "LinM", "linNm", "linSM", "linTp", "LinTrcv", "LntM",
    "EthIf", "ethSm", "EthSwt", "EthTrcv", "EthTSyn", "Eth",
    "FrIf", "FrTp", "IoHwAb", "MemIf", "Fee", "Ea",
    "Nm", "udpNm", "J1939Nm", "J1939Tp", "IpduM", "TcpIp",
}

SERVICES = {
    "Com", "comM", "PduR", "NvM", "Dcm", "Dem", "Det", "FiM", "E2E", "Crc",
    "Sd", "SomeIp", "SomeIpIf", "SomeIpSd", "SomeIpTp", "SomeIpXf",
    "SoAd", "doIP", "DoCan", "Dlt", "Xcp", "WdgIf",
    "Csm", "CryIf", "Crypto", "KeyM", "Srp", "SecOC", "StbM",
    "Mqtt", "LdCom", "Tm", "SwC", "SchM",
}

OS = {"Os", "Os"}

INTEGRATION = {"BswM", "BswM", "EcuM", "ecuC"}

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
