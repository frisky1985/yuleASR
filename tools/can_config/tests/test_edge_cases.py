#!/usr/bin/env python3
"""
Edge case and extended tests for CAN configuration tool modules.

Covers:
- DBC parser: extended frames, multiplex signals, value tables, errors
- CAN Matrix parser: column matching, edge case parsing, message ID formats
- Com Config Generator: all combos of config options, summary edge cases
"""

import sys
import unittest
import tempfile
from pathlib import Path
from datetime import datetime

sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from dbc_parser import DbcParser, DbcSignal, DbcMessage, DbcNetwork, create_example_dbc
from can_matrix_parser import CanMatrixParser, CanMatrixSignal, CanMatrixMessage, CanMatrix, create_example_csv
from com_config_generator import ComConfigGenerator


# ============================================================================
# DBC Parser Edge Cases
# ============================================================================

class TestDbcParserEdgeCases(unittest.TestCase):
    """DBC parser edge cases"""

    def setUp(self):
        self.parser = DbcParser()

    def test_extended_frame_parsing(self):
        """Test that extended frames (ID > 0x7FF) are correctly identified"""
        dbc_content = """VERSION ""
BU_: ECU1 ECU2
BO_ 2048 ExtendedMessage: 8 ECU1
 SG_ Data : 0|32@1+ (1,0) [0|4294967295] "x" ECU2
"""
        network = self.parser.parse_content(dbc_content)
        self.assertEqual(len(network.messages), 1)
        msg = network.messages[0]
        self.assertTrue(msg.is_extended)

    def test_standard_frame_parsing(self):
        """Test that standard frames (ID <= 0x7FF) are correctly identified"""
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 100 StdMessage: 8 ECU1
 SG_ S : 0|8@1+ (1,0) [0|255] "" Vector__XXX
"""
        network = self.parser.parse_content(dbc_content)
        self.assertEqual(len(network.messages), 1)
        msg = network.messages[0]
        self.assertFalse(msg.is_extended)

    def test_multiplexed_signal_mux_switch(self):
        """Test parsing of multiplexor switch signal (M)"""
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 500 MuxMessage: 8 ECU1
 SG_ MuxSwitch M : 0|4@1+ (1,0) [0|15] "" ECU2
 SG_ Signal1 m0 : 4|8@1+ (1,0) [0|255] "" ECU2
 SG_ Signal2 m1 : 12|8@1+ (1,0) [0|255] "" ECU2
"""
        network = self.parser.parse_content(dbc_content)
        self.assertEqual(len(network.messages), 1)
        msg = network.messages[0]
        self.assertEqual(len(msg.signals), 3)

        mux_sig = msg.signals[0]
        self.assertEqual(mux_sig.name, 'MuxSwitch')
        self.assertTrue(mux_sig.multiplexor)

        sig1 = msg.signals[1]
        self.assertEqual(sig1.multiplex_value, 0)

        sig2 = msg.signals[2]
        self.assertEqual(sig2.multiplex_value, 1)

    def test_signed_signal_parsing(self):
        """Test parsing of signed signals"""
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 100 SensorData: 8 ECU1
 SG_ Temperature : 0|16@1- (0.01,-40) [-40|125] "degC" ECU2
 SG_ Pressure : 16|12@1- (1,0) [0|4095] "kPa" ECU2
"""
        network = self.parser.parse_content(dbc_content)
        msg = network.messages[0]
        temp_sig = msg.signals[0]
        self.assertTrue(temp_sig.is_signed)
        self.assertEqual(temp_sig.factor, 0.01)
        self.assertEqual(temp_sig.offset, -40.0)
        # Signed signal should be parsed correctly
        press_sig = msg.signals[1]
        self.assertTrue(press_sig.is_signed)

    def test_value_table_parsing(self):
        """Test parsing of value tables (VAL_ entries)"""
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 200 StatusMsg: 8 ECU1
 SG_ Status : 0|3@1+ (1,0) [0|7] "" ECU2
VAL_ 200 Status 0 "OFF" 1 "RUNNING" 2 "STANDBY" 3 "ERROR" ;
"""
        network = self.parser.parse_content(dbc_content)
        self.assertEqual(len(network.values_tables), 1)
        key = "200_Status"
        self.assertIn(key, network.values_tables)
        self.assertEqual(network.values_tables[key], {0: "OFF", 1: "RUNNING", 2: "STANDBY", 3: "ERROR"})

    def test_comment_parsing(self):
        """Test parsing of comments (CM_ entries)"""
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 100 EngineData: 8 ECU1
 SG_ Speed : 0|16@1+ (1,0) [0|8000] "rpm" ECU2
CM_ BO_ 100 "Engine sensor data module";
"""
        network = self.parser.parse_content(dbc_content)
        msg = network.messages[0]
        self.assertEqual(msg.comment, "Engine sensor data module")

    def test_cycle_time_parsing(self):
        """Test parsing of GenMsgCycleTime attribute"""
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 100 Data: 8 ECU1
 SG_ S : 0|8@1+ (1,0) [0|255] "" Vector__XXX
BA_DEF_ BO_ "GenMsgCycleTime" INT 0 65535;
BA_ "GenMsgCycleTime" BO_ 100 50;
"""
        network = self.parser.parse_content(dbc_content)
        msg = network.messages[0]
        self.assertEqual(msg.cycle_time, 50)

    def test_to_com_config_maps_correctly(self):
        """Test that DBC to_com_config produces correct mappings"""
        dbc_content = create_example_dbc()
        network = self.parser.parse_content(dbc_content)
        config = self.parser.to_com_config()

        self.assertIn('ecu_name', config)
        self.assertIn('ipdus', config)
        self.assertIn('signals', config)

        # Check that engine-related IPDU exists
        ipdu_names = [ipdu['name'] for ipdu in config['ipdus']]
        self.assertIn('EngineData_IPDU', ipdu_names)

        # Check signal mapping
        signal_names = [s['name'] for s in config['signals']]
        self.assertIn('EngineSpeed', signal_names)
        self.assertIn('EngineTemp', signal_names)

    def test_missing_file_raises(self):
        """Test that parsing non-existent file raises FileNotFoundError"""
        with self.assertRaises(FileNotFoundError):
            self.parser.parse_file("/nonexistent/file.dbc")

    def test_multiple_receivers(self):
        """Test that multiple receivers are parsed"""
        dbc_content = """VERSION ""
BU_: ECU1 ECU2 Gateway
BO_ 100 Data: 8 ECU1
 SG_ S : 0|8@1+ (1,0) [0|255] "" ECU2,Gateway
"""
        network = self.parser.parse_content(dbc_content)
        msg = network.messages[0]
        sig = msg.signals[0]
        self.assertEqual(len(sig.receiver), 2)

    def test_empty_dbc(self):
        """Test parsing of minimal DBC"""
        dbc_content = """VERSION ""
BU_: ECU1
"""
        network = self.parser.parse_content(dbc_content)
        self.assertEqual(len(network.messages), 0)
        self.assertEqual(len(network.nodes), 1)
        self.assertEqual(network.nodes[0], "ECU1")

    def test_no_signals_message(self):
        """Test parsing of message with no signals"""
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 100 EmptyMsg: 8 ECU1
CM_ BO_ 100 "Empty message";
"""
        network = self.parser.parse_content(dbc_content)
        self.assertEqual(len(network.messages), 1)
        msg = network.messages[0]
        self.assertEqual(len(msg.signals), 0)


# ============================================================================
# CAN Matrix Parser Edge Cases
# ============================================================================

class TestCanMatrixParserEdgeCases(unittest.TestCase):
    """CAN Matrix parser edge cases"""

    def setUp(self):
        self.parser = CanMatrixParser()

    def test_parse_message_id_hex(self):
        """Test various message ID formats"""
        cases = [
            ('0x100', 256),
            ('0xFF', 255),
            ('100', 100),
            ('123x', 291),
            ('', 0),
            ('0xABC', 2748),
            ('  0x200  ', 512),
        ]
        for id_str, expected in cases:
            self.assertEqual(self.parser._parse_message_id(id_str), expected, f"Failed for {id_str}")

    def test_parse_int_handling(self):
        """Test _parse_int with various inputs"""
        cases = [
            ('100', 100),
            ('100ms', 100),
            ('', 0),
            ('0', 0),
            ('-5', -5),
            (42, 42),
        ]
        for value, expected in cases:
            self.assertEqual(self.parser._parse_int(value), expected, f"Failed for {value!r}")

    def test_parse_float_handling(self):
        """Test _parse_float with various inputs"""
        cases = [
            ('1.5', 1.5),
            ('', 0.0),
            (3, 3.0),
        ]
        for value, expected in cases:
            self.assertEqual(self.parser._parse_float(value), expected, f"Failed for {value!r}")

    def test_column_name_mapping_exact(self):
        """Test exact column name mapping"""
        columns = ['Message ID', 'Signal Name', 'Start Bit', 'Length']
        self.parser._build_column_map(columns)

        self.assertIn('message_id', self.parser._column_map)
        self.assertIn('signal_name', self.parser._column_map)
        self.assertIn('start_bit', self.parser._column_map)
        self.assertIn('bit_length', self.parser._column_map)

    def test_parse_motorola_signal(self):
        """Test Motorola byte order parsing"""
        csv_content = """Message ID,Message Name,DLC,Sender,Cycle Time,Signal Name,Start Bit,Length,Byte Order,Data Type,Factor,Offset,Minimum,Maximum,Unit,Receiver
0x100,MotorolaMsg,8,ECU1,100,Sig1,7,8,Motorola,unsigned,1,0,0,255,,ECU2
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
            f.write(csv_content)
            temp_path = f.name
        try:
            matrix = self.parser.parse_csv(temp_path)
            sig = matrix.messages[0].signals[0]
            self.assertIn('motorola', sig.byte_order.lower())
        finally:
            Path(temp_path).unlink()

    def test_empty_row_skipped(self):
        """Test that empty rows are skipped"""
        csv_content = """Message ID,Message Name,DLC,Sender,Cycle Time,Signal Name,Start Bit,Length
0x100,Msg1,8,ECU1,100,Sig1,0,8
,,,,
0x200,Msg2,8,ECU2,200,Sig2,0,8
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
            f.write(csv_content)
            temp_path = f.name
        try:
            matrix = self.parser.parse_csv(temp_path)
            self.assertEqual(len(matrix.messages), 2)
        finally:
            Path(temp_path).unlink()

    def test_to_com_config_with_matrix(self):
        """Test to_com_config conversion from parsed matrix"""
        csv_content = create_example_csv()
        with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
            f.write(csv_content)
            temp_path = f.name
        try:
            matrix = self.parser.parse_csv(temp_path)
            config = self.parser.to_com_config()
            self.assertIn('ipdus', config)
            self.assertIn('signals', config)
            self.assertGreater(len(config['ipdus']), 0)
        finally:
            Path(temp_path).unlink()

    def test_missing_csv_file(self):
        """Test missing CSV file raises error"""
        with self.assertRaises(FileNotFoundError):
            self.parser.parse_csv("/nonexistent/file.csv")


# ============================================================================
# Com Config Generator Edge Cases
# ============================================================================

class TestComConfigGeneratorEdgeCases(unittest.TestCase):
    """Com config generator edge cases"""

    def setUp(self):
        self.basic_config = {
            'ecu_name': 'ECU0',
            'ipdus': [],
            'signals': [],
            'signal_groups': []
        }

    def test_empty_config(self):
        """Test generation with empty config"""
        generator = ComConfigGenerator(self.basic_config)
        h_content = generator.generate_cfg_h()
        c_content = generator.generate_cfg_c()

        self.assertIn('COM_NUM_OF_IPDUS     0', h_content)
        self.assertIn('COM_NUM_OF_SIGNALS   0', h_content)
        self.assertIn('Com_Config', c_content)

    def test_multiple_ipdus_and_signals(self):
        """Test with multiple IPDUs and signals"""
        config = {
            'ecu_name': 'MultiECU',
            'ipdus': [
                {'name': 'IPDU_A', 'message_id': 0x100, 'dlc': 8,
                 'direction': 'SEND', 'cycle_time': 100, 'signals': ['SigA1', 'SigA2']},
                {'name': 'IPDU_B', 'message_id': 0x200, 'dlc': 4,
                 'direction': 'RECEIVE', 'cycle_time': 50, 'signals': ['SigB1']},
            ],
            'signals': [
                {'name': 'SigA1', 'ipdu': 'IPDU_A', 'start_bit': 0, 'bit_length': 16,
                 'byte_order': 'LITTLE_ENDIAN', 'data_type': 'UINT16', 'factor': 1.0,
                 'offset': 0.0, 'minimum': 0.0, 'maximum': 65535.0, 'init_value': 0},
                {'name': 'SigA2', 'ipdu': 'IPDU_A', 'start_bit': 16, 'bit_length': 8,
                 'byte_order': 'LITTLE_ENDIAN', 'data_type': 'UINT8', 'factor': 1.0,
                 'offset': 0.0, 'minimum': 0.0, 'maximum': 255.0, 'init_value': 0},
                {'name': 'SigB1', 'ipdu': 'IPDU_B', 'start_bit': 0, 'bit_length': 32,
                 'byte_order': 'BIG_ENDIAN', 'data_type': 'UINT32', 'factor': 0.1,
                 'offset': -100.0, 'minimum': -100.0, 'maximum': 3000.0, 'init_value': 0},
            ],
            'signal_groups': []
        }
        generator = ComConfigGenerator(config)
        h_content = generator.generate_cfg_h()
        c_content = generator.generate_cfg_c()

        self.assertIn('COM_NUM_OF_IPDUS     2', h_content)
        self.assertIn('COM_NUM_OF_SIGNALS   3', h_content)
        self.assertIn('IPDU_A', h_content)
        self.assertIn('IPDU_B', h_content)
        self.assertIn('SigA1', h_content)
        self.assertIn('SigB1', c_content)
        self.assertIn('COM_BIG_ENDIAN', c_content)
        self.assertIn('COM_LITTLE_ENDIAN', c_content)
        self.assertIn('RECEIVE', c_content)

    def test_all_data_types(self):
        """Test all data types are covered"""
        config = {
            'ecu_name': 'DataTypeTest',
            'ipdus': [
                {'name': 'IPDU_All', 'message_id': 0x300, 'dlc': 8,
                 'direction': 'SEND', 'cycle_time': 100, 'signals': ['Sig1', 'Sig2', 'Sig3', 'Sig4']},
            ],
            'signals': [
                {'name': 'Sig1', 'ipdu': 'IPDU_All', 'start_bit': 0, 'bit_length': 8,
                 'byte_order': 'LITTLE_ENDIAN', 'data_type': 'UINT8', 'factor': 1.0,
                 'offset': 0.0, 'minimum': 0.0, 'maximum': 255.0, 'init_value': 0},
                {'name': 'Sig2', 'ipdu': 'IPDU_All', 'start_bit': 8, 'bit_length': 16,
                 'byte_order': 'LITTLE_ENDIAN', 'data_type': 'SINT16', 'factor': 1.0,
                 'offset': 0.0, 'minimum': -32768.0, 'maximum': 32767.0, 'init_value': 0},
                {'name': 'Sig3', 'ipdu': 'IPDU_All', 'start_bit': 24, 'bit_length': 32,
                 'byte_order': 'LITTLE_ENDIAN', 'data_type': 'UINT32', 'factor': 0.001,
                 'offset': 0.0, 'minimum': 0.0, 'maximum': 4294967.0, 'init_value': 0},
                {'name': 'Sig4', 'ipdu': 'IPDU_All', 'start_bit': 56, 'bit_length': 64,
                 'byte_order': 'BIG_ENDIAN', 'data_type': 'UINT64', 'factor': 1.0,
                 'offset': 0.0, 'minimum': 0.0, 'maximum': 1.844e19, 'init_value': 0},
            ],
            'signal_groups': []
        }
        generator = ComConfigGenerator(config)
        h_content = generator.generate_cfg_h()
        c_content = generator.generate_cfg_c()

        self.assertIn('COM_UINT8', c_content)
        self.assertIn('COM_SINT16', c_content)
        self.assertIn('COM_UINT32', c_content)
        self.assertIn('COM_UINT64', c_content)

    def test_generate_summary_output(self):
        """Test generate_summary method"""
        config = {
            'ecu_name': 'TestECU',
            'ipdus': [
                {'name': 'IPDU_Test', 'message_id': 0x100, 'dlc': 8,
                 'direction': 'SEND', 'cycle_time': 100, 'signals': ['TestSig']},
            ],
            'signals': [
                {'name': 'TestSig', 'ipdu': 'IPDU_Test', 'start_bit': 0, 'bit_length': 16,
                 'byte_order': 'LITTLE_ENDIAN', 'data_type': 'UINT16', 'factor': 1.0,
                 'offset': 0.0, 'minimum': 0.0, 'maximum': 65535.0, 'init_value': 0},
            ],
            'signal_groups': []
        }
        generator = ComConfigGenerator(config)
        summary = generator.generate_summary()
        self.assertIn('TestECU', summary)
        self.assertIn('IPDU_Test', summary)
        self.assertIn('TestSig', summary)
        self.assertIn('Com模块配置摘要', summary)

    def test_generate_files_verification(self):
        """Test that generated files have correct content"""
        config = {
            'ecu_name': 'ECU0',
            'ipdus': [
                {'name': 'IPDU_Data', 'message_id': 0x123, 'dlc': 8,
                 'direction': 'SEND', 'cycle_time': 100, 'signals': ['DataSig']},
            ],
            'signals': [
                {'name': 'DataSig', 'ipdu': 'IPDU_Data', 'start_bit': 0, 'bit_length': 16,
                 'byte_order': 'LITTLE_ENDIAN', 'data_type': 'UINT16', 'factor': 1.0,
                 'offset': 0.0, 'minimum': 0.0, 'maximum': 100.0, 'init_value': 0},
            ],
            'signal_groups': []
        }
        with tempfile.TemporaryDirectory() as tmpdir:
            generator = ComConfigGenerator(config)
            cfg_h, cfg_c = generator.generate(tmpdir)
            self.assertTrue(Path(cfg_h).exists())
            self.assertTrue(Path(cfg_c).exists())

            with open(cfg_h) as f:
                content = f.read()
                self.assertIn('#ifndef COM_CFG_H', content)

    def test_prepare_config_defaults(self):
        """Test that _prepare_config sets correct defaults"""
        minimal = {'ipdus': [], 'signals': [], 'signal_groups': []}
        gen = ComConfigGenerator(minimal)
        config_data = gen.config
        # Defaults should be set
        self.assertTrue(config_data.get('dev_error_detect'))
        self.assertFalse(config_data.get('version_info_api'))
        self.assertTrue(config_data.get('enable_update_bit_check'))
        self.assertTrue(config_data.get('signal_change_check'))
        self.assertIn('timestamp', config_data)

    def test_unsigned_data_type_detection(self):
        """Test unsigned data type selection"""
        from com_config_generator import ComConfigGenerator
        # We test through DBC parser's to_com_config which calls the selection
        dbc_content = """VERSION ""
BU_: ECU1
BO_ 100 Types: 8 ECU1
 SG_ U8Sig  : 0|8@1+ (1,0) [0|255] "" ECU2
 SG_ U16Sig : 8|16@1+ (1,0) [0|65535] "" ECU2
 SG_ U32Sig : 24|32@1+ (1,0) [0|4294967295] "" ECU2
 SG_ S8Sig  : 56|8@1- (1,0) [-128|127] "" ECU2
 SG_ S32Sig : 0|32@1- (1,0) [-2147483648|2147483647] "" ECU2
"""
        parser = DbcParser()
        parser.parse_content(dbc_content)
        config = parser.to_com_config()
        signals = config['signals']
        types = {s['name']: s['data_type'] for s in signals}
        self.assertEqual(types.get('U8Sig'), 'UINT8')
        self.assertEqual(types.get('U16Sig'), 'UINT16')
        self.assertEqual(types.get('U32Sig'), 'UINT32')
        self.assertEqual(types.get('S8Sig'), 'SINT16')  # 8-bit signed
        self.assertEqual(types.get('S32Sig'), 'SINT32')


if __name__ == '__main__':
    unittest.main()
