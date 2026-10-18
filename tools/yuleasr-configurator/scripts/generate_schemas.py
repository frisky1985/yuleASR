#!/usr/bin/env python3
"""Generate JSON Schema files for each AUTOSAR BSW module.

Analyzes the existing bsw_config.json and generates per-module schemas
based on the actual configuration structure.
"""

import json
from pathlib import Path
from typing import Any, Dict

REPO = Path(__file__).resolve().parents[3]
CONFIG_FILE = REPO / "config" / "bsw_config.json"
SCHEMA_DIR = REPO / "tools" / "yuleasr-configurator" / "schemas"


def infer_json_schema(value: Any, key: str = "") -> Dict[str, Any]:
    """Infer JSON Schema from a Python value."""
    if isinstance(value, bool):
        return {"type": "boolean"}
    elif isinstance(value, int):
        return {"type": "integer"}
    elif isinstance(value, float):
        return {"type": "number"}
    elif isinstance(value, str):
        return {"type": "string"}
    elif isinstance(value, list):
        if not value:
            return {"type": "array", "items": {}}
        # Infer item type from first element
        item_schema = infer_json_schema(value[0], key)
        return {"type": "array", "items": item_schema}
    elif isinstance(value, dict):
        properties = {}
        required = []
        for k, v in value.items():
            properties[k] = infer_json_schema(v, k)
            # All fields are required by default
            required.append(k)
        return {
            "type": "object",
            "properties": properties,
            "required": required,
        }
    else:
        return {}


def generate_module_schema(module_name: str, module_config: Dict[str, Any]) -> Dict[str, Any]:
    """Generate JSON Schema for a single module."""
    schema = {
        "$schema": "http://json-schema.org/draft-07/schema#",
        "$id": f"https://yuletech.com/schemas/{module_name.lower()}.json",
        "title": f"{module_name} Configuration Schema",
        "description": f"Configuration schema for AUTOSAR {module_name} module",
        "type": "object",
        "properties": {},
        "required": ["enabled"],
    }

    # Add properties from module config
    for key, value in module_config.items():
        if key == "name":
            # Name is a constant
            schema["properties"][key] = {
                "type": "string",
                "const": module_name,
                "description": "Module name",
            }
        elif key == "enabled":
            schema["properties"][key] = {
                "type": "boolean",
                "default": True,
                "description": "Enable/disable module",
            }
        elif key == "version":
            schema["properties"][key] = {
                "type": "string",
                "pattern": r"^\d+\.\d+\.\d+$",
                "description": "Module version (semver)",
            }
        else:
            prop_schema = infer_json_schema(value, key)
            prop_schema["description"] = f"{key.replace('_', ' ').title()}"
            schema["properties"][key] = prop_schema

    return schema


def main():
    """Generate schemas for all modules."""
    print("Loading configuration...")
    with open(CONFIG_FILE) as f:
        config = json.load(f)

    modules = config.get("modules", {})
    print(f"Found {len(modules)} modules")

    # Deduplicate modules (case-insensitive)
    seen = set()
    unique_modules = {}
    for name, cfg in modules.items():
        lower_name = name.lower()
        if lower_name not in seen:
            seen.add(lower_name)
            unique_modules[name] = cfg

    print(f"Unique modules (case-insensitive): {len(unique_modules)}")

    # Generate schemas
    generated = 0
    for module_name, module_config in sorted(unique_modules.items()):
        if not isinstance(module_config, dict):
            continue

        schema = generate_module_schema(module_name, module_config)

        # Determine layer based on module name
        layer = determine_layer(module_name)
        layer_dir = SCHEMA_DIR / layer
        layer_dir.mkdir(parents=True, exist_ok=True)

        schema_file = layer_dir / f"{module_name.lower()}_schema.json"
        with open(schema_file, "w") as f:
            json.dump(schema, f, indent=2)

        generated += 1
        print(f"  ✓ {module_name:12s} -> {layer}/{module_name.lower()}_schema.json")

    print(f"\nGenerated {generated} schema files")


def determine_layer(module_name: str) -> str:
    """Determine AUTOSAR layer for a module."""
    mcal = {"Mcu", "Port", "Dio", "Can", "Spi", "Adc", "Pwm", "gpt", "Wdg", "Icu", "Lin",
            "Eep", "Flash", "Fls", "FlStSt", "I2c", "Uart", "Ocu", "RamSafety", "RamTst"}
    ecual = {"CanIf", "CanTp", "CanM", "CanSM", "CanNm", "CanTrcv", "CanTSyn",
             "LinIf", "LinM", "LinNm", "LinSM", "LinTp", "LinTrcv", "LntM",
             "EthIf", "EthSM", "EthSwt", "EthTrcv", "EthTSyn", "Eth",
             "FrIf", "FrTp", "IoHwAb", "MemIf", "Fee", "Ea",
             "Nm", "UdpNm", "J1939Nm", "J1939Tp", "IpduM", "TcpIp"}
    os_set = {"Os"}
    integration = {"BswM", "EcuM", "EcuC"}

    lower = module_name.lower()
    if lower in mcal:
        return "mcal"
    elif lower in ecual:
        return "ecual"
    elif lower in os_set:
        return "Os"
    elif lower in integration:
        return "integration"
    else:
        return "services"


if __name__ == "__main__":
    main()
