#!/usr/bin/env python3
"""Test FastAPI backend endpoints."""

import sys
import time
from pathlib import Path

import requests

# Start server in background
import subprocess
server_proc = subprocess.Popen(
    [sys.executable, "-m", "uvicorn", "yuleasr.web.api.main:app", "--host", "127.0.0.1", "--port", "8000"],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    cwd=str(Path(__file__).parent.parent),
)

# Wait for server to start
time.sleep(2)

BASE_URL = "http://127.0.0.1:8000"

try:
    print("=" * 70)
    print("FastAPI Backend Test Suite")
    print("=" * 70)
    
    # Test 1: Health check
    print("\n[1/6] Testing health check...")
    resp = requests.get(f"{BASE_URL}/health")
    assert resp.status_code == 200, f"Health check failed: {resp.status_code}"
    data = resp.json()
    assert data["status"] == "healthy"
    print(f"  ✓ Health: {data['status']}")
    
    # Test 2: Load config
    print("\n[2/6] Testing config loading...")
    resp = requests.get(f"{BASE_URL}/api/config")
    assert resp.status_code == 200, f"Config load failed: {resp.status_code}"
    data = resp.json()
    print(f"  ✓ Loaded config: {data['module_count']} modules")
    
    # Test 3: List schemas
    print("\n[3/6] Testing schema listing...")
    resp = requests.get(f"{BASE_URL}/api/schemas")
    assert resp.status_code == 200, f"Schema list failed: {resp.status_code}"
    schemas = resp.json()
    total_schemas = sum(len(mods) for mods in schemas.values())
    print(f"  ✓ Found {total_schemas} schemas across {len(schemas)} layers")
    
    # Test 4: List modules
    print("\n[4/6] Testing module listing...")
    resp = requests.get(f"{BASE_URL}/api/modules")
    assert resp.status_code == 200, f"Module list failed: {resp.status_code}"
    modules = resp.json()
    print(f"  ✓ Listed {len(modules)} modules")
    
    # Test 5: Get init order
    print("\n[5/6] Testing init order...")
    resp = requests.get(f"{BASE_URL}/api/init-order")
    assert resp.status_code == 200, f"Init order failed: {resp.status_code}"
    data = resp.json()
    print(f"  ✓ Init order: {len(data['order'])} modules")
    print(f"    First 5: {', '.join(data['order'][:5])}")
    
    # Test 6: Validate config
    print("\n[6/6] Testing config validation...")
    config_resp = requests.get(f"{BASE_URL}/api/config")
    config = config_resp.json()["config"]
    resp = requests.post(f"{BASE_URL}/api/config/validate", json=config)
    assert resp.status_code == 200, f"Validation failed: {resp.status_code}"
    data = resp.json()
    print(f"  ✓ Validation: {data['error_count']} errors, {data['warning_count']} warnings")
    
    print("\n" + "=" * 70)
    print("ALL TESTS PASSED ✓")
    print("=" * 70)

finally:
    # Cleanup
    server_proc.terminate()
    server_proc.wait()
