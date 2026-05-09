"""
Base model for ECU Configuration (ECUC) in AUTOSAR.
Provides the foundation for all configuration generators.
"""
from abc import ABC, abstractmethod
from typing import Dict, List, Any, Optional, Union
from dataclasses import dataclass, field, asdict
from enum import Enum, auto
import json


class EcucConfigStatus(Enum):
    """Configuration status enumeration."""
    UNCONFIGURED = auto()
    CONFIGURED = auto()
    VALIDATED = auto()
    GENERATED = auto()


@dataclass
class EcucParameter:
    """Base class for ECUC parameters."""
    name: str
    value: Any
    param_type: str = "STRING"
    min_value: Optional[Any] = None
    max_value: Optional[Any] = None
    default_value: Optional[Any] = None
    description: str = ""
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "value": self.value,
            "type": self.param_type,
            "description": self.description
        }


@dataclass
class EcucReference:
    """ECUC reference to another configuration element."""
    name: str
    target: str
    ref_type: str = "REFERENCE"
    description: str = ""
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "target": self.target,
            "type": self.ref_type,
            "description": self.description
        }


@dataclass
class EcucContainer:
    """ECUC container for grouping parameters."""
    name: str
    short_name: str = ""
    description: str = ""
    parameters: List[EcucParameter] = field(default_factory=list)
    references: List[EcucReference] = field(default_factory=list)
    sub_containers: List['EcucContainer'] = field(default_factory=list)
    
    def add_parameter(self, param: EcucParameter) -> None:
        self.parameters.append(param)
    
    def add_reference(self, ref: EcucReference) -> None:
        self.references.append(ref)
    
    def add_sub_container(self, container: 'EcucContainer') -> None:
        self.sub_containers.append(container)
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "shortName": self.short_name or self.name,
            "description": self.description,
            "parameters": [p.to_dict() for p in self.parameters],
            "references": [r.to_dict() for r in self.references],
            "subContainers": [c.to_dict() for c in self.sub_containers]
        }


class EcucConfigModel(ABC):
    """
    Abstract base class for ECU Configuration models.
    All BSW module generators should inherit from this class.
    """
    
    def __init__(self, module_name: str, version: str = "1.0.0"):
        self.module_name = module_name
        self.version = version
        self.status = EcucConfigStatus.UNCONFIGURED
        self.containers: List[EcucContainer] = []
        self.dependencies: List[str] = []
        self._config_data: Dict[str, Any] = {}
    
    @abstractmethod
    def configure(self, config: Dict[str, Any]) -> None:
        """
        Configure the module with provided configuration data.
        Must be implemented by subclasses.
        """
        pass
    
    @abstractmethod
    def validate(self) -> bool:
        """
        Validate the configuration.
        Must be implemented by subclasses.
        """
        pass
    
    @abstractmethod
    def generate_arxml(self) -> str:
        """
        Generate ARXML configuration output.
        Must be implemented by subclasses.
        """
        pass
    
    def add_container(self, container: EcucContainer) -> None:
        """Add a container to the module configuration."""
        self.containers.append(container)
    
    def add_dependency(self, module_name: str) -> None:
        """Add a module dependency."""
        if module_name not in self.dependencies:
            self.dependencies.append(module_name)
    
    def get_dependencies(self) -> List[str]:
        """Get list of module dependencies."""
        return self.dependencies.copy()
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert configuration to dictionary."""
        return {
            "moduleName": self.module_name,
            "version": self.version,
            "status": self.status.name,
            "dependencies": self.dependencies,
            "containers": [c.to_dict() for c in self.containers]
        }
    
    def to_json(self, indent: int = 2) -> str:
        """Convert configuration to JSON string."""
        return json.dumps(self.to_dict(), indent=indent, ensure_ascii=False)
    
    def get_config_value(self, path: str, default: Any = None) -> Any:
        """
        Get configuration value by path (e.g., 'ComGeneral/ComMaxIPduCnt').
        """
        keys = path.split('/')
        data = self._config_data
        for key in keys:
            if isinstance(data, dict) and key in data:
                data = data[key]
            else:
                return default
        return data
    
    def set_config_value(self, path: str, value: Any) -> None:
        """
        Set configuration value by path.
        """
        keys = path.split('/')
        data = self._config_data
        for key in keys[:-1]:
            if key not in data:
                data[key] = {}
            data = data[key]
        data[keys[-1]] = value
