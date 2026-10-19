"""ARXML (AUTOSAR XML) parser and exporter.

Supports importing/exporting BSW configuration in AUTOSAR standard ARXML format.
"""

import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any, Dict, List, Optional
from dataclasses import dataclass


# AUTOSAR ARXML namespaces
ARXML_NS = {
    "ar": "http://autosar.org/schema/r4.0",
    "xsi": "http://www.w3.org/2001/XMLSchema-instance",
}

# Register namespaces for output
for prefix, uri in ARXML_NS.items():
    ET.register_namespace(prefix, uri)


@dataclass
class ArxmlParseResult:
    """Result of parsing an ARXML file."""
    config: Dict[str, Any]
    modules: List[str]
    warnings: List[str]


class ArxmlParser:
    """Parse AUTOSAR ARXML files into configuration dicts."""

    def __init__(self):
        self.warnings: List[str] = []

    def parse_file(self, path: Path) -> ArxmlParseResult:
        """Parse an ARXML file and return configuration."""
        self.warnings = []
        tree = ET.parse(path)
        root = tree.getroot()
        return self._parse_root(root)

    def parse_string(self, xml_string: str) -> ArxmlParseResult:
        """Parse ARXML from string."""
        self.warnings = []
        root = ET.fromstring(xml_string)
        return self._parse_root(root)

    def _parse_root(self, root: ET.Element) -> ArxmlParseResult:
        """Parse the root AUTOSAR element."""
        config = {
            "version": "1.0",
            "project": {"name": "Imported from ARXML"},
            "modules": {},
        }

        # Find all ECU extractors or BSW modules
        # Try multiple paths: direct ECUC-VALUE, or nested under ECUC-VALUES
        ecu_extractors = self._find_elements(root, ".//ECUC-VALUE")

        # If no ECUC-VALUE found directly, check if there's an ECUC-VALUES container
        if not ecu_extractors:
            ecu_values_containers = self._find_elements(root, ".//ECUC-VALUES")
            if ecu_values_containers:
                for container in ecu_values_containers:
                    ecu_extractors.extend(self._find_elements(container, ".//ECUC-VALUE"))

        if ecu_extractors:
            for ecuc in ecu_extractors:
                self._parse_ecuc_value(ecuc, config)
        else:
            # Try to find module configurations directly
            self._parse_direct_modules(root, config)

        modules = list(config["modules"].keys())
        return ArxmlParseResult(
            config=config,
            modules=modules,
            warnings=self.warnings,
        )

    def _find_elements(self, root: ET.Element, path: str) -> List[ET.Element]:
        """Find elements with namespace handling."""
        results = []
        # Try with each namespace
        for ns_prefix, ns_uri in ARXML_NS.items():
            # Replace tag names with namespace-qualified versions
            # e.g., ".//ECUC-VALUE" -> ".//{http://autosar.org/schema/r4.0}ECUC-VALUE"
            ns_path = path
            # Note: Order matters - replace longer tags first to avoid substring issues
            for tag in ["ECUC-BOOLEAN-PARAM-VALUE", "ECUC-NUMERICAL-PARAM-VALUE",
                        "ECUC-STRING-PARAM-VALUE", "ECUC-VALUE", "ECUC-VALUES",
                        "CONTAINER", "SHORT-NAME", "DEFINITION-REF"]:
                ns_path = ns_path.replace(tag, f"{{{ns_uri}}}{tag}")
            try:
                found = root.findall(ns_path)
                results.extend(found)
            except Exception:
                pass
        # Fallback: try without namespace
        if not results:
            try:
                results.extend(root.findall(path))
            except Exception:
                pass
        return results

    def _parse_ecuc_value(self, ecuc: ET.Element, config: Dict[str, Any]):
        """Parse an ECUC-VALUE element."""
        container = self._find_child(ecuc, "CONTAINER")
        if container is None:
            self.warnings.append("ECUC-VALUE without CONTAINER")
            return

        module_name = self._get_text(container, "SHORT-NAME")
        if not module_name:
            self.warnings.append("CONTAINER without SHORT-NAME")
            return

        module_config = self._parse_container(container)
        config["modules"][module_name] = module_config

    def _parse_container(self, container: ET.Element) -> Dict[str, Any]:
        """Parse a CONTAINER element into a config dict."""
        result: Dict[str, Any] = {"enabled": True}

        # Parse parameters
        for param in self._find_elements(container, ".//ECUC-NUMERICAL-PARAM-VALUE"):
            name = self._get_text(param, "DEFINITION-REF")
            value = self._get_text(param, "VALUE")
            if name and value:
                param_name = name.split("/")[-1].lower().replace("-", "_")
                result[param_name] = self._convert_value(value)

        for param in self._find_elements(container, ".//ECUC-STRING-PARAM-VALUE"):
            name = self._get_text(param, "DEFINITION-REF")
            value = self._get_text(param, "VALUE")
            if name and value:
                param_name = name.split("/")[-1].lower().replace("-", "_")
                result[param_name] = value

        for param in self._find_elements(container, ".//ECUC-BOOLEAN-PARAM-VALUE"):
            name = self._get_text(param, "DEFINITION-REF")
            value = self._get_text(param, "VALUE")
            if name and value:
                param_name = name.split("/")[-1].lower().replace("-", "_")
                result[param_name] = value.upper() == "TRUE"

        return result

    def _parse_direct_modules(self, root: ET.Element, config: Dict[str, Any]):
        """Parse modules directly from root (non-standard format)."""
        for child in root:
            tag = child.tag.split("}")[-1] if "}" in child.tag else child.tag
            if tag.isupper() and len(tag) > 2:
                module_config = self._element_to_dict(child)
                config["modules"][tag] = module_config

    def _element_to_dict(self, elem: ET.Element) -> Dict[str, Any]:
        """Convert an XML element to a dict."""
        result: Dict[str, Any] = {}
        for child in elem:
            tag = child.tag.split("}")[-1] if "}" in child.tag else child.tag
            tag = tag.lower().replace("-", "_")
            if len(child) > 0:
                result[tag] = self._element_to_dict(child)
            else:
                text = child.text
                if text:
                    result[tag] = self._convert_value(text.strip())
        return result

    def _find_child(self, parent: ET.Element, tag: str) -> Optional[ET.Element]:
        """Find a child element by tag name."""
        for ns_uri in ARXML_NS.values():
            child = parent.find(f"{{{ns_uri}}}{tag}")
            if child is not None:
                return child
        return parent.find(tag)

    def _get_text(self, parent: ET.Element, tag: str) -> Optional[str]:
        """Get text content of a child element."""
        child = self._find_child(parent, tag)
        return child.text if child is not None else None

    def _convert_value(self, value: str) -> Any:
        """Convert string value to appropriate Python type."""
        if value.upper() == "TRUE":
            return True
        if value.upper() == "FALSE":
            return False
        try:
            return int(value)
        except ValueError:
            pass
        try:
            return float(value)
        except ValueError:
            pass
        return value


class ArxmlExporter:
    """Export configuration dicts to AUTOSAR ARXML format."""

    def __init__(self):
        self.ns = ARXML_NS["ar"]

    def export_file(self, config: Dict[str, Any], path: Path) -> None:
        """Export configuration to an ARXML file."""
        root = self._build_root(config)
        tree = ET.ElementTree(root)
        ET.indent(tree, space="  ")
        tree.write(path, encoding="utf-8", xml_declaration=True)

    def export_string(self, config: Dict[str, Any]) -> str:
        """Export configuration to ARXML string."""
        root = self._build_root(config)
        ET.indent(root, space="  ")
        return ET.tostring(root, encoding="unicode")

    def _build_root(self, config: Dict[str, Any]) -> ET.Element:
        """Build the root AUTOSAR element."""
        root = ET.Element(f"{{{self.ns}}}AUTOSAR")
        root.set(f"{{{ARXML_NS['xsi']}}}schemaLocation", self.ns)

        project = config.get("project", {})
        project_name = project.get("name", "yuleASR")

        # Add admin data
        admin_data = ET.SubElement(root, f"{{{self.ns}}}ADMIN-DATA")
        created = ET.SubElement(admin_data, f"{{{self.ns}}}CREATION-DATE")
        created.text = "2026-01-01T00:00:00Z"

        # Add ECU extractors
        ecu_extractors = ET.SubElement(root, f"{{{self.ns}}}ECU-EXTRACTORS")

        modules = config.get("modules", {})
        for module_name, module_config in modules.items():
            if not module_config.get("enabled", True):
                continue
            ecu_value = self._build_ecuc_value(module_name, module_config)
            ecu_extractors.append(ecu_value)

        return root

    def _build_ecuc_value(self, module_name: str, module_config: Dict[str, Any]) -> ET.Element:
        """Build an ECUC-VALUE element for a module."""
        ecuc = ET.Element(f"{{{self.ns}}}ECUC-VALUE")

        short_name = ET.SubElement(ecuc, f"{{{self.ns}}}SHORT-NAME")
        short_name.text = module_name

        container = ET.SubElement(ecuc, f"{{{self.ns}}}CONTAINER")
        container_short = ET.SubElement(container, f"{{{self.ns}}}SHORT-NAME")
        container_short.text = module_name

        # Add parameters
        for key, value in module_config.items():
            if key in ("name", "enabled"):
                continue
            param = self._build_param(key, value, module_name)
            if param is not None:
                container.append(param)

        return ecuc

    def _build_param(self, name: str, value: Any, module_name: str) -> Optional[ET.Element]:
        """Build a parameter element."""
        param_name = name.upper().replace("_", "-")
        # Use real AUTOSAR ECUC structure: /{Module}/{Module}General/{Parameter}
        # or /{Module}/{Module}{Container}/{Parameter} for nested params
        def_ref = f"/{module_name}/{module_name}General/{param_name}"

        if isinstance(value, bool):
            param = ET.Element(f"{{{self.ns}}}ECUC-BOOLEAN-PARAM-VALUE")
            def_ref_elem = ET.SubElement(param, f"{{{self.ns}}}DEFINITION-REF")
            def_ref_elem.text = def_ref
            value_elem = ET.SubElement(param, f"{{{self.ns}}}VALUE")
            value_elem.text = str(value).upper()
            return param

        elif isinstance(value, (int, float)):
            param = ET.Element(f"{{{self.ns}}}ECUC-NUMERICAL-PARAM-VALUE")
            def_ref_elem = ET.SubElement(param, f"{{{self.ns}}}DEFINITION-REF")
            def_ref_elem.text = def_ref
            value_elem = ET.SubElement(param, f"{{{self.ns}}}VALUE")
            value_elem.text = str(value)
            return param

        elif isinstance(value, str):
            param = ET.Element(f"{{{self.ns}}}ECUC-STRING-PARAM-VALUE")
            def_ref_elem = ET.SubElement(param, f"{{{self.ns}}}DEFINITION-REF")
            def_ref_elem.text = def_ref
            value_elem = ET.SubElement(param, f"{{{self.ns}}}VALUE")
            value_elem.text = value
            return param

        elif isinstance(value, dict):
            # Nested container
            container = ET.Element(f"{{{self.ns}}}CONTAINER")
            short_name = ET.SubElement(container, f"{{{self.ns}}}SHORT-NAME")
            short_name.text = param_name
            for k, v in value.items():
                sub_param = self._build_param(k, v, module_name)
                if sub_param is not None:
                    container.append(sub_param)
            return container

        return None
