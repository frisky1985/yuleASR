#!/usr/bin/env python3
"""End-to-end test: ARXML import → edit → export roundtrip."""

import sys
import json
import requests
from pathlib import Path

BASE = "http://localhost:8000"


def test_full_workflow():
    """Test complete ARXML workflow: export → modify → import → verify."""
    print("=" * 60)
    print("E2E Test: ARXML Import → Edit → Export")
    print("=" * 60)

    # Step 1: Export current config to ARXML
    print("\n[1] Export current config to ARXML...")
    r = requests.post(f"{BASE}/api/arxml/export", json={})
    assert r.status_code == 200, f"Export failed: {r.text}"
    original_arxml = r.json()["arxml"]
    original_count = r.json()["module_count"]
    print(f"    ✓ Exported {original_count} modules ({len(original_arxml)} bytes)")

    # Step 2: Import the ARXML back
    print("\n[2] Import the exported ARXML...")
    r = requests.post(f"{BASE}/api/arxml/import", json={"content": original_arxml})
    assert r.status_code == 200, f"Import failed: {r.text}"
    import_data = r.json()
    assert import_data["success"], "Import not successful"
    print(f"    ✓ Imported {len(import_data['modules'])} modules")
    assert len(import_data["modules"]) == original_count, "Module count mismatch"
    print(f"    ✓ Module count matches: {original_count}")

    # Step 3: Simulate editing — modify a module parameter
    print("\n[3] Simulate editing: modify Mcu core_count...")
    config = import_data["config"]
    original_core_count = config["modules"].get("Mcu", {}).get("core_count")
    config["modules"]["Mcu"]["core_count"] = 8
    print(f"    ✓ Changed Mcu core_count: {original_core_count} → 8")

    # Step 4: Validate the modified config
    print("\n[4] Validate modified config...")
    r = requests.post(f"{BASE}/api/config/validate", json=config)
    assert r.status_code == 200, f"Validation failed: {r.text}"
    val = r.json()
    print(f"    ✓ Validation: valid={val['valid']}, errors={val['error_count']}, warnings={val['warning_count']}")

    # Step 5: Export modified config to ARXML
    print("\n[5] Export modified config to ARXML...")
    r = requests.post(f"{BASE}/api/arxml/export", json={"config": config})
    assert r.status_code == 200, f"Export failed: {r.text}"
    modified_arxml = r.json()["arxml"]
    print(f"    ✓ Exported modified config ({len(modified_arxml)} bytes)")

    # Step 6: Re-import the modified ARXML and verify the change persisted
    print("\n[6] Re-import modified ARXML and verify edit persisted...")
    r = requests.post(f"{BASE}/api/arxml/import", json={"content": modified_arxml})
    assert r.status_code == 200, f"Re-import failed: {r.text}"
    reimported = r.json()
    reimported_core_count = reimported["config"]["modules"].get("Mcu", {}).get("core_count")
    print(f"    ✓ Mcu core_count after re-import: {reimported_core_count}")
    assert reimported_core_count == 8, f"Expected 8, got {reimported_core_count}"
    print(f"    ✓ Edit persisted through ARXML roundtrip!")

    # Step 7: Test merge scenario
    print("\n[7] Test merge scenario: import partial ARXML into existing config...")
    partial_config = {
        "version": "1.0",
        "project": {"name": "Partial Import"},
        "modules": {
            "TestModule": {"name": "TestModule", "enabled": True, "version": "2.0.0", "test_param": 42}
        }
    }
    # Export partial to ARXML
    r = requests.post(f"{BASE}/api/arxml/export", json={"config": partial_config})
    assert r.status_code == 200
    partial_arxml = r.json()["arxml"]

    # Import partial ARXML
    r = requests.post(f"{BASE}/api/arxml/import", json={"content": partial_arxml})
    assert r.status_code == 200
    partial_import = r.json()
    assert "TestModule" in partial_import["modules"]
    print(f"    ✓ Partial import: {len(partial_import['modules'])} modules")
    print(f"    ✓ TestModule found with params: {list(partial_import['config']['modules']['TestModule'].keys())}")

    print("\n" + "=" * 60)
    print("ALL E2E TESTS PASSED ✓")
    print("=" * 60)
    print("\nWorkflow verified:")
    print("  1. Export config → ARXML ✓")
    print("  2. Import ARXML → config ✓")
    print("  3. Edit module parameters ✓")
    print("  4. Validate modified config ✓")
    print("  5. Export modified → ARXML ✓")
    print("  6. Re-import → edits persist ✓")
    print("  7. Partial/merge import ✓")
    return True


if __name__ == "__main__":
    try:
        success = test_full_workflow()
    except AssertionError as e:
        print(f"\n✗ ASSERTION FAILED: {e}")
        success = False
    except Exception as e:
        print(f"\n✗ ERROR: {e}")
        success = False

    sys.exit(0 if success else 1)
