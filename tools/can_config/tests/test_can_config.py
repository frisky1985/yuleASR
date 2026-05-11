#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CAN配置工具单元测试
"""

import sys
import unittest
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from dbc_parser import DbcParser, DbcSignal, DbcMessage, DbcNetwork, create_example_dbc
from can_matrix_parser import CanMatrixParser, CanMatrixSignal, CanMatrixMessage, create_example_csv
from com_config_generator import ComConfigGenerator


class TestDbcParser(unittest.TestCase):
    """DBC解析器测试"""
    
    def setUp(self):
        self.parser = DbcParser()
    
    def test_parse_nodes(self):
        """测试节点解析"""
        dbc_content = """VERSION ""
NS_ :
BS_:
BU_: ECU1 ECU2 Gateway
"""
        network = self.parser.parse_content(dbc_content)
        self.assertEqual(len(network.nodes), 3)
        self.assertIn('ECU1', network.nodes)
        self.assertIn('ECU2', network.nodes)
        self.assertIn('Gateway', network.nodes)
    
    def test_parse_message(self):
        """测试消息解析"""
        dbc_content = """VERSION ""
BU_: ECU1 ECU2
BO_ 100 EngineData: 8 ECU1
 SG_ EngineSpeed : 0|16@1+ (0.125,0) [0|8000] "rpm" ECU2
 SG_ EngineTemp : 16|8@1+ (1,-40) [-40|215] "degC" ECU2
"""
        network = self.parser.parse_content(dbc_content)
        self.assertEqual(len(network.messages), 1)
        
        msg = network.messages[0]
        self.assertEqual(msg.id, 100)
        self.assertEqual(msg.name, 'EngineData')
        self.assertEqual(msg.dlc, 8)
        self.assertEqual(msg.sender, 'ECU1')
    
    def test_parse_signal(self):
        """测试信号解析"""
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 100 EngineData: 8 ECU1
 SG_ EngineSpeed : 0|16@1+ (0.125,0) [0|8000] "rpm" Vector__XXX
"""
        network = self.parser.parse_content(dbc_content)
        msg = network.messages[0]
        self.assertEqual(len(msg.signals), 1)
        
        sig = msg.signals[0]
        self.assertEqual(sig.name, 'EngineSpeed')
        self.assertEqual(sig.start_bit, 0)
        self.assertEqual(sig.length, 16)
        self.assertEqual(sig.byte_order, 1)  # Intel
        self.assertFalse(sig.is_signed)
        self.assertEqual(sig.factor, 0.125)
        self.assertEqual(sig.offset, 0.0)
        self.assertEqual(sig.minimum, 0.0)
        self.assertEqual(sig.maximum, 8000.0)
        self.assertEqual(sig.unit, 'rpm')
    
    def test_parse_cycle_time(self):
        """测试周期时间解析"""
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 100 EngineData: 8 ECU1
 SG_ Speed : 0|8@1+ (1,0) [0|255] "" Vector__XXX
BA_DEF_ BO_ "GenMsgCycleTime" INT 0 65535;
BA_ "GenMsgCycleTime" BO_ 100 100;
"""
        network = self.parser.parse_content(dbc_content)
        msg = network.messages[0]
        self.assertEqual(msg.cycle_time, 100)
    
    def test_to_com_config(self):
        """测试转换为Com配置"""
        dbc_content = create_example_dbc()
        network = self.parser.parse_content(dbc_content)
        config = self.parser.to_com_config()
        
        self.assertIn('ipdus', config)
        self.assertIn('signals', config)
        self.assertGreater(len(config['ipdus']), 0)
        self.assertGreater(len(config['signals']), 0)


class TestCanMatrixParser(unittest.TestCase):
    """CAN Matrix解析器测试"""
    
    def setUp(self):
        self.parser = CanMatrixParser()
    
    def test_parse_csv(self):
        """测试CSV解析"""
        csv_content = create_example_csv()
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
            f.write(csv_content)
            temp_path = f.name
        
        try:
            matrix = self.parser.parse_csv(temp_path)
            self.assertGreater(len(matrix.messages), 0)
            self.assertGreater(len(matrix.nodes), 0)
        finally:
            Path(temp_path).unlink()
    
    def test_parse_message_id_hex(self):
        """测试十六进制ID解析"""
        test_cases = [
            ('0x100', 256),
            ('0x123', 291),
            ('100', 100),
            ('123x', 291),
        ]
        
        for id_str, expected in test_cases:
            result = self.parser._parse_message_id(id_str)
            self.assertEqual(result, expected, f"Failed for {id_str}")
    
    def test_column_mapping(self):
        """测试列名映射"""
        columns = ['Message ID', 'Message Name', 'DLC', 'Signal Name', 'Start Bit', 'Length']
        self.parser._build_column_map(columns)
        
        self.assertIn('message_id', self.parser._column_map)
        self.assertIn('message_name', self.parser._column_map)
        self.assertIn('signal_name', self.parser._column_map)


class TestComConfigGenerator(unittest.TestCase):
    """Com配置生成器测试"""
    
    def setUp(self):
        self.test_config = {
            'ecu_name': 'TestECU',
            'ipdus': [
                {
                    'name': 'IPDU_Test',
                    'message_id': 0x100,
                    'dlc': 8,
                    'direction': 'SEND',
                    'cycle_time': 100,
                    'signals': ['TestSignal']
                }
            ],
            'signals': [
                {
                    'name': 'TestSignal',
                    'ipdu': 'IPDU_Test',
                    'start_bit': 0,
                    'bit_length': 16,
                    'byte_order': 'LITTLE_ENDIAN',
                    'data_type': 'UINT16',
                    'factor': 1.0,
                    'offset': 0.0,
                    'minimum': 0.0,
                    'maximum': 65535.0,
                    'init_value': 0,
                }
            ],
            'signal_groups': []
        }
    
    def test_generate_cfg_h(self):
        """测试生成头文件"""
        generator = ComConfigGenerator(self.test_config)
        content = generator.generate_cfg_h()
        
        self.assertIn('#ifndef COM_CFG_H', content)
        self.assertIn('#define COM_CFG_H', content)
        self.assertIn('COM_NUM_OF_IPDUS', content)
        self.assertIn('IPDU_Test_ID', content)
        self.assertIn('ComConf_ComSignal_TestSignal', content)
    
    def test_generate_cfg_c(self):
        """测试生成源文件"""
        generator = ComConfigGenerator(self.test_config)
        content = generator.generate_cfg_c()
        
        self.assertIn('#include "Com_Cfg.h"', content)
        self.assertIn('Com_IPduConfig', content)
        self.assertIn('Com_SignalConfig', content)
        self.assertIn('TestSignal', content)
    
    def test_generate_files(self):
        """测试生成文件"""
        with tempfile.TemporaryDirectory() as tmpdir:
            generator = ComConfigGenerator(self.test_config)
            cfg_h, cfg_c = generator.generate(tmpdir, prefix="Test_")
            
            self.assertTrue(Path(cfg_h).exists())
            self.assertTrue(Path(cfg_c).exists())
            
            # 验证内容
            with open(cfg_h, 'r') as f:
                content = f.read()
                self.assertIn('Test_', content)


class TestIntegration(unittest.TestCase):
    """集成测试"""
    
    def test_dbc_to_com_config(self):
        """测试DBC到Com配置的完整流程"""
        # 1. 解析DBC
        dbc_content = create_example_dbc()
        dbc_parser = DbcParser()
        network = dbc_parser.parse_content(dbc_content)
        
        # 2. 转换为Com配置
        config = dbc_parser.to_com_config()
        
        # 3. 生成配置文件
        generator = ComConfigGenerator(config)
        
        with tempfile.TemporaryDirectory() as tmpdir:
            cfg_h, cfg_c = generator.generate(tmpdir)
            
            # 验证文件存在
            self.assertTrue(Path(cfg_h).exists())
            self.assertTrue(Path(cfg_c).exists())
            
            # 验证内容包含关键字
            with open(cfg_h, 'r') as f:
                content = f.read()
                self.assertIn('COM_CFG_H', content)
                self.assertIn('EngineData', content)
    
    def test_csv_to_com_config(self):
        """测试CSV到Com配置的完整流程"""
        csv_content = create_example_csv()
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
            f.write(csv_content)
            temp_path = f.name
        
        try:
            # 1. 解析CSV
            matrix_parser = CanMatrixParser()
            matrix = matrix_parser.parse_csv(temp_path)
            
            # 2. 转换为Com配置
            config = matrix_parser.to_com_config()
            
            # 3. 生成配置文件
            generator = ComConfigGenerator(config)
            
            with tempfile.TemporaryDirectory() as tmpdir:
                cfg_h, cfg_c = generator.generate(tmpdir)
                
                self.assertTrue(Path(cfg_h).exists())
                self.assertTrue(Path(cfg_c).exists())
        finally:
            Path(temp_path).unlink()


def run_tests():
    """运行测试"""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    suite.addTests(loader.loadTestsFromTestCase(TestDbcParser))
    suite.addTests(loader.loadTestsFromTestCase(TestCanMatrixParser))
    suite.addTests(loader.loadTestsFromTestCase(TestComConfigGenerator))
    suite.addTests(loader.loadTestsFromTestCase(TestIntegration))
    
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    sys.exit(run_tests())
