#!/usr/bin/env python3
"""Test ARXML API endpoints."""

import sys
import requests
from pathlib import Path


def test_api():
    """Test ARXML API endpoints."""
    base_url = "http://localhost:8000"

    print("=" * 60)
    print("Testing ARXML API Endpoints")
    print("=" * 60)

    # Test 1: Export ARXML
    print("\n[1] Testing POST /api/arxml/export...")
    try:
        response = requests.post(f"{base_url}/api/arxml/export", json={})
        if response.status_code == 200:
            data = response.json()
            print(f"    ✓ Export successful")
            print(f"    ✓ Module count: {data['module_count']}")
            print(f"    ✓ ARXML size: {len(data['arxml'])} bytes")
            arxml_content = data['arxml']
        else:
            print(f"    ✗ Failed: {response.status_code}")
            print(f"    {response.text}")
            return False
    except Exception as e:
        print(f"    ✗ Error: {e}")
        return False

    # Test 2: Import ARXML
    print("\n[2] Testing POST /api/arxml/import...")
    try:
        response = requests.post(
            f"{base_url}/api/arxml/import",
            json={"content": arxml_content}
        )
        if response.status_code == 200:
            data = response.json()
            print(f"    ✓ Import successful")
            print(f"    ✓ Modules imported: {len(data['modules'])}")
            print(f"    ✓ Warnings: {len(data['warnings'])}")
        else:
            print(f"    ✗ Failed: {response.status_code}")
            print(f"    {response.text}")
            return False
    except Exception as e:
        print(f"    ✗ Error: {e}")
        return False

    # Test 3: Export to file
    print("\n[3] Testing POST /api/arxml/export-file...")
    try:
        output_path = "/tmp/test_export.arxml"
        response = requests.post(
            f"{base_url}/api/arxml/export-file",
            params={"output_path": output_path},
            json={}
        )
        if response.status_code == 200:
            data = response.json()
            print(f"    ✓ Export to file successful")
            print(f"    ✓ Path: {data['path']}")
            print(f"    ✓ Module count: {data['module_count']}")

            # Verify file exists
            if Path(output_path).exists():
                print(f"    ✓ File exists on disk")
            else:
                print(f"    ✗ File not found on disk")
                return False
        else:
            print(f"    ✗ Failed: {response.status_code}")
            print(f"    {response.text}")
            return False
    except Exception as e:
        print(f"    ✗ Error: {e}")
        return False

    # Test 4: Import from file
    print("\n[4] Testing POST /api/arxml/import-file...")
    try:
        response = requests.post(
            f"{base_url}/api/arxml/import-file",
            params={"file_path": output_path}
        )
        if response.status_code == 200:
            data = response.json()
            print(f"    ✓ Import from file successful")
            print(f"    ✓ Modules imported: {len(data['modules'])}")
        else:
            print(f"    ✗ Failed: {response.status_code}")
            print(f"    {response.text}")
            return False
    except Exception as e:
        print(f"    ✗ Error: {e}")
        return False

    print("\n" + "=" * 60)
    print("ALL API TESTS PASSED ✓")
    print("=" * 60)
    return True


if __name__ == "__main__":
    success = test_api()
    sys.exit(0 if success else 1)
