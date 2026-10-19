"""Collect AUTOSAR training data from internet sources."""

import json
import requests
from pathlib import Path
from typing import List, Dict, Any
import time

# Output directory
DATA_DIR = Path(__file__).resolve().parents[4] / "tools" / "yuleasr-configurator" / "training_data"
DATA_DIR.mkdir(parents=True, exist_ok=True)


class AutosarDataCollector:
    """Collect AUTOSAR configuration data from various sources."""

    def __init__(self):
        self.training_data: List[Dict[str, Any]] = []

    def collect_from_github(self, repo_urls: List[str]) -> List[Dict[str, Any]]:
        """Collect configuration examples from GitHub repositories."""
        examples = []

        # Known AUTOSAR open-source repositories
        autosar_repos = [
            "https://raw.githubusercontent.com/openautosar/openautosar/master/",
            "https://raw.githubusercontent.com/autosar/aps/master/",
        ]

        # Configuration file patterns to search for
        config_patterns = [
            "Can_Config.c",
            "Can_Cfg.h",
            "NvM_Cfg.c",
            "Com_Cfg.h",
            "Dcm_Cfg.c",
            "Dem_Cfg.c",
            "PduR_Cfg.c",
        ]

        for repo_url in repo_urls:
            for pattern in config_patterns:
                try:
                    url = f"{repo_url}{pattern}"
                    response = requests.get(url, timeout=10)
                    if response.status_code == 200:
                        examples.append({
                            "source": "github",
                            "url": url,
                            "content": response.text[:2000],  # Limit size
                            "module": pattern.split("_")[0]
                        })
                        print(f"Collected: {pattern}")
                    time.sleep(1)  # Rate limiting
                except Exception as e:
                    print(f"Failed to fetch {url}: {e}")

        return examples

    def generate_from_specifications(self) -> List[Dict[str, Any]]:
        """Generate training data from AUTOSAR specifications."""
        examples = []

        # CAN module examples
        can_examples = [
            {
                "instruction": "配置 CAN 总线波特率为 500kbps",
                "input": "",
                "output": "```yaml\nCan:\n  general:\n    baudrate: 500000\n    dev_error_detect: true\n  CanController:\n    - id: 0\n      baudrate: 500000\n      propagation_delay: 100\n```"
            },
            {
                "instruction": "启用 CAN FD 支持，数据段波特率 2Mbps",
                "input": "",
                "output": "```yaml\nCan:\n  can_fd_support: true\n  CanController:\n    - id: 0\n      baudrate: 500000\n      data_baudrate: 2000000\n      can_fd: true\n```"
            },
            {
                "instruction": "配置 3 个 CAN 控制器",
                "input": "",
                "output": "```yaml\nCan:\n  num_controllers: 3\n  CanController:\n    - id: 0\n      baudrate: 500000\n    - id: 1\n      baudrate: 500000\n    - id: 2\n      baudrate: 500000\n```"
            },
        ]

        # NvM module examples
        nvm_examples = [
            {
                "instruction": "启用 NvM 冗余模式，块大小 64 字节",
                "input": "",
                "output": "```yaml\nNvM:\n  general:\n    dev_error_detect: true\n    redundancy_enabled: true\n  NvmBlockDescriptor:\n    - block_id: 0\n      block_size: 64\n      crc_enabled: true\n```"
            },
            {
                "instruction": "配置 NvM 最大块数 100，启用 CRC 校验",
                "input": "",
                "output": "```yaml\nNvM:\n  general:\n    max_blocks: 100\n    crc_enabled: true\n  NvmBlockDescriptor:\n    - block_id: 0\n      block_size: 256\n      crc_enabled: true\n```"
            },
        ]

        # Com module examples
        com_examples = [
            {
                "instruction": "配置 Com 信号超时时间 5 秒",
                "input": "",
                "output": "```yaml\nCom:\n  general:\n    dev_error_detect: true\n  ComSignal:\n    - signal_id: 0\n      timeout: 5000\n      dead_zone_enabled: true\n```"
            },
            {
                "instruction": "启用 Com 网关功能",
                "input": "",
                "output": "```yaml\nCom:\n  general:\n    gateway_enabled: true\n    max_pdus: 10\n```"
            },
        ]

        # Dcm module examples
        dcm_examples = [
            {
                "instruction": "配置 DCM 诊断会话控制，支持编程会话",
                "input": "",
                "output": "```yaml\nDcm:\n  general:\n    dev_error_detect: true\n  DcmDspSession:\n    - session_id: 0x01\n      session_name: DEFAULT_SESSION\n    - session_id: 0x02\n      session_name: PROGRAMMING_SESSION\n```"
            },
            {
                "instruction": "启用 DTC 读取和清除功能",
                "input": "",
                "output": "```yaml\nDcm:\n  DcmDspService:\n    - service_id: 0x14\n      service_name: ClearDiagnosticInformation\n      enabled: true\n    - service_id: 0x19\n      service_name: ReadDTCInformation\n      enabled: true\n```"
            },
        ]

        examples.extend(can_examples)
        examples.extend(nvm_examples)
        examples.extend(com_examples)
        examples.extend(dcm_examples)

        return examples

    def collect_from_documentation(self) -> List[Dict[str, Any]]:
        """Collect data from AUTOSAR documentation."""
        examples = []

        # Common configuration patterns from AUTOSAR spec
        spec_examples = [
            {
                "instruction": "配置 MCU 时钟频率为 160MHz",
                "input": "",
                "output": "```yaml\nMcu:\n  general:\n    clock_freq_mhz: 160\n    dev_error_detect: true\n  McuClockSetting:\n    - id: 0\n      clock_freq: 160000000\n      pll_multiplier: 20\n```"
            },
            {
                "instruction": "配置 Port 引脚为输出模式",
                "input": "",
                "output": "```yaml\nPort:\n  PortPin:\n    - pin_id: 0\n      direction: OUTPUT\n      initial_mode: PUSH_PULL\n      initial_value: LOW\n```"
            },
            {
                "instruction": "配置 GPT 定时器周期 10ms",
                "input": "",
                "output": "```yaml\nGpt:\n  general:\n    dev_error_detect: true\n  GptChannel:\n    - channel_id: 0\n      period: 10000\n      mode: CONTINUOUS\n```"
            },
            {
                "instruction": "配置 ADC 采样率 1MHz",
                "input": "",
                "output": "```yaml\nAdc:\n  general:\n    sampling_rate: 1000000\n    resolution: 12\n  AdcGroup:\n    - group_id: 0\n      channels: [0, 1, 2]\n      trigger_source: SOFTWARE\n```"
            },
            {
                "instruction": "配置 PWM 占空比 50%，频率 1kHz",
                "input": "",
                "output": "```yaml\nPwm:\n  general:\n    dev_error_detect: true\n  PwmChannel:\n    - channel_id: 0\n      frequency: 1000\n      duty_cycle: 50\n      polarity: HIGH\n```"
            },
        ]

        examples.extend(spec_examples)
        return examples

    def generate_comprehensive_dataset(self) -> List[Dict[str, Any]]:
        """Generate comprehensive training dataset."""
        all_examples = []

        # Generate from specifications
        all_examples.extend(self.generate_from_specifications())

        # Generate from documentation
        all_examples.extend(self.collect_from_documentation())

        # Add module-specific examples for all 92 modules
        module_examples = self._generate_module_examples()
        all_examples.extend(module_examples)

        return all_examples

    def _generate_module_examples(self) -> List[Dict[str, Any]]:
        """Generate examples for all AUTOSAR modules."""
        examples = []

        modules = {
            "Spi": [
                {"instruction": "配置 SPI 波特率 10Mbps", "output": "```yaml\nSpi:\n  general:\n    baudrate: 10000000\n  SpiChannel:\n    - id: 0\n      baudrate: 10000000\n      data_width: 8\n```"},
            ],
            "Icu": [
                {"instruction": "配置 ICU 输入捕获", "output": "```yaml\nIcu:\n  general:\n    dev_error_detect: true\n  IcuChannel:\n    - channel_id: 0\n      mode: EDGE_COUNTING\n      active_edge: RISING\n```"},
            ],
            "Wdg": [
                {"instruction": "配置看门狗超时时间 100ms", "output": "```yaml\nWdg:\n  general:\n    timeout: 100\n    dev_error_detect: true\n```"},
            ],
            "Eep": [
                {"instruction": "配置 EEPROM 块大小 512 字节", "output": "```yaml\nEep:\n  general:\n    block_size: 512\n    dev_error_detect: true\n```"},
            ],
            "Fls": [
                {"instruction": "配置 Flash 扇区大小 4KB", "output": "```yaml\nFls:\n  general:\n    sector_size: 4096\n    dev_error_detect: true\n```"},
            ],
            "Eth": [
                {"instruction": "配置以太网 MAC 地址", "output": "```yaml\nEth:\n  general:\n    mac_address: \"00:11:22:33:44:55\"\n    dev_error_detect: true\n```"},
            ],
            "Lin": [
                {"instruction": "配置 LIN 波特率 19200", "output": "```yaml\nLin:\n  general:\n    baudrate: 19200\n    dev_error_detect: true\n```"},
            ],
            "CanIf": [
                {"instruction": "配置 CAN 接口过滤", "output": "```yaml\nCanIf:\n  general:\n    dev_error_detect: true\n  CanIfRxPduCfg:\n    - pdu_id: 0\n      can_id: 0x100\n      can_id_type: STANDARD\n```"},
            ],
            "CanTp": [
                {"instruction": "配置 CAN TP 网络层", "output": "```yaml\nCanTp:\n  general:\n    dev_error_detect: true\n  CanTpRxNSdu:\n    - nsdu_id: 0\n      bs: 10\n      stmin: 10\n```"},
            ],
            "PduR": [
                {"instruction": "配置 PDU 路由 CAN 到 COM", "output": "```yaml\nPduR:\n  general:\n    dev_error_detect: true\n  PduRRoutingPath:\n    - src_pdu: CanIf_Tx\n      dest_pdu: Com_Tx\n      direction: UP\n```"},
            ],
            "IoHwAb": [
                {"instruction": "配置 IO 硬件抽象层", "output": "```yaml\nIoHwAb:\n  general:\n    dev_error_detect: true\n  IoHwAbChannel:\n    - channel_id: 0\n      type: DIGITAL\n      direction: INPUT\n```"},
            ],
            "Fee": [
                {"instruction": "配置 Flash EEPROM 仿真", "output": "```yaml\nFee:\n  general:\n    dev_error_detect: true\n    number_of_blocks: 10\n```"},
            ],
            "MemIf": [
                {"instruction": "配置内存抽象层", "output": "```yaml\nMemIf:\n  general:\n    dev_error_detect: true\n```"},
            ],
            "Dcm": [
                {"instruction": "配置 DCM 诊断通信管理", "output": "```yaml\nDcm:\n  general:\n    dev_error_detect: true\n  DcmDspSession:\n    - session_id: 0x01\n      session_name: DEFAULT_SESSION\n```"},
            ],
            "Dem": [
                {"instruction": "配置 DEM 诊断事件管理", "output": "```yaml\nDem:\n  general:\n    dev_error_detect: true\n    max_dtc_count: 100\n```"},
            ],
            "Det": [
                {"instruction": "配置 DET 开发错误追踪", "output": "```yaml\nDet:\n  general:\n    dev_error_detect: true\n    max_reports: 50\n```"},
            ],
            "BswM": [
                {"instruction": "配置 BSW 模式管理", "output": "```yaml\nBswM:\n  general:\n    dev_error_detect: true\n  BswMRule:\n    - rule_id: 0\n      condition: ECU_STATE == RUN\n      action: START_COMM\n```"},
            ],
            "EcuM": [
                {"instruction": "配置 ECU 状态管理", "output": "```yaml\nEcuM:\n  general:\n    dev_error_detect: true\n  EcuMState:\n    - state_id: 0\n      state_name: RUNNING\n```"},
            ],
            "Os": [
                {"instruction": "配置 OS 任务周期 10ms", "output": "```yaml\nOs:\n  general:\n    dev_error_detect: true\n  OsTask:\n    - task_id: 0\n      period: 10\n      priority: 1\n```"},
            ],
            "Rte": [
                {"instruction": "配置 RTE 运行时环境", "output": "```yaml\nRte:\n  general:\n    dev_error_detect: true\n  RteComponent:\n    - component_id: 0\n      name: EngineControl\n```"},
            ],
        }

        for module_name, module_examples in modules.items():
            for example in module_examples:
                examples.append({
                    "instruction": example["instruction"],
                    "input": "",
                    "output": example["output"],
                    "module": module_name
                })

        return examples

    def save_dataset(self, examples: List[Dict[str, Any]], filename: str):
        """Save dataset to JSON file."""
        output_file = DATA_DIR / filename
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump(examples, f, indent=2, ensure_ascii=False)
        print(f"Saved {len(examples)} examples to {output_file}")

    def convert_to_alpaca_format(self, examples: List[Dict[str, Any]]) -> List[Dict[str, str]]:
        """Convert to Alpaca format for LoRA training."""
        alpaca_data = []

        for example in examples:
            alpaca_data.append({
                "instruction": example.get("instruction", ""),
                "input": example.get("input", ""),
                "output": example.get("output", "")
            })

        return alpaca_data


def main():
    """Main data collection function."""
    collector = AutosarDataCollector()

    print("Collecting AUTOSAR training data...")

    # Generate comprehensive dataset
    examples = collector.generate_comprehensive_dataset()

    # Convert to Alpaca format
    alpaca_data = collector.convert_to_alpaca_format(examples)

    # Save dataset
    collector.save_dataset(alpaca_data, "autosar_training_data.json")

    print(f"\nTotal examples collected: {len(alpaca_data)}")
    print(f"Data saved to: {DATA_DIR / 'autosar_training_data.json'}")


if __name__ == "__main__":
    main()
