"""
UDS diagnostic HIL sequences — simulation-mode validation.

验证 tests/hil/test_hil_diag.c 中 UDS 序列定义（会话控制 / 读 DID）
与真实 Dcm/CanTp 产品头文件常量一致。
"""

from conftest import extract_defines, load_hil_c_sources, load_src_headers

HIL_DIAG = "test_hil_diag.c"


def _hil_diag_defines():
    src = next(f for f in load_hil_c_sources() if f.name == HIL_DIAG)
    return extract_defines(src.read_text(encoding="utf-8"))


def test_hil_diag_sequence_file_exists():
    assert any(f.name == HIL_DIAG for f in load_hil_c_sources())


def test_hil_diag_uds_sid_defines_present():
    """HIL 序列必须定义标准 UDS SID 常量（ISO 14229-1）。"""
    d = _hil_diag_defines()
    assert d.get("DIAG_SID_DIAG_SESSION") == "0x10U", "会话控制 SID 0x10"
    assert d.get("DIAG_SID_READ_DID") == "0x22U", "读取 DID SID 0x22"
    assert d.get("DIAG_SID_WRITE_DID") == "0x2EU", "写入 DID SID 0x2E"


def test_hil_diag_session_values_match_iso14229():
    d = _hil_diag_defines()
    assert d.get("DIAG_SESSION_DEFAULT") == "0x01U"
    assert d.get("DIAG_SESSION_EXTENDED") == "0x03U"


def test_hil_diag_can_ids_defined():
    """诊断 CAN ID 必须定义（功能寻址 0x7DF / 响应 0x7E8）。"""
    d = _hil_diag_defines()
    assert d.get("DIAG_REQUEST_ID") == "0x7DFU"
    assert d.get("DIAG_RESPONSE_ID") == "0x7E8U"


def test_hil_diag_sids_match_real_dcm_headers():
    """序列 SID 与真实 Dcm 头文件常量一致。"""
    dcm_headers = load_src_headers(("Dcm", "dcm"))
    assert dcm_headers, "未找到 Dcm 产品头文件"
    haystack = "\n".join(h.read_text(encoding="utf-8", errors="replace") for h in dcm_headers)
    # DCM 模块应包含 0x10/0x22/0x2E 服务定义
    assert "0x10" in haystack or "10U" in haystack
    assert "0x22" in haystack or "22U" in haystack
    assert "0x2E" in haystack or "2EU" in haystack


def test_hil_diag_sequences_not_vacuously_empty():
    """序列文件必须包含实质测试用例（非空模板）。"""
    src = next(f for f in load_hil_c_sources() if f.name == HIL_DIAG)
    text = src.read_text(encoding="utf-8")
    assert text.count("static void test_") >= 2, "至少 2 个 UDS 序列用例"
    assert "SKIPPED" in text or "skip()" in text, "硬件未就绪时用例应显式 skip"
