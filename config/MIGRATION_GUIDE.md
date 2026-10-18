# Configuration Migration Guide: JSON → YAML

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
