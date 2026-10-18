#!/usr/bin/env python3
"""
Auto-generate minimal config entries for BSW modules missing from bsw_config.json.

This script:
1. Scans src/bsw/{mcal,ecual,services}/ for all modules
2. Loads current bsw_config.json
3. Adds minimal config entries for missing modules
4. Generates minimal _Cfg.h.j2 templates for modules without them
5. Writes updated bsw_config.json and new templates

Usage:
    python config/tools/extend_config_coverage.py
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Set

REPO_ROOT = Path(__file__).resolve().parents[2]
CONFIG_PATH = REPO_ROOT / "config" / "bsw_config.json"
TEMPLATES_DIR = REPO_ROOT / "config" / "templates"
BSW_ROOT = REPO_ROOT / "src" / "bsw"

# Modules that should NOT have auto-generated configs (test/support directories)
EXCLUDE_DIRS = {"include", "src", "config", "test", "common", "scripts"}

# Minimal config template for a module
MINIMAL_CONFIG = {
    "enabled": True,
    "version": "1.0.0",
    "general": {
        "dev_error_detect": True,
        "version_info_api": True
    }
}


def discover_modules() -> Set[str]:
    """Discover all BSW modules from source tree."""
    modules = set()
    for layer in ["mcal", "ecual", "services"]:
        layer_path = BSW_ROOT / layer
        if not layer_path.exists():
            continue
        for module_dir in layer_path.iterdir():
            if module_dir.is_dir() and module_dir.name not in EXCLUDE_DIRS:
                modules.add(module_dir.name)
    return modules


def load_current_config() -> Dict:
    """Load current bsw_config.json."""
    with open(CONFIG_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def save_config(cfg: Dict):
    """Save updated bsw_config.json."""
    with open(CONFIG_PATH, "w", encoding="utf-8") as f:
        json.dump(cfg, f, indent=2, ensure_ascii=False)
        f.write("\n")


def generate_template(module_name: str) -> str:
    """Generate minimal Jinja2 template for a module."""
    # Convert module name to uppercase for macros
    module_upper = module_name.upper()

    # Use string concatenation to avoid f-string/Jinja2 syntax conflicts
    template = """/*
 * Auto-generated configuration header for """ + module_name + """
 * Source: config/bsw_config.json
 * Do not edit manually - regenerate with: python config/tools/generate_bsw_config.py
 */

#ifndef """ + module_upper + """_CFG_H
#define """ + module_upper + """_CFG_H

/* Module enabled: {{ modules.""" + module_name + """.enabled }} */
#define """ + module_upper + """_ENABLED \\
    {% if modules.""" + module_name + """.enabled %}STD_ON{% else %}STD_OFF{% endif %}

/* Development error detection: {{ modules.""" + module_name + """.general.dev_error_detect }} */
#define """ + module_upper + """_DEV_ERROR_DETECT \\
    {% if modules.""" + module_name + """.general.dev_error_detect %}STD_ON{% else %}STD_OFF{% endif %}

/* Version info API: {{ modules.""" + module_name + """.general.version_info_api }} */
#define """ + module_upper + """_VERSION_INFO_API \\
    {% if modules.""" + module_name + """.general.version_info_api %}STD_ON{% else %}STD_OFF{% endif %}

#endif /* """ + module_upper + """_CFG_H */
"""
    return template


def main():
    print("Discovering BSW modules...")
    all_modules = discover_modules()
    print(f"Found {len(all_modules)} modules in source tree")

    print("\nLoading current config...")
    cfg = load_current_config()
    configured_modules = set(cfg.get("modules", {}).keys())
    print(f"Currently configured: {len(configured_modules)} modules")

    missing_modules = all_modules - configured_modules
    print(f"Missing config: {len(missing_modules)} modules")

    if not missing_modules:
        print("\n✓ All modules already have config entries")
        return 0

    print("\nAdding minimal config for missing modules...")
    for module in sorted(missing_modules):
        # Convert module name to proper case (e.g., "CanIf" -> "CanIf")
        # Use the directory name as-is for now
        module_key = module

        cfg["modules"][module_key] = {
            "name": module,
            **MINIMAL_CONFIG
        }
        print(f"  + {module}")

    print(f"\nSaving updated config to {CONFIG_PATH}...")
    save_config(cfg)

    print("\nGenerating minimal templates for modules without them...")
    existing_templates = {p.stem.replace("_Cfg.h", "") for p in TEMPLATES_DIR.glob("*.j2")}
    templates_created = 0

    for module in sorted(missing_modules):
        # Check if template exists (case-insensitive)
        module_upper = module.upper()
        has_template = any(t.upper() == module_upper for t in existing_templates)

        if not has_template:
            template_content = generate_template(module)
            template_path = TEMPLATES_DIR / f"{module}_Cfg.h.j2"
            with open(template_path, "w", encoding="utf-8") as f:
                f.write(template_content)
            templates_created += 1
            print(f"  + {template_path.name}")

    print(f"\n✓ Created {templates_created} new templates")
    print(f"✓ Config coverage: {len(all_modules)}/{len(all_modules)} (100%)")
    print("\nNext steps:")
    print("  1. Review generated config entries in config/bsw_config.json")
    print("  2. Customize module-specific parameters as needed")
    print("  3. Run: python config/tools/generate_bsw_config.py")
    print("  4. Verify generated headers in config/generated/")

    return 0


if __name__ == "__main__":
    sys.exit(main())
