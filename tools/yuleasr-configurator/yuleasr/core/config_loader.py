"""YAML/JSON configuration loader with merge and include support."""

from __future__ import annotations

import json
import logging
from pathlib import Path
from typing import Any, Dict, List, Optional, Union

import yaml

LOG = logging.getLogger(__name__)


class ConfigLoadError(Exception):
    """Raised when configuration cannot be loaded or parsed."""


def load_config(path: Path) -> Dict[str, Any]:
    """Load a YAML or JSON configuration file.

    Supports ``.yaml``/``.yml`` and ``.json`` extensions.  Raises
    ``ConfigLoadError`` on missing files, parse errors, or unsupported
    formats.
    """
    if not path.exists():
        raise ConfigLoadError(f"config file not found: {path}")

    suffix = path.suffix.lower()
    try:
        with open(path, "r", encoding="utf-8") as f:
            if suffix in (".yaml", ".yml"):
                data = yaml.safe_load(f)
            elif suffix == ".json":
                data = json.load(f)
            else:
                raise ConfigLoadError(f"unsupported config format: {suffix}")
    except yaml.YAMLError as e:
        raise ConfigLoadError(f"YAML parse error in {path}: {e}")
    except json.JSONDecodeError as e:
        raise ConfigLoadError(f"JSON parse error in {path}: {e}")
    except OSError as e:
        raise ConfigLoadError(f"cannot read {path}: {e}")

    if not isinstance(data, dict):
        raise ConfigLoadError(f"top-level config must be a mapping, got {type(data).__name__}")

    _resolve_includes(data, path.parent)
    return data


def _resolve_includes(data: Dict[str, Any], base_dir: Path) -> None:
    """Recursively resolve ``$include`` directives in-place.

    An include directive looks like::

        $include: "relative/path/to/file.yaml"

    The included file's keys are merged into the current mapping.  Keys in
    the including file take precedence (shallow merge).
    """
    include_ref = data.pop("$include", None)
    if include_ref is None:
        return

    if isinstance(include_ref, str):
        include_ref = [include_ref]

    for ref in include_ref:
        inc_path = (base_dir / ref).resolve()
        if not inc_path.exists():
            raise ConfigLoadError(f"included file not found: {inc_path}")
        included = load_config(inc_path)
        _deep_merge(included, data)


def _deep_merge(base: Dict[str, Any], override: Dict[str, Any]) -> None:
    """Merge *base* into *override* in-place (override wins on conflict)."""
    for key, value in override.items():
        if key in base:
            if isinstance(base[key], dict) and isinstance(value, dict):
                _deep_merge(base[key], value)
            else:
                base[key] = value
        else:
            base[key] = value


def merge_configs(configs: List[Dict[str, Any]]) -> Dict[str, Any]:
    """Merge multiple config dicts (later entries override earlier ones)."""
    if not configs:
        return {}
    result = configs[0].copy()
    for cfg in configs[1:]:
        _deep_merge(result, cfg)
    return result


def dump_yaml(data: Dict[str, Any], path: Path) -> None:
    """Write config as YAML."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        yaml.dump(data, f, default_flow_style=False, sort_keys=False, allow_unicode=True)


def dump_json(data: Dict[str, Any], path: Path) -> None:
    """Write config as JSON."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
