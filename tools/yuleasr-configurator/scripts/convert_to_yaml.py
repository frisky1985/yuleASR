#!/usr/bin/env python3
"""Convert bsw_config.json to YAML-first format.

This script:
1. Loads the existing JSON configuration
2. Deduplicates modules (case-insensitive, keeps more complete version)
3. Converts to YAML with proper formatting
4. Adds YAML anchors for common patterns
5. Creates a migration guide
"""

import json
from pathlib import Path
from typing import Any, Dict

import yaml

REPO = Path(__file__).resolve().parents[3]
JSON_CONFIG = REPO / "config" / "bsw_config.json"
YAML_CONFIG = REPO / "config" / "bsw_config.yaml"


def deduplicate_modules(modules: Dict[str, Any]) -> Dict[str, Any]:
    """Deduplicate modules case-insensitively, keeping the more complete version."""
    deduped = {}
    
    for name, config in modules.items():
        lower_name = name.lower()
        
        if lower_name not in deduped:
            deduped[lower_name] = (name, config)
        else:
            # Keep the version with more keys
            existing_name, existing_config = deduped[lower_name]
            if len(config) > len(existing_config):
                deduped[lower_name] = (name, config)
    
    # Return with original casing from the kept version
    return {name: config for name, config in deduped.values()}


def add_yaml_anchors(config: Dict[str, Any]) -> Dict[str, Any]:
    """Add YAML anchors for common patterns to reduce duplication."""
    # This is a placeholder for future enhancement
    # For now, just return the config as-is
    # In the future, we could identify common general sections and use anchors
    return config


def convert_json_to_yaml():
    """Main conversion function."""
    print("Loading JSON configuration...")
    with open(JSON_CONFIG) as f:
        config = json.load(f)
    
    print(f"Loaded {len(config.get('modules', {}))} modules")
    
    # Deduplicate modules
    print("Deduplicating modules...")
    original_count = len(config.get('modules', {}))
    config['modules'] = deduplicate_modules(config.get('modules', {}))
    deduped_count = len(config.get('modules', {}))
    print(f"  {original_count} → {deduped_count} modules ({original_count - deduped_count} duplicates removed)")
    
    # Add YAML anchors (future enhancement)
    config = add_yaml_anchors(config)
    
    # Sort modules alphabetically for better readability
    config['modules'] = dict(sorted(config['modules'].items()))
    
    # Write YAML
    print(f"Writing YAML configuration to {YAML_CONFIG.name}...")
    with open(YAML_CONFIG, 'w') as f:
        yaml.dump(
            config,
            f,
            default_flow_style=False,
            sort_keys=False,
            allow_unicode=True,
            width=120,
        )
    
    print(f"✓ Created {YAML_CONFIG}")
    
    # Create migration guide
    create_migration_guide()
    
    return config


def create_migration_guide():
    """Create a migration guide for users."""
    guide_path = REPO / "config" / "MIGRATION_GUIDE.md"
    
    guide_content = """# Configuration Migration Guide: JSON → YAML

## Overview

The yuleASR configuration has been migrated from JSON to YAML format for improved readability and maintainability.

## What Changed

### File Location
- **Old:** `config/bsw_config.json`
- **New:** `config/bsw_config.yaml`

### Module Deduplication
The original JSON configuration contained duplicate modules with different casing (e.g., `Can` and `can`). The YAML configuration has been deduplicated, keeping only the more complete version of each module.

**Before:** 111 modules (with 19 duplicates)  
**After:** 92 unique modules

### YAML Features

The new YAML configuration supports:

1. **Comments** — Add inline documentation
2. **Anchors & Aliases** — Reduce duplication (future enhancement)
3. **Multi-line strings** — Better for descriptions
4. **Relaxed syntax** — No need for quotes around strings

## Migration Steps

### For Users

1. **Update your workflows** to use `bsw_config.yaml` instead of `bsw_config.json`

2. **CLI usage:**
   ```bash
   # Old (still works)
   yuleasr generate -c config/bsw_config.json
   
   # New (recommended)
   yuleasr generate -c config/bsw_config.yaml
   ```

3. **Validation:**
   ```bash
   yuleasr validate -c config/bsw_config.yaml
   ```

### For Developers

1. **Edit the YAML file** directly — it's more readable than JSON

2. **Add comments** to document configuration choices:
   ```yaml
   modules:
     Can:
       enabled: true
       baudrate: 500000  # 500 kbit/s for powertrain CAN
   ```

3. **Use YAML anchors** for common patterns (future):
   ```yaml
   defaults:
     general: &default_general
       dev_error_detect: true
       version_info_api: true
   
   modules:
     Can:
       general: *default_general
   ```

## Backward Compatibility

The old JSON configuration file is still available and supported. However, we recommend migrating to YAML for new projects.

## Validation

Both JSON and YAML configurations are validated against the same JSON Schema files:

```bash
# Validate YAML config
yuleasr validate -c config/bsw_config.yaml -s tools/yuleasr-configurator/schemas

# Validate JSON config (still works)
yuleasr validate -c config/bsw_config.json -s tools/yuleasr-configurator/schemas
```

## Benefits of YAML

1. **Readability** — Easier to read and edit
2. **Comments** — Document your configuration
3. **Less verbose** — No need for quotes and braces
4. **Anchors** — Reuse common patterns (future)
5. **Industry standard** — Widely used in DevOps and cloud configs

## Questions?

If you encounter any issues with the YAML configuration, please:
1. Check the validation output: `yuleasr validate -c config/bsw_config.yaml`
2. Compare with the JSON version: `diff config/bsw_config.json config/bsw_config.yaml`
3. Open an issue on GitHub

---

**Migration Date:** 2026-10-16  
**Migration Tool:** `tools/yuleasr-configurator/scripts/convert_to_yaml.py`
"""
    
    with open(guide_path, 'w') as f:
        f.write(guide_content)
    
    print(f"✓ Created migration guide: {guide_path}")


if __name__ == "__main__":
    convert_json_to_yaml()
