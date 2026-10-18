#!/usr/bin/env python3
"""
修正 bsw_config.yaml 中的模块命名，使其符合 AUTOSAR 规范。

AUTOSAR 命名规范：
- PascalCase（每个单词首字母大写）
- 缩写词通常全部大写（如 SM, NM, TP, If, Trcv, OC, IPDU）
- 数字保留在名称中（如 J1939, E2E）
"""

import re
from pathlib import Path

# AUTOSAR 标准模块命名映射
MODULE_NAME_CORRECTIONS = {
    # MCAL 层
    'Adc': 'Adc',
    'Can': 'Can',
    'Eth': 'Eth',
    'I2c': 'I2c',
    'Icu': 'Icu',
    'Lin': 'Lin',
    'Mcu': 'Mcu',
    'Ocu': 'Ocu',
    'Port': 'Port',
    'Pwm': 'Pwm',
    'Spi': 'Spi',
    'Wdg': 'Wdg',
    
    # ECUAL 层
    'CanIf': 'CanIf',
    'CanTp': 'CanTp',
    'CanTrcv': 'CanTrcv',
    'CanTSyn': 'CanTSyn',
    'Ea': 'Ea',
    'EthIf': 'EthIf',
    'EthSM': 'EthSM',
    'EthSwt': 'EthSwt',
    'EthTrcv': 'EthTrcv',
    'EthTSyn': 'EthTSyn',
    'Fee': 'Fee',
    'Fls': 'Fls',
    'FrIf': 'FrIf',
    'FrTp': 'FrTp',
    'IoHwAb': 'IoHwAb',
    'LinIf': 'LinIf',
    'LinTrcv': 'LinTrcv',
    'MemIf': 'MemIf',
    'PduR': 'PduR',
    
    # Services 层
    'CanM': 'CanM',
    'CanNm': 'CanNm',
    'CanSM': 'CanSM',
    'Com': 'Com',
    'ComM': 'ComM',
    'Crc': 'Crc',
    'CryIf': 'CryIf',
    'Crypto': 'Crypto',
    'Csm': 'Csm',
    'Dcm': 'Dcm',
    'Dem': 'Dem',
    'Det': 'Det',
    'Dio': 'Dio',
    'Dlt': 'Dlt',
    'DoCan': 'DoCan',
    'DoIP': 'DoIP',
    'E2E': 'E2E',
    'EcuC': 'EcuC',
    'EcuM': 'EcuM',
    'Eep': 'Eep',
    'FiM': 'FiM',
    'Flash': 'Flash',
    'FlStSt': 'FlStSt',
    'IpduM': 'IpduM',
    'J1939Nm': 'J1939Nm',
    'J1939Tp': 'J1939Tp',
    'KeyM': 'KeyM',
    'LdCom': 'LdCom',
    'LinM': 'LinM',
    'LinNm': 'LinNm',
    'LinSM': 'LinSM',
    'LinTp': 'LinTp',
    'LntM': 'LntM',
    'Mem': 'Mem',
    'Mqtt': 'Mqtt',
    'Nm': 'Nm',
    'NvM': 'NvM',
    'RamSafety': 'RamSafety',
    'RamTst': 'RamTst',
    'SchM': 'SchM',
    'Sd': 'Sd',
    'SecOC': 'SecOC',
    'SoAd': 'SoAd',
    'SomeIp': 'SomeIp',
    'SomeIpIf': 'SomeIpIf',
    'SomeIpSd': 'SomeIpSd',
    'SomeIpTp': 'SomeIpTp',
    'SomeIpXf': 'SomeIpXf',
    'Srp': 'Srp',
    'StbM': 'StbM',
    'SwC': 'SwC',
    'TcpIp': 'TcpIp',
    'Tm': 'Tm',
    'Uart': 'Uart',
    'UdpNm': 'UdpNm',
    'WdgIf': 'WdgIf',
    'WdgM': 'WdgM',
    'Xcp': 'Xcp',
    
    # Integration 层
    'BswM': 'BswM',
    
    # OS 层
    'Os': 'Os',
    
    # RTE 层
    'Rte': 'Rte',
}


def fix_module_names_in_yaml(yaml_path: Path) -> None:
    """修正 YAML 文件中的模块命名"""
    content = yaml_path.read_text(encoding='utf-8')
    lines = content.split('\n')
    modified_lines = []
    changes = []
    
    for line in lines:
        # 匹配模块定义行（2个空格缩进 + 模块名 + 冒号）
        match = re.match(r'^(\s{2})(\w+):(.*)$', line)
        if match:
            indent, module_name, rest = match.groups()
            if module_name in MODULE_NAME_CORRECTIONS:
                correct_name = MODULE_NAME_CORRECTIONS[module_name]
                if module_name != correct_name:
                    changes.append(f"  {module_name} -> {correct_name}")
                    line = f"{indent}{correct_name}:{rest}"
        modified_lines.append(line)
    
    if changes:
        yaml_path.write_text('\n'.join(modified_lines), encoding='utf-8')
        print(f"修正了 {len(changes)} 个模块命名:")
        for change in changes:
            print(change)
    else:
        print("所有模块命名已符合 AUTOSAR 规范")


def main():
    yaml_path = Path(__file__).parent.parent / 'config' / 'bsw_config.yaml'
    if not yaml_path.exists():
        print(f"错误：文件不存在 {yaml_path}")
        return
    
    print(f"检查文件：{yaml_path}")
    fix_module_names_in_yaml(yaml_path)


if __name__ == '__main__':
    main()
