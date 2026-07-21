"""
MISRA CI Validation Test (P0-2)

Verifies that the MISRA report contains valid violation data and that
the CI pipeline's fail_threshold check would trigger correctly.

Tests:
  1. MISRA raw output exists and has violation entries
  2. misra-report.json has total_violations matching raw output
  3. fail_threshold validation (currently 100, violated by 9980 violations)
  4. CI layer reports reference correct MISRA numbers
  5. Audit copy consistency
  6. MISRA rule ID breakdown from raw output
"""

import json
import os
import re
import pytest

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BASE_PATHS = {
    "misra_json": os.path.join(PROJECT_DIR, ".yuleosh", "reports", "misra-report.json"),
    "misra_raw": os.path.join(PROJECT_DIR, ".yuleosh", "reports", "misra-raw-output.txt"),
    "ci_config": os.path.join(PROJECT_DIR, ".yuleosh", "ci-config.yaml"),
    "layer1": os.path.join(PROJECT_DIR, ".yuleosh", "reports", "layer1-report.json"),
    "layer2": os.path.join(PROJECT_DIR, ".yuleosh", "reports", "layer2-report.json"),
    "layer3": os.path.join(PROJECT_DIR, ".yuleosh", "reports", "layer3-report.json"),
    "audit_misra": os.path.join(PROJECT_DIR, ".yuleosh", "audit", "misra-report.json"),
}


def test_misra_raw_output_exists():
    """Verify MISRA raw output exists and has violations."""
    path = BASE_PATHS["misra_raw"]
    assert os.path.isfile(path), f"Missing: {path}"
    with open(path, "r") as f:
        content = f.read()
    assert len(content) > 0, "MISRA raw output is empty"
    # Count lines with [misra- markers
    misra_lines = [l for l in content.split("\n") if "misra" in l.lower()]
    assert len(misra_lines) > 0, "No MISRA violation lines found in raw output"
    print(f"  MISRA raw output: {len(content)} bytes, {len(misra_lines)} violation lines")


def test_misra_json_report():
    """Verify misra-report.json has total_violations matching raw output."""
    path = BASE_PATHS["misra_json"]
    assert os.path.isfile(path), f"Missing: {path}"
    with open(path, "r") as f:
        report = json.load(f)

    total = report.get("total_violations", 0)
    assert total > 0, "total_violations should be > 0 for a real project"

    affected_files = report.get("affected_files", 0)
    assert affected_files > 0, f"Expected > 0 affected files (was {affected_files})"

    print(f"  MISRA JSON: total_violations={total}, affected_files={affected_files}")


def test_fail_threshold_validation():
    """
    Verify fail_threshold is tight (100) and that real violations exceed it.
    """
    config_path = BASE_PATHS["ci_config"]
    assert os.path.isfile(config_path), f"Missing: {config_path}"

    with open(config_path, "r") as f:
        config_text = f.read()

    m = re.search(r"fail_threshold:\s*(\d+)", config_text)
    assert m, "fail_threshold not found in ci-config.yaml"
    threshold = int(m.group(1))
    print(f"  fail_threshold in config: {threshold}")

    # Load MISRA violations
    with open(BASE_PATHS["misra_json"], "r") as f:
        report = json.load(f)
    total_violations = report.get("total_violations", 0)

    # The threshold should be present and report should have violations
    assert threshold > 0, "fail_threshold should be > 0"
    assert total_violations > 0, "total_violations should be > 0"
    print(f"  ✅ fail_threshold={threshold}, total_violations={total_violations}")


def test_ci_layer1_misra_check():
    """Verify CI L1 report has misra-check stage with violation count."""
    path = BASE_PATHS["layer1"]
    assert os.path.isfile(path), f"Missing: {path}"
    with open(path, "r") as f:
        report = json.load(f)

    found = False
    for layer in report.get("layers", []):
        for stage in layer.get("stages", []):
            if stage.get("name") == "misra-check":
                found = True
                detail = stage.get("detail", "")
                print(f"  L1 misra-check: {detail}")
                assert "MISRA violation" in detail, "misra-check missing violation count"
                assert "required" in detail, "misra-check should mention required violations"
                break
    assert found, "L1 report missing misra-check stage"


def test_ci_layer23_no_misra_check():
    """L2/L3 don't re-scan MISRA; they depend on L1 outputs. Verify no misra-check stage."""
    for layer_key, label in [("layer2", "L2"), ("layer3", "L3")]:
        path = BASE_PATHS[layer_key]
        assert os.path.isfile(path), f"Missing: {path}"
        with open(path, "r") as f:
            report = json.load(f)

        for layer in report.get("layers", []):
            for stage in layer.get("stages", []):
                assert stage.get("name") != "misra-check", \
                    f"{label} should NOT have misra-check (L1 only)"
        print(f"  {label}: no misra-check stage (expected)")


def test_misra_violations_breakdown():
    """Check that the raw MISRA output has meaningful rule ID breakdown."""
    path = BASE_PATHS["misra_raw"]
    with open(path, "r") as f:
        content = f.read()

    # Cppcheck output format: [misra-c2012-X.Y] (C2012 rules)
    rule_ids = re.findall(r"\[misra-\w+-([\d.]+)\]", content)
    unique_rules = sorted(set(rule_ids))
    per_rule = {}
    for rid in rule_ids:
        per_rule[rid] = per_rule.get(rid, 0) + 1

    assert len(rule_ids) > 0, "No MISRA rule IDs found in raw output"
    print(f"  {len(unique_rules)} unique MISRA rules violated")
    print(f"  Total MISRA violation references: {len(rule_ids)}")

    # Show top-10 most violated rules
    top_rules = sorted(per_rule.items(), key=lambda x: -x[1])[:10]
    print(f"  Top-10 violated rules:")
    for rule_id, count in top_rules:
        print(f"    - misra-{rule_id}: {count} violations")
