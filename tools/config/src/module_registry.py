"""Central registry for all BSW module configuration schemas."""
from typing import Dict, Any


class ModuleSchema:
    """Schema definition for a single BSW module."""
    def __init__(self, name: str, category: str, config_fields: Dict[str, Dict[str, Any]] = None):
        self.name = name
        self.category = category  # 'MCAL', 'ECUAL', 'Services', 'Complex'
        self.config_fields = config_fields or {}

    def add_field(self, name: str, field_type: str = "int", default: Any = None, description: str = ""):
        self.config_fields[name] = {
            "type": field_type,
            "default": default,
            "description": description,
        }

    def get_params(self) -> list:
        return list(self.config_fields.keys())


_registry: Dict[str, ModuleSchema] = {}


def register_module(name: str, category: str, fields: Dict[str, Dict[str, Any]] = None) -> ModuleSchema:
    """Register a new module schema."""
    from typing import Dict
    schema = ModuleSchema(name, category, fields or {})
    _registry[name] = schema
    return schema


def get_all() -> Dict[str, ModuleSchema]:
    """Return all registered modules."""
    return _registry


def get_module(name: str) -> ModuleSchema:
    """Get a specific module schema by name."""
    return _registry.get(name)


def clear():
    """Clear all registered modules (for testing)."""
    _registry.clear()
