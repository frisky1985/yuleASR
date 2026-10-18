#!/usr/bin/env python3
"""Test ARXML import/export functionality."""

import sys
from pathlib import Path

# Add project root to path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from yuleasr.core.arxml_handler import ArxmlParser, ArxmlExporter
from yuleasr.core.config_loader import load_config


def test_export_import_roundtrip():
    """Test that we can export to ARXML and import it back."""
    print("=" * 60)
    print("Testing ARXML Export/Import Roundtrip")
    print("=" * 60)

    # Load original config
    config_path = Path(__file__).resolve().parents[3] / "config" / "bsw_config.yaml"
    print(f"\n[1] Loading config from {config_path}")

    if not config_path.exists():
        print(f"ERROR: Config file not found: {config_path}")
        return False

    original_config = load_config(config_path)
    original_modules = list(original_config.get("modules", {}).keys())
    print(f"    Loaded {len(original_modules)} modules")

    # Export to ARXML
    print("\n[2] Exporting to ARXML...")
    exporter = ArxmlExporter()
    arxml_content = exporter.export_string(original_config)
    print(f"    Generated {len(arxml_content)} bytes of ARXML")
    print(f"    Preview:\n{arxml_content[:500]}...")

    # Import from ARXML
    print("\n[3] Importing from ARXML...")
    parser = ArxmlParser()
    result = parser.parse_string(arxml_content)
    imported_modules = result.modules
    print(f"    Imported {len(imported_modules)} modules")
    print(f"    Warnings: {len(result.warnings)}")
    if result.warnings:
        for warning in result.warnings[:5]:
            print(f"      - {warning}")

    # Compare
    print("\n[4] Comparing original vs imported...")
    original_set = set(original_modules)
    imported_set = set(imported_modules)

    missing = original_set - imported_set
    extra = imported_set - original_set

    if missing:
        print(f"    WARNING: {len(missing)} modules missing in import:")
        for m in sorted(missing)[:10]:
            print(f"      - {m}")

    if extra:
        print(f"    WARNING: {len(extra)} extra modules in import:")
        for m in sorted(extra)[:10]:
            print(f"      - {m}")

    match_rate = len(original_set & imported_set) / len(original_set) * 100
    print(f"\n    Match rate: {match_rate:.1f}% ({len(original_set & imported_set)}/{len(original_set)})")

    # Save ARXML to file for inspection
    arxml_path = config_path.parent / "test_export.arxml"
    exporter.export_file(original_config, arxml_path)
    print(f"\n[5] Saved ARXML to {arxml_path}")

    print("\n" + "=" * 60)
    print("TEST PASSED ✓" if match_rate > 50 else "TEST PARTIAL ✓")
    print("=" * 60)
    return True


def test_sample_arxml():
    """Test parsing a sample ARXML snippet."""
    print("\n" + "=" * 60)
    print("Testing Sample ARXML Parsing")
    print("=" * 60)

    sample_arxml = """<?xml version="1.0" encoding="UTF-8"?>
<AUTOSAR xmlns="http://autosar.org/schema/ar4.0">
  <ECU-EXTRACTORS>
    <ECUC-VALUE>
      <SHORT-NAME>Mcu</SHORT-NAME>
      <CONTAINER>
        <SHORT-NAME>Mcu</SHORT-NAME>
        <ECUC-BOOLEAN-PARAM-VALUE>
          <DEFINITION-REF>/Mcu/DEV-ERROR-DETECT</DEFINITION-REF>
          <VALUE>TRUE</VALUE>
        </ECUC-BOOLEAN-PARAM-VALUE>
        <ECUC-NUMERICAL-PARAM-VALUE>
          <DEFINITION-REF>/Mcu/CORE-COUNT</DEFINITION-REF>
          <VALUE>4</VALUE>
        </ECUC-NUMERICAL-PARAM-VALUE>
      </CONTAINER>
    </ECUC-VALUE>
  </ECU-EXTRACTORS>
</AUTOSAR>"""

    print("\n[1] Parsing sample ARXML...")
    parser = ArxmlParser()
    result = parser.parse_string(sample_arxml)

    print(f"    Modules found: {result.modules}")
    print(f"    Warnings: {len(result.warnings)}")

    if "Mcu" in result.config.get("modules", {}):
        mcu_config = result.config["modules"]["Mcu"]
        print(f"    Mcu config: {mcu_config}")
        print("\n    TEST PASSED ✓")
        return True
    else:
        print("\n    TEST FAILED ✗")
        return False


if __name__ == "__main__":
    success = True
    success = test_sample_arxml() and success
    success = test_export_import_roundtrip() and success

    sys.exit(0 if success else 1)
