#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DBC文件解析器

支持Vector DBC文件格式，提取消息、信号、网络节点等信息
用于生成AUTOSAR Com模块配置
"""

import re
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Any
from pathlib import Path


@dataclass
class DbcSignal:
    """DBC信号定义"""
    name: str
    start_bit: int
    length: int
    byte_order: int  # 0=Motorola (big endian), 1=Intel (little endian)
    is_signed: bool
    factor: float
    offset: float
    minimum: float
    maximum: float
    unit: str
    receiver: List[str] = field(default_factory=list)
    multiplexor: bool = False  # 是否为复用选择子
    multiplex_value: Optional[int] = None  # 复用值


@dataclass
class DbcMessage:
    """DBC消息定义"""
    id: int
    name: str
    dlc: int
    sender: str
    signals: List[DbcSignal] = field(default_factory=list)
    cycle_time: Optional[int] = None  # 周期时间(ms)
    comment: str = ""

    @property
    def is_extended(self) -> bool:
        """是否为扩展帧 (29位ID)"""
        return self.id > 0x7FF


@dataclass
class DbcNetwork:
    """DBC网络定义"""
    nodes: List[str] = field(default_factory=list)
    messages: List[DbcMessage] = field(default_factory=list)
    values_tables: Dict[str, Dict[int, str]] = field(default_factory=dict)


class DbcParser:
    """
    DBC文件解析器
    
    支持解析Vector格式的DBC文件
    """
    
    def __init__(self):
        self.network = DbcNetwork()
        self._current_line = 0
        
    def parse_file(self, filepath: str) -> DbcNetwork:
        """
        解析DBC文件
        
        Args:
            filepath: DBC文件路径
            
        Returns:
            DbcNetwork: 解析后的网络定义
        """
        path = Path(filepath)
        if not path.exists():
            raise FileNotFoundError(f"DBC文件不存在: {filepath}")
            
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            
        return self.parse_content(content)
    
    def parse_content(self, content: str) -> DbcNetwork:
        """
        解析DBC内容
        
        Args:
            content: DBC文件内容
            
        Returns:
            DbcNetwork: 解析后的网络定义
        """
        self.network = DbcNetwork()
        lines = content.split('\n')
        
        # 第一遍：解析节点和消息定义
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            self._current_line = i + 1
            
            if line.startswith('BU_:'):
                self._parse_nodes(line)
            elif line.startswith('BO_ '):
                # 解析消息，同时处理信号
                i = self._parse_message(lines, i)
            elif line.startswith(' SG_ ') or line.startswith('SG_ '):
                # 信号在消息后缓存的行中处理
                pass  # 已在_parse_message中处理
            elif line.startswith('CM_ '):
                self._parse_comment(line)
            elif line.startswith('VAL_ '):
                self._parse_value_table(line)
            elif line.startswith('BA_DEF_DEF_ '):
                pass  # 忽略默认值定义
            elif line.startswith('BA_DEF_ '):
                pass  # 忽略属性定义
            elif line.startswith('BA_ '):
                self._parse_attribute(line)
                
            i += 1
            
        return self.network
    
    def _parse_nodes(self, line: str):
        """解析网络节点"""
        # 格式: BU_: Node1 Node2 Node3
        parts = line.split()
        if len(parts) > 1:
            self.network.nodes = [p for p in parts[1:] if p]
    
    def _parse_message(self, lines: List[str], start_idx: int) -> int:
        """
        解析消息及其信号
        
        Returns:
            int: 下一行索引
        """
        line = lines[start_idx].strip()
        
        # 格式: BO_ msg_id msg_name: dlc sender
        # 示例: BO_ 1234 EngineData: 8 ECU1
        match = re.match(r'BO_\s+(\d+)\s+(\w+):\s+(\d+)\s+(\S+)', line)
        if not match:
            return start_idx
            
        msg_id = int(match.group(1))
        msg_name = match.group(2)
        dlc = int(match.group(3))
        sender = match.group(4)
        
        message = DbcMessage(
            id=msg_id,
            name=msg_name,
            dlc=dlc,
            sender=sender
        )
        
        # 解析信号（紧跟在消息后的行）
        i = start_idx + 1
        while i < len(lines):
            sig_line = lines[i].strip()
            
            # 如果不是信号定义，退出
            if not sig_line.startswith('SG_'):
                break
                
            signal = self._parse_signal(sig_line)
            if signal:
                message.signals.append(signal)
                
            i += 1
            
        self.network.messages.append(message)
        return i - 1  # 返回当前处理的行索引
    
    def _parse_signal(self, line: str) -> Optional[DbcSignal]:
        """解析信号定义"""
        # 格式: SG_ signal_name mux_def : start_bit|length@byte_order+ (factor,offset) [min|max] "unit" receiver
        # 示例: SG_ EngineSpeed : 0|16@1+ (0.125,0) [0|8000] "rpm" Vector__XXX
        # 示例(复用): SG_ EngineSpeed m0 : 0|16@1+ (0.125,0) [0|8000] "rpm" Vector__XXX
        # 示例(Mux): SG_ MuxSwitch M : 0|4@1+ (1,0) [0|15] "" Vector__XXX
        
        # 匹配复用标记
        mux_pattern = r'SG_\s+(\w+)\s+(M|m(\d+))\s*:'
        standard_pattern = r'SG_\s+(\w+)\s*:'
        
        multiplexor = False
        multiplex_value = None
        
        mux_match = re.match(mux_pattern, line)
        if mux_match:
            signal_name = mux_match.group(1)
            mux_type = mux_match.group(2)
            if mux_type == 'M':
                multiplexor = True
            else:
                multiplex_value = int(mux_match.group(3))
            # 重新定位到信号定义部分
            line = line[mux_match.end():]
        else:
            std_match = re.match(standard_pattern, line)
            if not std_match:
                return None
            signal_name = std_match.group(1)
            line = line[std_match.end():]
        
        # 解析信号参数
        # 格式: start_bit|length@byte_order+/- (factor,offset) [min|max] "unit" receiver
        pattern = r'\s*(\d+)\|(\d+)@(\d+)([+-])\s+\(([\d.]+),\s*([\d.-]+)\)\s+\[([\d.-]+)\|([\d.-]+)\]\s+"([^"]*)"\s+(\S+)'
        match = re.match(pattern, line)
        
        if not match:
            return None
            
        start_bit = int(match.group(1))
        length = int(match.group(2))
        byte_order = int(match.group(3))
        is_signed = match.group(4) == '-'
        factor = float(match.group(5))
        offset = float(match.group(6))
        minimum = float(match.group(7))
        maximum = float(match.group(8))
        unit = match.group(9)
        receiver_str = match.group(10)
        
        receivers = [r.strip() for r in receiver_str.split(',') if r.strip() and r != 'Vector__XXX']
        
        return DbcSignal(
            name=signal_name,
            start_bit=start_bit,
            length=length,
            byte_order=byte_order,
            is_signed=is_signed,
            factor=factor,
            offset=offset,
            minimum=minimum,
            maximum=maximum,
            unit=unit,
            receiver=receivers,
            multiplexor=multiplexor,
            multiplex_value=multiplex_value
        )
    
    def _parse_comment(self, line: str):
        """解析注释"""
        # 格式: CM_ BO_ msg_id "comment";
        msg_match = re.match(r'CM_\s+BO_\s+(\d+)\s+"([^"]*)"', line)
        if msg_match:
            msg_id = int(msg_match.group(1))
            comment = msg_match.group(2)
            for msg in self.network.messages:
                if msg.id == msg_id:
                    msg.comment = comment
                    break
                    
        # 格式: CM_ SG_ msg_id signal_name "comment";
        sig_match = re.match(r'CM_\s+SG_\s+(\d+)\s+(\w+)\s+"([^"]*)"', line)
        if sig_match:
            msg_id = int(sig_match.group(1))
            sig_name = sig_match.group(2)
            comment = sig_match.group(3)
            for msg in self.network.messages:
                if msg.id == msg_id:
                    for sig in msg.signals:
                        if sig.name == sig_name:
                            # 可以扩展信号注释
                            break
                    break
    
    def _parse_attribute(self, line: str):
        """解析属性"""
        # 格式: BA_ "AttributeName" BO_ msg_id value;
        cycle_match = re.match(r'BA_\s+"GenMsgCycleTime"\s+BO_\s+(\d+)\s+(\d+)', line)
        if cycle_match:
            msg_id = int(cycle_match.group(1))
            cycle_time = int(cycle_match.group(2))
            for msg in self.network.messages:
                if msg.id == msg_id:
                    msg.cycle_time = cycle_time
                    break
    
    def _parse_value_table(self, line: str):
        """解析值表"""
        # 格式: VAL_ msg_id signal_name val1 "desc1" val2 "desc2";
        match = re.match(r'VAL_\s+(\d+)\s+(\w+)\s+(.+);', line)
        if match:
            msg_id = int(match.group(1))
            sig_name = match.group(2)
            values_str = match.group(3)
            
            values = {}
            val_pattern = r'(\d+)\s+"([^"]*)"'
            for val_match in re.finditer(val_pattern, values_str):
                val = int(val_match.group(1))
                desc = val_match.group(2)
                values[val] = desc
                
            if values:
                key = f"{msg_id}_{sig_name}"
                self.network.values_tables[key] = values
    
    def to_com_config(self) -> Dict[str, Any]:
        """
        转换为Com模块配置格式
        
        Returns:
            dict: 适合生成Com配置的数据结构
        """
        config = {
            'ecu_name': 'ECU0',
            'ipdus': [],
            'signals': [],
            'signal_groups': []
        }
        
        for msg in self.network.messages:
            # 创建IPDU
            ipdu = {
                'name': f"{msg.name}_IPDU",
                'message_id': msg.id,
                'dlc': msg.dlc,
                'direction': 'SEND' if msg.sender in self.network.nodes else 'RECEIVE',
                'cycle_time': msg.cycle_time or 0,
                'signals': []
            }
            
            # 处理信号
            for sig in msg.signals:
                signal = {
                    'name': sig.name,
                    'ipdu': ipdu['name'],
                    'start_bit': sig.start_bit,
                    'bit_length': sig.length,
                    'byte_order': 'BIG_ENDIAN' if sig.byte_order == 0 else 'LITTLE_ENDIAN',
                    'data_type': 'SINT16' if sig.is_signed and sig.length <= 16 else (
                        'SINT32' if sig.is_signed else (
                            'UINT8' if sig.length <= 8 else (
                                'UINT16' if sig.length <= 16 else 'UINT32'
                            )
                        )
                    ),
                    'factor': sig.factor,
                    'offset': sig.offset,
                    'minimum': sig.minimum,
                    'maximum': sig.maximum,
                    'unit': sig.unit,
                    'init_value': 0
                }
                ipdu['signals'].append(signal['name'])
                config['signals'].append(signal)
                
            config['ipdus'].append(ipdu)
            
        return config


def create_example_dbc() -> str:
    """创建示例DBC内容"""
    return """VERSION ""


NS_ :
    NS_DESC_
    CM_
    BA_DEF_
    BA_
    VAL_
    CAT_DEF_
    CAT_
    FILTER
    BA_DEF_DEF_
    EV_DATA_
    ENVVAR_DATA_
    SGTYPE_
    SGTYPE_VAL_
    BA_DEF_SGTYPE_
    BA_SGTYPE_
    SIG_TYPE_REF_
    VAL_TABLE_
    SIG_GROUP_
    SIG_VALTYPE_
    SIGTYPE_VALTYPE_
    BO_TX_BU_
    BA_DEF_REL_
    BA_REL_
    BA_DEF_DEF_REL_
    BU_SG_REL_
    BU_EV_REL_
    BU_BO_REL_
    SG_MUL_VAL_

BS_:

BU_: ECU1 ECU2 Gateway

BO_ 100 EngineData: 8 ECU1
 SG_ EngineSpeed : 0|16@1+ (0.125,0) [0|8000] "rpm" ECU2,Gateway
 SG_ EngineTemp : 16|8@1+ (1,-40) [-40|215] "degC" ECU2,Gateway
 SG_ EngineStatus : 24|2@1+ (1,0) [0|3] "" ECU2

BO_ 200 VehicleSpeed: 4 ECU1
 SG_ Speed : 0|16@1+ (0.01,0) [0|300] "km/h" ECU2,Gateway
 SG_ SpeedValid : 16|1@1+ (1,0) [0|1] "" ECU2

BO_ 300 BatteryStatus: 6 ECU2
 SG_ Voltage : 0|16@1+ (0.01,0) [0|65] "V" ECU1,Gateway
 SG_ Current : 16|16@1- (0.001,-32) [-32|32] "A" ECU1
 SG_ SOC : 32|8@1+ (0.5,0) [0|100] "%" ECU1

CM_ BO_ 100 "Engine sensor data";
CM_ BO_ 200 "Vehicle speed information";
CM_ BO_ 300 "Battery management data";

BA_DEF_ BO_ "GenMsgCycleTime" INT 0 65535;
BA_ "GenMsgCycleTime" BO_ 100 100;
BA_ "GenMsgCycleTime" BO_ 200 50;
BA_ "GenMsgCycleTime" BO_ 300 500;
"""


if __name__ == "__main__":
    # 测试示例
    dbc_content = create_example_dbc()
    
    parser = DbcParser()
    network = parser.parse_content(dbc_content)
    
    print("🚀 DBC文件解析结果:")
    print(f"\n网络节点: {', '.join(network.nodes)}")
    print(f"消息数量: {len(network.messages)}")
    
    for msg in network.messages:
        print(f"\n  📬 {msg.name} (ID=0x{msg.id:X}, DLC={msg.dlc})")
        print(f"     发送方: {msg.sender}, 周期: {msg.cycle_time}ms")
        for sig in msg.signals:
            endian = "Big" if sig.byte_order == 0 else "Little"
            signed = "Signed" if sig.is_signed else "Unsigned"
            print(f"     │─ {sig.name}: {sig.start_bit}|{sig.length}@{endian} {signed}")
            print(f"     │   Factor={sig.factor}, Offset={sig.offset}, Range=[{sig.minimum}, {sig.maximum}] {sig.unit}")
