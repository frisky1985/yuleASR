"""Generate few-shot examples for AUTOSAR modules."""

from pathlib import Path
import json

REPO_ROOT = Path(__file__).resolve().parents[4]
EXAMPLES_DIR = REPO_ROOT / "tools" / "yuleasr-configurator" / "knowledge_base" / "few_shot_examples"

# Comprehensive few-shot examples for key modules
FEW_SHOT_EXAMPLES = {
    "Can": [
        {
            "request": "配置 CAN 总线，波特率 500kbps，3 个控制器",
            "config": """```yaml
baudrate: 500000
num_controllers: 3
general:
  dev_error_detect: true
  version_info_api: true
CanController:
  - id: 0
    baudrate: 500000
    propagation_delay: 100
  - id: 1
    baudrate: 500000
    propagation_delay: 100
  - id: 2
    baudrate: 500000
    propagation_delay: 100
```"""
        },
        {
            "request": "启用 CAN FD 支持，数据段波特率 2Mbps",
            "config": """```yaml
can_fd_support: true
general:
  dev_error_detect: true
CanController:
  - id: 0
    baudrate: 500000
    data_baudrate: 2000000
    can_fd: true
```"""
        },
        {
            "request": "配置 CAN 唤醒支持和错误检测",
            "config": """```yaml
general:
  dev_error_detect: true
  wakeup_support: true
  version_info_api: true
```"""
        },
    ],
    "NvM": [
        {
            "request": "启用冗余模式，块大小 64 字节",
            "config": """```yaml
general:
  dev_error_detect: true
  version_info_api: true
NvmBlockDescriptor:
  - block_id: 0
    block_size: 64
    redundancy_enabled: true
    crc_enabled: true
    write_protect: false
```"""
        },
        {
            "request": "配置最大块数 100，启用 CRC 校验",
            "config": """```yaml
general:
  dev_error_detect: true
  max_blocks: 100
NvmBlockDescriptor:
  - block_id: 0
    block_size: 256
    crc_enabled: true
    write_protect: false
```"""
        },
    ],
    "Com": [
        {
            "request": "配置信号超时时间 5 秒，启用死区检测",
            "config": """```yaml
general:
  dev_error_detect: true
  version_info_api: true
ComSignal:
  - signal_id: 0
    timeout: 5000
    dead_zone_enabled: true
    dead_zone_value: 0.1
```"""
        },
        {
            "request": "启用网关功能，配置 10 个 PDU",
            "config": """```yaml
general:
  dev_error_detect: true
  gateway_enabled: true
  max_pdus: 10
ComPdu:
  - pdu_id: 0
    direction: TX
    length: 64
  - pdu_id: 1
    direction: RX
    length: 64
```"""
        },
    ],
    "Dcm": [
        {
            "request": "配置诊断会话控制服务，支持编程会话",
            "config": """```yaml
general:
  dev_error_detect: true
  version_info_api: true
DcmDspSession:
  - session_id: 0x01
    session_name: DEFAULT_SESSION
  - session_id: 0x02
    session_name: PROGRAMMING_SESSION
  - session_id: 0x03
    session_name: EXTENDED_SESSION
DcmDspService:
  - service_id: 0x10
    service_name: DiagnosticSessionControl
    enabled: true
```"""
        },
        {
            "request": "启用 DTC 读取和清除功能",
            "config": """```yaml
general:
  dev_error_detect: true
DcmDspService:
  - service_id: 0x14
    service_name: ClearDiagnosticInformation
    enabled: true
  - service_id: 0x19
    service_name: ReadDTCInformation
    enabled: true
```"""
        },
    ],
    "Dem": [
        {
            "request": "配置 DTC 存储，支持冻结帧",
            "config": """```yaml
general:
  dev_error_detect: true
  max_dtc_count: 100
  freeze_frame_enabled: true
DemDtc:
  - dtc_id: 0x000001
    severity: high
    freeze_frame_record: 0
```"""
        },
    ],
    "PduR": [
        {
            "request": "配置 PDU 路由，CAN 到 COM",
            "config": """```yaml
general:
  dev_error_detect: true
PduRRoutingPath:
  - src_pdu: CanIf_Tx
    dest_pdu: Com_Tx
    direction: UP
  - src_pdu: Com_Rx
    dest_pdu: CanIf_Rx
    direction: DOWN
```"""
        },
    ],
}


def generate_examples():
    """Generate few-shot example files."""
    EXAMPLES_DIR.mkdir(parents=True, exist_ok=True)

    for module_name, examples in FEW_SHOT_EXAMPLES.items():
        output_file = EXAMPLES_DIR / f"{module_name}_examples.json"
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump(examples, f, indent=2, ensure_ascii=False)
        print(f"Generated {len(examples)} examples for {module_name}")

    print(f"\nAll examples saved to {EXAMPLES_DIR}")


if __name__ == "__main__":
    generate_examples()
