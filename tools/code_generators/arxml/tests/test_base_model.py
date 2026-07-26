#!/usr/bin/env python3
"""Targeted tests for base_model.py"""

import sys
import os
import unittest
import json
from pathlib import Path
from typing import Dict, Any

sys.path.insert(0, str(Path(__file__).parent.parent))

from base_model import (
    EcucConfigModel, EcucConfigStatus, EcucContainer, 
    EcucParameter, EcucReference
)


class ConcreteConfigModel(EcucConfigModel):
    """Concrete implementation for testing abstract base"""
    def configure(self, config: Dict[str, Any]) -> None:
        self._config_data = config
        self.status = EcucConfigStatus.CONFIGURED

    def validate(self) -> bool:
        self.status = EcucConfigStatus.VALIDATED
        return True

    def generate_arxml(self) -> str:
        self.status = EcucConfigStatus.GENERATED
        return "<AUTOSAR><!-- generated --></AUTOSAR>"


class TestEcucConfigModelConcrete(unittest.TestCase):
    """Test concrete base model implementation"""

    def setUp(self):
        self.model = ConcreteConfigModel("TestModule", "2.0.0")

    def test_initial_state(self):
        """Test initial state"""
        self.assertEqual(self.model.module_name, "TestModule")
        self.assertEqual(self.model.version, "2.0.0")
        self.assertEqual(self.model.status, EcucConfigStatus.UNCONFIGURED)
        self.assertEqual(len(self.model.containers), 0)
        self.assertEqual(len(self.model.dependencies), 0)
        self.assertEqual(self.model._config_data, {})

    def test_configure(self):
        """Test abstract configure method"""
        config = {"param1": "value1", "nested": {"key": "val"}}
        self.model.configure(config)
        self.assertEqual(self.model.status, EcucConfigStatus.CONFIGURED)
        self.assertEqual(self.model._config_data["param1"], "value1")

    def test_validate(self):
        """Test validate method"""
        self.model.configure({"a": 1})
        result = self.model.validate()
        self.assertTrue(result)
        self.assertEqual(self.model.status, EcucConfigStatus.VALIDATED)

    def test_generate_arxml(self):
        """Test generate_arxml"""
        self.model.configure({"a": 1})
        self.model.validate()
        xml = self.model.generate_arxml()
        self.assertIn("AUTOSAR", xml)
        self.assertEqual(self.model.status, EcucConfigStatus.GENERATED)

    def test_add_container_and_to_dict(self):
        """Test add_container and to_dict"""
        container = EcucContainer(name="Container1", short_name="C1")
        container.add_parameter(EcucParameter(name="P1", value=42, param_type="INTEGER"))
        container.add_reference(EcucReference(name="R1", target="/T1"))
        self.model.add_container(container)

        d = self.model.to_dict()
        self.assertEqual(d["moduleName"], "TestModule")
        self.assertEqual(d["version"], "2.0.0")
        self.assertEqual(len(d["containers"]), 1)
        self.assertEqual(d["containers"][0]["name"], "Container1")
        self.assertEqual(len(d["containers"][0]["parameters"]), 1)
        self.assertEqual(len(d["containers"][0]["references"]), 1)

    def test_to_json(self):
        """Test to_json"""
        self.model.configure({"test": True})
        json_str = self.model.to_json(indent=4)
        d = json.loads(json_str)
        self.assertEqual(d["moduleName"], "TestModule")
        self.assertIn("status", d)

    def test_add_dependency(self):
        """Test add_dependency"""
        self.model.add_dependency("Com")
        self.model.add_dependency("CanIf")
        self.model.add_dependency("Com")  # Duplicate, should not add
        deps = self.model.get_dependencies()
        self.assertEqual(deps, ["Com", "CanIf"])

    def test_get_dependencies_isolation(self):
        """Test get_dependencies returns a copy"""
        self.model.add_dependency("SomeModule")
        deps_copy = self.model.get_dependencies()
        deps_copy.append("AnotherModule")
        # Original should not be modified
        self.assertEqual(self.model.get_dependencies(), ["SomeModule"])

    def test_get_config_value_simple(self):
        """Test get_config_value with simple key"""
        self.model.configure({"param1": 100, "param2": "hello"})
        value = self.model.get_config_value("param1")
        self.assertEqual(value, 100)

    def test_get_config_value_nested(self):
        """Test get_config_value with nested path"""
        self.model.configure({"nested": {"deep": {"value": 42}}})
        value = self.model.get_config_value("nested/deep/value")
        self.assertEqual(value, 42)

    def test_get_config_value_default(self):
        """Test get_config_value returns default for missing path"""
        value = self.model.get_config_value("nonexistent/key", default="fallback")
        self.assertEqual(value, "fallback")

    def test_set_config_value(self):
        """Test set_config_value with nested path"""
        self.model.set_config_value("Config/Param1", 100)
        self.assertEqual(self.model._config_data["Config"]["Param1"], 100)

    def test_set_config_value_deeply_nested(self):
        """Test set_config_value deeply nested"""
        self.model.set_config_value("A/B/C/D", "deep_value")
        self.assertEqual(self.model._config_data["A"]["B"]["C"]["D"], "deep_value")

    def test_set_config_value_overwrite(self):
        """Test set_config_value overwrite"""
        self.model.set_config_value("Param", "original")
        self.model.set_config_value("Param", "updated")
        self.assertEqual(self.model._config_data["Param"], "updated")

    def test_add_container_reference(self):
        """Test EcucReference"""
        ref = EcucReference(name="MyRef", target="/Target/Path",
                            ref_type="REFERENCE", description="Reference description")
        self.assertEqual(ref.name, "MyRef")
        self.assertEqual(ref.target, "/Target/Path")
        d = ref.to_dict()
        self.assertEqual(d["target"], "/Target/Path")

    def test_ecuc_parameter_defaults(self):
        """Test EcucParameter with defaults"""
        param = EcucParameter(name="Test", value="val")
        self.assertEqual(param.param_type, "STRING")
        self.assertIsNone(param.min_value)
        self.assertIsNone(param.max_value)

    def test_ecuc_parameter_with_limits(self):
        """Test EcucParameter with limits"""
        param = EcucParameter(name="Range", value=50, param_type="INTEGER",
                              min_value=0, max_value=100, default_value=25)
        self.assertEqual(param.min_value, 0)
        self.assertEqual(param.max_value, 100)
        self.assertEqual(param.default_value, 25)
        d = param.to_dict()
        self.assertEqual(d["value"], 50)

    def test_container_defaults(self):
        """Test EcucContainer defaults"""
        container = EcucContainer(name="Default")
        self.assertEqual(container.short_name, "")
        self.assertEqual(len(container.parameters), 0)
        self.assertEqual(len(container.references), 0)
        self.assertEqual(len(container.sub_containers), 0)

    def test_container_short_name(self):
        """Test EcucContainer with short_name"""
        container = EcucContainer(name="Test", short_name="TestShort", description="A test")
        self.assertEqual(container.short_name, "TestShort")
        self.assertEqual(container.description, "A test")
        d = container.to_dict()
        self.assertEqual(d["shortName"], "TestShort")

    def test_container_sub_containers(self):
        """Test EcucContainer sub_containers"""
        parent = EcucContainer(name="Parent")
        child = EcucContainer(name="Child")
        parent.add_sub_container(child)
        self.assertEqual(len(parent.sub_containers), 1)
        d = parent.to_dict()
        self.assertEqual(len(d["subContainers"]), 1)

    def test_config_value_returns_default_for_missing(self):
        """Test that get_config_value returns None for missing without default"""
        val = self.model.get_config_value("nonexistent")
        self.assertIsNone(val)

    def test_get_config_value_partial_path(self):
        """Test get_config_value with partial existing path"""
        self.model.configure({"existing": {"a": 1, "b": 2}})
        val = self.model.get_config_value("existing/nonexistent", default="nope")
        self.assertEqual(val, "nope")


if __name__ == '__main__':
    unittest.main()
