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


def _review(status="pending", reviewer=None, date=None, findings=None, decision=None):
    return {"status": status, "reviewer": reviewer, "date": date,
            "findings": findings or [], "decision": decision}


def test_pending_review_is_non_blocking_gap():
    """A pending placeholder is an honest gap — not counted as a review, not a failure."""
    ok, fails, summ = tg.validate({"lrm": {"requirements": [
        _req("COM-SHALL-001", code_files=["src/bsw/services/com/src/Com.c"],
              has_code=True, matched_tests=["tests/unit/com/test_com.c"], has_test=True,
              reviews=[_review("pending")], has_review=False)]}})
    assert ok, fails
    assert summ["reviewed"] == 0 and summ["review_gap"] == 1, summ
    print(f"  ✅ pending review is a non-blocking gap (reviewed=0, gap=1)")


def test_approved_review_counts_and_passes():
    ok, fails, summ = tg.validate({"lrm": {"requirements": [
        _req("COM-SHALL-001", code_files=["src/bsw/services/com/src/Com.c"],
              has_code=True, matched_tests=["tests/unit/com/test_com.c"], has_test=True,
              reviews=[_review("approved", reviewer="张三", date="2026-08-25",
                               decision="approve")], has_review=True)]}})
    assert ok, fails
    assert summ["reviewed"] == 1 and summ["review_gap"] == 0, summ
    print(f"  ✅ approved review with reviewer+date counts (reviewed=1, gap=0)")


def test_approved_review_requires_reviewer_and_date():
    ok, fails, summ = tg.validate({"lrm": {"requirements": [
        _req("DCM-SHALL-002", reviews=[_review("approved")], has_review=True)]}})
    assert not ok, fails
    assert any("missing reviewer" in f for f in fails), fails
    assert any("missing a valid date" in f for f in fails), fails
    print(f"  ✅ approved review without reviewer/date is rejected")


def test_has_review_flag_must_match_approved_reviews():
    # pending review but has_review=True -> drift failure
    ok, fails, summ = tg.validate({"lrm": {"requirements": [
        _req("X-1", reviews=[_review("pending")], has_review=True)]}})
    assert not ok and any("has_review" in f for f in fails), fails
    print(f"  ✅ has_review flag drift against approved reviews is rejected")


def test_require_review_blocks_on_gap():
    ok, fails, summ = tg.validate(
        {"lrm": {"requirements": [_req("Y-1", reviews=[_review("pending")])]}},
        require_review=True)
    assert not ok and summ["review_gap"] == 1, (fails, summ)
    print(f"  ✅ --require-review blocks when review gap exists")
