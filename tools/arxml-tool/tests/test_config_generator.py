#!/usr/bin/env python3
"""
Tests for ARXML Configuration Generator
"""

import unittest
import json
import tempfile
import shutil
from pathlib import Path
import sys

# 添加src到路径
sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from config_generator import (
    ConfigGenerator,
    ModuleConfig,
    EcucContainer,
    EcucParameter
)


class TestConfigGenerator(unittest.TestCase):
    """测试配置生成器"""

    def setUp(self):
        """测试前准备"""
        self.temp_dir = Path(tempfile.mkdtemp())
        self.generator = ConfigGenerator(self.temp_dir)
        
        # 创建测试配置
        self.test_config = ModuleConfig(
            name="TestModule",
            module_def="TestModule",
            description="Test module configuration"
        )
        
        # 添加容器和参数
        container = EcucContainer(
            name="General",
            definition="TestModule/General",
            description="General settings"
        )
        
        container.parameters.extend([
            EcucParameter(
                name="EnableFeature",
                value="true",
                type="BOOLEAN",
                definition="TestModule/General/EnableFeature"
            ),
            EcucParameter(
                name="Timeout",
                value="100",
                type="INTEGER",
                definition="TestModule/General/Timeout"
            ),
            EcucParameter(
                name="Version",
                value="1.2.3",
                type="STRING",
                definition="TestModule/General/Version"
            )
        ])
        
        self.test_config.containers.append(container)

    def tearDown(self):
        """测试后清理"""
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_generate_cfg_h(self):
        """测试生成头文件"""
        files = self.generator.generate_module_config(self.test_config)
        
        # 检查头文件
        header_file = self.temp_dir / "TestModule_Cfg.h"
        self.assertIn(header_file, files)
        self.assertTrue(header_file.exists())
        
        content = header_file.read_text()
        
        # 检查关键内容
        self.assertIn("#ifndef TESTMODULE_CFG_H", content)
        self.assertIn("#define TESTMODULE_CFG_H", content)
        self.assertIn("TESTMODULE_GENERAL_ENABLEFEATURE", content)
        self.assertIn("TESTMODULE_GENERAL_TIMEOUT", content)
        self.assertIn("TESTMODULE_GENERAL_VERSION", content)

    def test_generate_cfg_c(self):
        """测试生成源文件"""
        files = self.generator.generate_module_config(self.test_config)
        
        source_file = self.temp_dir / "TestModule_Cfg.c"
        self.assertIn(source_file, files)
        self.assertTrue(source_file.exists())
        
        content = source_file.read_text()
        self.assertIn("#include \"TestModule_Cfg.h\"", content)

    def test_generate_lcfg_c(self):
        """测试生成链接时配置"""
        files = self.generator.generate_module_config(self.test_config)
        
        lcfg_file = self.temp_dir / "TestModule_Lcfg.c"
        self.assertIn(lcfg_file, files)
        self.assertTrue(lcfg_file.exists())
        
        content = lcfg_file.read_text()
        
        # 检查版本定义
        self.assertIn("TESTMODULE_LCFG_SW_MAJOR_VERSION", content)
        
        # 检查配置表
        self.assertIn("TestModule_General_ConfigTable", content)

    def test_generate_ecuc_arxml(self):
        """测试生成ECUC ARXML"""
        files = self.generator.generate_module_config(self.test_config)
        
        arxml_file = self.temp_dir / "TestModule_Config.arxml"
        self.assertIn(arxml_file, files)
        self.assertTrue(arxml_file.exists())
        
        content = arxml_file.read_text()
        
        # 检查ARXML结构
        self.assertIn('<?xml version="1.0"', content)
        self.assertIn("<AUTOSAR", content)
        self.assertIn("ECUC-MODULE-CONFIGURATION-VALUES", content)
        self.assertIn("TestModule", content)

    def test_generate_all_files(self):
        """测试生成所有文件"""
        files = self.generator.generate_module_config(self.test_config)
        
        # 检查所有4个文件都生成
        self.assertEqual(len(files), 4)
        
        expected_files = [
            "TestModule_Cfg.h",
            "TestModule_Cfg.c",
            "TestModule_Lcfg.c",
            "TestModule_Config.arxml"
        ]
        
        for filename in expected_files:
            self.assertTrue(
                (self.temp_dir / filename).exists(),
                f"Missing file: {filename}"
            )

    def test_boolean_conversion(self):
        """测试布尔值转换"""
        param_true = EcucParameter(
            name="TestBool",
            value="true",
            type="BOOLEAN",
            definition="Test"
        )
        
        param_false = EcucParameter(
            name="TestBool",
            value="false",
            type="BOOLEAN",
            definition="Test"
        )
        
        true_val = self.generator._convert_to_c_macro_value(param_true)
        false_val = self.generator._convert_to_c_macro_value(param_false)
        
        self.assertEqual(true_val, "TRUE")
        self.assertEqual(false_val, "FALSE")

    def test_string_conversion(self):
        """测试字符串值转换"""
        param = EcucParameter(
            name="TestString",
            value="HelloWorld",
            type="STRING",
            definition="Test"
        )
        
        value = self.generator._convert_to_c_macro_value(param)
        self.assertEqual(value, '"HelloWorld"')

    def test_batch_generation(self):
        """测试批量生成"""
        config2 = ModuleConfig(
            name="TestModule2",
            module_def="TestModule2",
            description="Second test module"
        )
        
        results = self.generator.generate_batch([self.test_config, config2])
        
        self.assertIn("TestModule", results)
        self.assertIn("TestModule2", results)
        self.assertEqual(len(results["TestModule"]), 4)
        self.assertEqual(len(results["TestModule2"]), 4)


class TestConfigGeneratorFromJSON(unittest.TestCase):
    """测试从JSON生成配置"""

    def setUp(self):
        self.temp_dir = Path(tempfile.mkdtemp())
        self.generator = ConfigGenerator(self.temp_dir)
        
        # 创建测试JSON文件
        self.json_config = {
            "name": "Can",
            "module_def": "Can",
            "description": "CAN configuration",
            "containers": [
                {
                    "name": "CanGeneral",
                    "definition": "Can/CanGeneral",
                    "parameters": [
                        {
                            "name": "DevErrorDetect",
                            "value": "true",
                            "type": "BOOLEAN",
                            "definition": "Can/CanGeneral/DevErrorDetect"
                        }
                    ]
                }
            ]
        }
        
        self.json_file = self.temp_dir / "test_config.json"
        with open(self.json_file, 'w') as f:
            json.dump(self.json_config, f)

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_generate_from_json(self):
        """测试从JSON生成"""
        files = self.generator.generate_from_json(self.json_file)
        
        self.assertEqual(len(files), 4)
        
        # 检查头文件内容
        header_file = self.temp_dir / "Can_Cfg.h"
        content = header_file.read_text()
        self.assertIn("CAN_CANGENERAL_DEVERRORDETECT", content)
        self.assertIn("TRUE", content)


if __name__ == '__main__':
    unittest.main(verbosity=2)
