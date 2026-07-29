#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CAN Matrix Excel文件解析器

支持标准CAN Matrix Excel格式，提取消息、信号、网络节点等信息
用于生成AUTOSAR Com模块配置
"""

import csv
import re
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Any
from pathlib import Path


@dataclass
class CanMatrixSignal:
    """CAN Matrix信号定义"""
    name: str
    message_name: str
    start_bit: int
    length: int
    byte_order: str  # 'Intel' or 'Motorola'
    is_signed: bool
    factor: float
    offset: float
    minimum: float
    maximum: float
    unit: str
    receiver: str
    sender: str
    init_value: float = 0.0
    comment: str = ""
    multiplexor: str = ""  # 空、M、数字


@dataclass
class CanMatrixMessage:
    """CAN Matrix消息定义"""
    id: int
    name: str
    dlc: int
    sender: str
    cycle_time: int
    signals: List[CanMatrixSignal] = field(default_factory=list)
    comment: str = ""

    @property
    def is_extended(self) -> bool:
        """是否为扩展帧"""
        return self.id > 0x7FF


@dataclass
class CanMatrix:
    """CAN Matrix数据"""
    nodes: List[str] = field(default_factory=list)
    messages: List[CanMatrixMessage] = field(default_factory=list)


class CanMatrixParser:
    """
    CAN Matrix文件解析器
    
    支持CSV格式和Excel格式（需安装pandas和openpyxl）
    支持标准Vector CAN Matrix和Vector Tools格式
    """
    
    # 标准列名映射
    COLUMN_MAPPINGS = {
        # DBC标准列名
        'message_id': ['Message ID', 'Message_ID', 'Msg ID', 'CAN ID', 'ID', '消息ID'],
        'message_name': ['Message Name', 'Message_Name', 'Msg Name', '消息名称'],
        'dlc': ['DLC', 'Length', 'Len', '长度'],
        'sender': ['Sender', 'Transmitter', 'Node', '发送节点', '发送方'],
        'cycle_time': ['Cycle Time', 'CycleTime', 'Period', '周期'],
        
        # 信号列名
        'signal_name': ['Signal Name', 'Signal_Name', 'Name', '信号名称'],
        'start_bit': ['Start Bit', 'StartBit', 'Start', '起始位'],
        'bit_length': ['Length', 'Bit Length', 'BitLength', 'Size', '长度'],
        'byte_order': ['Byte Order', 'ByteOrder', 'Order', 'Endian', '字节顺序'],
        'data_type': ['Data Type', 'DataType', 'Type', 'Value Type', '数据类型'],
        'factor': ['Factor', 'Scale', 'Resolution', '系数'],
        'offset': ['Offset', 'Bias', '偏移'],
        'minimum': ['Minimum', 'Min', '最小值'],
        'maximum': ['Maximum', 'Max', '最大值'],
        'unit': ['Unit', 'Units', '单位'],
        'receiver': ['Receiver', 'Receivers', 'Rx Node', '接收节点', '接收方'],
        'init_value': ['Init Value', 'InitValue', 'Initial Value', '初始值'],
        'comment': ['Comment', 'Description', 'Desc', '注释', '描述'],
        'multiplexor': ['Multiplexor', 'Multiplexer', 'Mux', '复用'],
    }
    
    def __init__(self):
        self.matrix = CanMatrix()
        self._column_map = {}
        
    def parse_csv(self, filepath: str) -> CanMatrix:
        """
        解析CSV文件
        
        Args:
            filepath: CSV文件路径
            
        Returns:
            CanMatrix: 解析后的CAN Matrix
        """
        path = Path(filepath)
        if not path.exists():
            raise FileNotFoundError(f"CSV文件不存在: {filepath}")
            
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            reader = csv.DictReader(f)
            self._build_column_map(reader.fieldnames or [])
            rows = list(reader)
            
        return self._parse_rows(rows)
    
    def parse_excel(self, filepath: str, sheet_name: Optional[str] = None) -> CanMatrix:
        """
        解析Excel文件
        
        Args:
            filepath: Excel文件路径
            sheet_name: 工作表名称（默认第一个）
            
        Returns:
            CanMatrix: 解析后的CAN Matrix
        """
        try:
            import pandas as pd
        except ImportError:
            raise ImportError("需要安装pandas: pip3 install pandas openpyxl")
            
        path = Path(filepath)
        if not path.exists():
            raise FileNotFoundError(f"Excel文件不存在: {filepath}")
            
        # 读取Excel
        if sheet_name:
            df = pd.read_excel(path, sheet_name=sheet_name)
        else:
            # 默认读取第一个工作表
            xl = pd.ExcelFile(path)
            df = pd.read_excel(path, sheet_name=xl.sheet_names[0])
            
        # 处理空值
        df = df.fillna('')
        
        # 构建列映射
        self._build_column_map(df.columns.tolist())
        
        # 转换为字典列表
        rows = df.to_dict('records')
        
        return self._parse_rows(rows)
    
    def _build_column_map(self, columns: List[str]):
        """构建列名映射"""
        self._column_map = {}
        
        for std_name, variants in self.COLUMN_MAPPINGS.items():
            for col in columns:
                if col in variants or any(v.lower() == col.lower() for v in variants):
                    self._column_map[std_name] = col
                    break
                    
        # 如果没有找到映射，尝试匹配子串
        for std_name, variants in self.COLUMN_MAPPINGS.items():
            if std_name not in self._column_map:
                for col in columns:
                    col_lower = col.lower().replace('_', '').replace(' ', '')
                    for var in variants:
                        var_lower = var.lower().replace('_', '').replace(' ', '')
                        if var_lower in col_lower or col_lower in var_lower:
                            self._column_map[std_name] = col
                            break
                    if std_name in self._column_map:
                        break
    
    def _get_value(self, row: Dict, key: str, default: Any = '') -> Any:
        """获取行中的值"""
        if key in self._column_map:
            col_name = self._column_map[key]
            if col_name in row:
                return row[col_name] if row[col_name] != '' else default
        return default
    
    def _parse_rows(self, rows: List[Dict]) -> CanMatrix:
        """解析行数据"""
        self.matrix = CanMatrix()
        
        # 收集所有节点
        nodes_set = set()
        
        # 按消息分组
        messages_dict: Dict[str, CanMatrixMessage] = {}
        
        for row in rows:
            # 获取消息信息
            msg_id_str = str(self._get_value(row, 'message_id', ''))
            msg_name = str(self._get_value(row, 'message_name', ''))
            
            if not msg_id_str and not msg_name:
                continue  # 跳过空行
                
            # 解析消息ID
            msg_id = self._parse_message_id(msg_id_str)
            
            # 获取消息其他信息
            dlc = self._parse_int(self._get_value(row, 'dlc', 8))
            sender = str(self._get_value(row, 'sender', 'ECU1'))
            cycle_time = self._parse_int(self._get_value(row, 'cycle_time', 100))
            msg_comment = str(self._get_value(row, 'comment', ''))
            
            # 收集节点
            nodes_set.add(sender)
            
            # 获取或创建消息
            if msg_name not in messages_dict:
                message = CanMatrixMessage(
                    id=msg_id,
                    name=msg_name,
                    dlc=dlc,
                    sender=sender,
                    cycle_time=cycle_time,
                    comment=msg_comment
                )
                messages_dict[msg_name] = message
            else:
                message = messages_dict[msg_name]
                
            # 解析信号
            signal_name = str(self._get_value(row, 'signal_name', ''))
            if signal_name:
                signal = self._parse_signal(row, signal_name, msg_name)
                if signal:
                    message.signals.append(signal)
                    # 收集接收方节点
                    if signal.receiver:
                        nodes_set.add(signal.receiver)
                        
        self.matrix.messages = list(messages_dict.values())
        self.matrix.nodes = sorted(list(nodes_set))
        
        return self.matrix
    
    def _parse_signal(self, row: Dict, signal_name: str, message_name: str) -> Optional[CanMatrixSignal]:
        """解析信号"""
        try:
            start_bit = self._parse_int(self._get_value(row, 'start_bit', 0))
            bit_length = self._parse_int(self._get_value(row, 'bit_length', 8))
            byte_order = str(self._get_value(row, 'byte_order', 'Intel'))
            
            # 解析数据类型
            data_type = str(self._get_value(row, 'data_type', 'unsigned'))
            is_signed = 'signed' in data_type.lower() or 'sint' in data_type.lower()
            
            # 解析因子和偏移
            factor = self._parse_float(self._get_value(row, 'factor', 1.0))
            offset = self._parse_float(self._get_value(row, 'offset', 0.0))
            
            # 解析范围
            minimum = self._parse_float(self._get_value(row, 'minimum', 0.0))
            maximum = self._parse_float(self._get_value(row, 'maximum', (2 ** bit_length - 1) * factor + offset))
            
            # 其他字段
            unit = str(self._get_value(row, 'unit', ''))
            receiver = str(self._get_value(row, 'receiver', ''))
            sender = str(self._get_value(row, 'sender', ''))
            init_value = self._parse_float(self._get_value(row, 'init_value', 0.0))
            comment = str(self._get_value(row, 'comment', ''))
            multiplexor = str(self._get_value(row, 'multiplexor', ''))
            
            return CanMatrixSignal(
                name=signal_name,
                message_name=message_name,
                start_bit=start_bit,
                length=bit_length,
                byte_order=byte_order,
                is_signed=is_signed,
                factor=factor,
                offset=offset,
                minimum=minimum,
                maximum=maximum,
                unit=unit,
                receiver=receiver,
                sender=sender,
                init_value=init_value,
                comment=comment,
                multiplexor=multiplexor
            )
        except Exception as e:
            print(f"警告: 解析信号 {signal_name} 失败: {e}")
            return None
    
    def _parse_message_id(self, id_str: str) -> int:
        """解析消息ID"""
        if not id_str:
            return 0
            
        id_str = str(id_str).strip()
        
        # 处理十六进制格式 (0x123)
        if id_str.startswith('0x') or id_str.startswith('0X'):
            try:
                return int(id_str, 16)
            except:
                pass
                
        # 处理带扩展标志的格式 (123x)
        if id_str.endswith('x') or id_str.endswith('X'):
            try:
                return int(id_str[:-1], 16)
            except:
                pass
                
        # 普通数字
        try:
            return int(float(id_str))
        except:
            return 0
    
    def _parse_int(self, value: Any, default: int = 0) -> int:
        """解析整数"""
        if isinstance(value, int):
            return value
        if isinstance(value, str):
            value = value.strip()
            if not value:
                return default
            try:
                # 处理带单位的情况
                match = re.match(r'(-?[\d.]+)', value)
                if match:
                    return int(float(match.group(1)))
            except:
                pass
        try:
            return int(float(value))
        except:
            return default
    
    def _parse_float(self, value: Any, default: float = 0.0) -> float:
        """解析浮点数"""
        if isinstance(value, (int, float)):
            return float(value)
        if isinstance(value, str):
            value = value.strip()
            if not value:
                return default
            try:
                # 处理科学记数法
                return float(value.replace(',', ''))
            except:
                pass
        try:
            return float(value)
        except:
            return default
    
    def to_com_config(self) -> Dict[str, Any]:
        """
        转换为Com模块配置格式
        
        Returns:
            dict: 适合生成Com配置的数据结构
        """
        config = {
            'ecu_name': self.matrix.nodes[0] if self.matrix.nodes else 'ECU0',
            'ipdus': [],
            'signals': [],
            'signal_groups': []
        }
        
        for msg in self.matrix.messages:
            # 确定方向
            direction = 'SEND'  # 默认发送
            
            # 创建IPDU
            ipdu = {
                'name': f"IPDU_{msg.name}",
                'message_id': msg.id,
                'dlc': msg.dlc,
                'direction': direction,
                'cycle_time': msg.cycle_time,
                'signals': []
            }
            
            # 处理信号
            for sig in msg.signals:
                # 确定数据类型
                if sig.is_signed:
                    if sig.length <= 8:
                        data_type = 'SINT8'
                    elif sig.length <= 16:
                        data_type = 'SINT16'
                    elif sig.length <= 32:
                        data_type = 'SINT32'
                    else:
                        data_type = 'SINT64'
                else:
                    if sig.length <= 8:
                        data_type = 'UINT8'
                    elif sig.length <= 16:
                        data_type = 'UINT16'
                    elif sig.length <= 32:
                        data_type = 'UINT32'
                    else:
                        data_type = 'UINT64'
                
                signal = {
                    'name': sig.name,
                    'ipdu': ipdu['name'],
                    'start_bit': sig.start_bit,
                    'bit_length': sig.length,
                    'byte_order': 'BIG_ENDIAN' if 'motorola' in sig.byte_order.lower() else 'LITTLE_ENDIAN',
                    'data_type': data_type,
                    'factor': sig.factor,
                    'offset': sig.offset,
                    'minimum': sig.minimum,
                    'maximum': sig.maximum,
                    'unit': sig.unit,
                    'init_value': sig.init_value,
                    'comment': sig.comment
                }
                ipdu['signals'].append(signal['name'])
                config['signals'].append(signal)
                
            config['ipdus'].append(ipdu)
            
        return config


def create_example_csv() -> str:
    """创建示例CSV内容"""
    return """Message ID,Message Name,DLC,Sender,Cycle Time,Signal Name,Start Bit,Length,Byte Order,Data Type,Factor,Offset,Minimum,Maximum,Unit,Receiver,Comment
0x100,EngineData,8,ECU1,100,EngineSpeed,0,16,Intel,unsigned,0.125,0,0,8000,rpm,ECU2,Engine RPM
0x100,EngineData,8,ECU1,100,EngineTemp,16,8,Intel,signed,1,-40,-40,215,degC,ECU2,Engine Temperature
0x100,EngineData,8,ECU1,100,EngineStatus,24,2,Intel,unsigned,1,0,0,3,,ECU2,Engine Status
0x200,VehicleSpeed,4,ECU1,50,Speed,0,16,Intel,unsigned,0.01,0,0,300,km/h,ECU2,Vehicle Speed
0x200,VehicleSpeed,4,ECU1,50,SpeedValid,16,1,Intel,unsigned,1,0,0,1,,ECU2,Speed Valid Flag
0x300,BatteryStatus,6,ECU2,500,Voltage,0,16,Intel,unsigned,0.01,0,0,65,V,ECU1,Battery Voltage
0x300,BatteryStatus,6,ECU2,500,Current,16,16,Intel,signed,0.001,-32,-32,32,A,ECU1,Battery Current
0x300,BatteryStatus,6,ECU2,500,SOC,32,8,Intel,unsigned,0.5,0,0,100,%,ECU1,State of Charge
"""


if __name__ == "__main__":
    # 测试示例
    csv_content = create_example_csv()
    
    # 保存为临时文件
    import tempfile
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write(csv_content)
        temp_path = f.name
    
    parser = CanMatrixParser()
    matrix = parser.parse_csv(temp_path)
    
    print("🚀 CAN Matrix CSV解析结果:")
    print(f"\n网络节点: {', '.join(matrix.nodes)}")
    print(f"消息数量: {len(matrix.messages)}")
    
    for msg in matrix.messages:
        print(f"\n  📬 {msg.name} (ID=0x{msg.id:X}, DLC={msg.dlc})")
        print(f"     发送方: {msg.sender}, 周期: {msg.cycle_time}ms")
        for sig in msg.signals:
            endian = "Motorola" if "motorola" in sig.byte_order.lower() else "Intel"
            signed = "Signed" if sig.is_signed else "Unsigned"
            print(f"     │─ {sig.name}: Start={sig.start_bit}, Len={sig.length}, {endian}, {signed}")
            print(f"     │   Factor={sig.factor}, Offset={sig.offset}, Range=[{sig.minimum}, {sig.maximum}] {sig.unit}")
    
    # 清理临时文件
    import os
    os.unlink(temp_path)
