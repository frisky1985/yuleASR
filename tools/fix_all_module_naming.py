#!/usr/bin/env python3
"""
全面修正模块命名，包括：
1. bsw_config.yaml - 配置文件
2. schemas/*.json - Schema 文件
3. templates/*.j2 - 模板文件
4. Python/TypeScript 代码中的硬编码引用
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

# 文件名映射（用于重命名文件）
FILENAME_CORRECTIONS = {
    f'{old}_schema.json': f'{new}_schema.json'
    for old, new in MODULE_NAME_CORRECTIONS.items()
}
FILENAME_CORRECTIONS.update({
    f'{old}_Cfg.h.j2': f'{new}_Cfg.h.j2'
    for old, new in MODULE_NAME_CORRECTIONS.items()
})


def fix_yaml_file(yaml_path: Path) -> int:
    """修正 YAML 文件中的模块命名"""
    content = yaml_path.read_text(encoding='utf-8')
    lines = content.split('\n')
    modified_lines = []
    changes = 0
    
    for line in lines:
        # 匹配模块定义行（2个空格缩进 + 模块名 + 冒号）
        match = re.match(r'^(\s{2})(\w+):(.*)$', line)
        if match:
            indent, module_name, rest = match.groups()
            if module_name in MODULE_NAME_CORRECTIONS:
                correct_name = MODULE_NAME_CORRECTIONS[module_name]
                if module_name != correct_name:
                    changes += 1
                    line = f"{indent}{correct_name}:{rest}"
        modified_lines.append(line)
    
    if changes > 0:
        yaml_path.write_text('\n'.join(modified_lines), encoding='utf-8')
    
    return changes


def fix_schema_file(schema_path: Path) -> int:
    """修正 Schema 文件中的模块命名"""
    content = schema_path.read_text(encoding='utf-8')
    original = content
    changes = 0
    
    # 替换 $id 中的模块名
    for old, new in MODULE_NAME_CORRECTIONS.items():
        if old != new:
            # 替换 URL 中的模块名（如 https://yuletech.com/schemas/adc.json -> Adc.json）
            content = content.replace(f'/schemas/{old}.json', f'/schemas/{new}.json')
            content = content.replace(f'/schemas/{old}/', f'/schemas/{new}/')
            # 替换 title 和 description 中的模块名
            content = content.replace(f'{old} Configuration', f'{new} Configuration')
            content = content.replace(f'{old} module', f'{new} module')
            content = content.replace(f'{old} Module', f'{new} Module')
    
    if content != original:
        # 计算变化数
        changes = sum(1 for old, new in MODULE_NAME_CORRECTIONS.items() 
                     if old != new and old in original)
        schema_path.write_text(content, encoding='utf-8')
    
    return changes


def fix_template_file(template_path: Path) -> int:
    """修正模板文件中的模块命名"""
    content = template_path.read_text(encoding='utf-8')
    original = content
    changes = 0
    
    # 替换模板中的模块名引用
    for old, new in MODULE_NAME_CORRECTIONS.items():
        if old != new:
            # 替换各种格式的模块名引用
            content = content.replace(f'{old}_', f'{new}_')
            content = content.replace(f'{old}.', f'{new}.')
            content = content.replace(f'{old} ', f'{new} ')
            content = content.replace(f'"{old}"', f'"{new}"')
            content = content.replace(f"'{old}'", f"'{new}'")
    
    if content != original:
        changes = sum(1 for old, new in MODULE_NAME_CORRECTIONS.items() 
                     if old != new and old in original)
        template_path.write_text(content, encoding='utf-8')
    
    return changes


def fix_python_file(py_path: Path) -> int:
    """修正 Python 文件中的模块命名"""
    content = py_path.read_text(encoding='utf-8')
    original = content
    changes = 0
    
    # 替换 Python 代码中的模块名字符串
    for old, new in MODULE_NAME_CORRECTIONS.items():
        if old != new:
            # 替换字符串中的模块名
            content = content.replace(f"'{old}'", f"'{new}'")
            content = content.replace(f'"{old}"', f'"{new}"')
            # 替换注释中的模块名（谨慎）
            # content = content.replace(f' # {old}', f' # {new}')
    
    if content != original:
        changes = sum(1 for old, new in MODULE_NAME_CORRECTIONS.items() 
                     if old != new and old in original)
        py_path.write_text(content, encoding='utf-8')
    
    return changes


def fix_typescript_file(ts_path: Path) -> int:
    """修正 TypeScript 文件中的模块命名"""
    content = ts_path.read_text(encoding='utf-8')
    original = content
    changes = 0
    
    # 替换 TypeScript 代码中的模块名字符串
    for old, new in MODULE_NAME_CORRECTIONS.items():
        if old != new:
            content = content.replace(f"'{old}'", f"'{new}'")
            content = content.replace(f'"{old}"', f'"{new}"')
    
    if content != original:
        changes = sum(1 for old, new in MODULE_NAME_CORRECTIONS.items() 
                     if old != new and old in original)
        ts_path.write_text(content, encoding='utf-8')
    
    return changes


def rename_files(base_path: Path) -> int:
    """重命名文件以符合新的模块命名"""
    renames = 0
    
    # 重命名 schema 文件
    schemas_dir = base_path / 'schemas'
    if schemas_dir.exists():
        for schema_file in schemas_dir.rglob('*_schema.json'):
            old_name = schema_file.name
            for old, new in MODULE_NAME_CORRECTIONS.items():
                if old_name == f'{old}_schema.json':
                    new_name = f'{new}_schema.json'
                    new_path = schema_file.parent / new_name
                    if not new_path.exists():
                        schema_file.rename(new_path)
                        print(f"  重命名：{old_name} -> {new_name}")
                        renames += 1
                    break
    
    # 重命名模板文件
    templates_dir = base_path / 'templates'
    if templates_dir.exists():
        for template_file in templates_dir.rglob('*_Cfg.h.j2'):
            old_name = template_file.name
            for old, new in MODULE_NAME_CORRECTIONS.items():
                if old_name == f'{old}_Cfg.h.j2':
                    new_name = f'{new}_Cfg.h.j2'
                    new_path = template_file.parent / new_name
                    if not new_path.exists():
                        template_file.rename(new_path)
                        print(f"  重命名：{old_name} -> {new_name}")
                        renames += 1
                    break
    
    return renames


def main():
    base_path = Path(__file__).parent.parent
    total_changes = 0
    total_renames = 0
    
    print("=" * 60)
    print("修正模块命名以符合 AUTOSAR 规范")
    print("=" * 60)
    
    # 1. 修正 bsw_config.yaml
    print("\n1. 修正 bsw_config.yaml")
    yaml_path = base_path / 'config' / 'bsw_config.yaml'
    if yaml_path.exists():
        changes = fix_yaml_file(yaml_path)
        print(f"  修正了 {changes} 个模块命名")
        total_changes += changes
    
    # 2. 修正 Schema 文件
    print("\n2. 修正 Schema 文件")
    schemas_dir = base_path / 'schemas'
    if schemas_dir.exists():
        for schema_file in schemas_dir.rglob('*_schema.json'):
            changes = fix_schema_file(schema_file)
            if changes > 0:
                print(f"  {schema_file.name}: {changes} 处修改")
                total_changes += changes
    
    # 3. 修正模板文件
    print("\n3. 修正模板文件")
    templates_dir = base_path / 'templates'
    if templates_dir.exists():
        for template_file in templates_dir.rglob('*_Cfg.h.j2'):
            changes = fix_template_file(template_file)
            if changes > 0:
                print(f"  {template_file.name}: {changes} 处修改")
                total_changes += changes
    
    # 4. 修正 Python 代码
    print("\n4. 修正 Python 代码")
    for py_file in base_path.rglob('*.py'):
        if '.venv' in str(py_file) or 'node_modules' in str(py_file):
            continue
        changes = fix_python_file(py_file)
        if changes > 0:
            print(f"  {py_file.relative_to(base_path)}: {changes} 处修改")
            total_changes += changes
    
    # 5. 修正 TypeScript 代码
    print("\n5. 修正 TypeScript 代码")
    for ts_file in base_path.rglob('*.ts'):
        if 'node_modules' in str(ts_file) or '.d.ts' in str(ts_file):
            continue
        changes = fix_typescript_file(ts_file)
        if changes > 0:
            print(f"  {ts_file.relative_to(base_path)}: {changes} 处修改")
            total_changes += changes
    
    # 6. 重命名文件
    print("\n6. 重命名文件")
    renames = rename_files(base_path)
    total_renames += renames
    print(f"  重命名了 {renames} 个文件")
    
    print("\n" + "=" * 60)
    print(f"总计：{total_changes} 处内容修改，{total_renames} 个文件重命名")
    print("=" * 60)


if __name__ == '__main__':
    main()
