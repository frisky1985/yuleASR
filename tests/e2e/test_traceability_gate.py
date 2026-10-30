"""Traceability evidence gate tests (WP4)."""
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(REPO, "tools"))
import check_traceability_gate as tg  # noqa: E402


def _req(rid, **kw):
    base = {"req_id": rid, "code_files": [], "matched_tests": [], "test_reports": [],
            "reviews": [], "has_code": False, "has_test": False, "has_review": False}
    base.update(kw)
    return base


def test_python_code_file_rejected():
    ok, fails, _ = tg.validate({"lrm": {"requirements": [
        _req("DEM-SHALL-002", code_files=["src/micro-dds/tests/integration/test_integration.py"],
              has_code=True)]}})
    assert not ok and any("non-implementation" in f for f in fails), fails


def test_has_code_without_code_files_rejected():
    ok, fails, _ = tg.validate({"lrm": {"requirements": [
        _req("DCM-SHALL-001", has_code=True)]}})
    assert not ok and any("has_code=True but code_files is empty" in f for f in fails), fails


def test_code_files_without_has_code_flag_rejected():
    ok, fails, _ = tg.validate({"lrm": {"requirements": [
        _req("DEM-REQ-01", code_files=["src/bsw/services/dem/src/Dem.c"], has_code=False)]}})
    assert not ok and any("code_files present but has_code=False" in f for f in fails), fails


def test_overloaded_file_rejected():
    # one .c mapped to >20 requirements is a synthetic fallback
    reqs = [_req(f"R-{i}", code_files=["src/x/x.c"], has_code=True) for i in range(22)]
    ok, fails, summ = tg.validate({"lrm": {"requirements": reqs}})
    assert not ok and summ["overloaded_files"] == 1, (fails, summ)


def test_clean_report_passes():
    ok, fails, summ = tg.validate({"lrm": {"requirements": [
        _req("COM-SHALL-001", code_files=["src/bsw/services/com/src/Com.c"],
              has_code=True, matched_tests=["tests/unit/com/test_com.c"], has_test=True)]}})
    assert ok and summ["invalid_mappings"] == 0, (fails, summ)
    print(f"  ✅ traceability gate passes on a clean, C-mapped requirement")
