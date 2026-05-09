#!/usr/bin/env python3
"""
CLI测试模块

测试内容:
1. 命令行参数解析
2. 入口点调用
3. 退出码验证
"""

import sys
import os
import subprocess
import tempfile
import shutil
from pathlib import Path

# 添加src目录到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

import pytest


class TestCLI:
    """测试命令行接口"""
    
    @pytest.fixture
    def temp_dir(self):
        """创建临时目录"""
        temp = tempfile.mkdtemp()
        yield temp
        shutil.rmtree(temp)
    
    @pytest.fixture
    def arxml_tool_dir(self):
        """获取ARXML工具目录"""
        return Path(__file__).parent.parent
    
    def run_script(self, script_name, args, cwd=None):
        """运行脚本并返回结果"""
        script_path = Path(__file__).parent.parent / "src" / script_name
        cmd = [sys.executable, str(script_path)] + args
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            cwd=cwd
        )
        return result
    
    def test_config_generator_help(self, arxml_tool_dir):
        """测试配置生成器帮助"""
        result = self.run_script("config_generator.py", ["--help"])
        
        assert result.returncode == 0
        assert "ARXML Configuration Generator" in result.stdout
        assert "-i" in result.stdout or "--input" in result.stdout
        assert "-o" in result.stdout or "--output" in result.stdout
    
    def test_integrity_analyzer_help(self, arxml_tool_dir):
        """测试完整性分析器帮助"""
        result = self.run_script("integrity_analyzer.py", ["--help"])
        
        assert result.returncode == 0
        assert "ARXML完整性分析器" in result.stdout or "usage:" in result.stdout
        assert "input" in result.stdout
        assert "--strict" in result.stdout
    
    def test_config_generator_json_input(self, temp_dir, arxml_tool_dir):
        """测试配置生成器JSON输入"""
        # 创建测试JSON配置
        json_config = {
            "name": "TestModule",
            "module_def": "/AUTOSAR/TestModule",
            "containers": [
                {
                    "name": "General",
                    "definition": "/AUTOSAR/TestModule/General",
                    "parameters": [
                        {
                            "name": "Enable",
                            "value": "true",
                            "type": "BOOLEAN"
                        }
                    ]
                }
            ]
        }
        
        import json
        json_file = Path(temp_dir) / "test_config.json"
        with open(json_file, 'w') as f:
            json.dump(json_config, f)
        
        output_dir = Path(temp_dir) / "output"
        
        # 运行生成器
        result = self.run_script(
            "config_generator.py",
            ["-i", str(json_file), "-o", str(output_dir), "-f", "json", "-v"]
        )
        
        # 验证结果
        assert result.returncode == 0
        
        # 检查生成的文件
        assert (output_dir / "TestModule_Cfg.h").exists()
        assert (output_dir / "TestModule_Cfg.c").exists()
        assert (output_dir / "TestModule_Lcfg.c").exists()
        assert (output_dir / "TestModule_Config.arxml").exists()
    
    def test_integrity_analyzer_basic(self, temp_dir, arxml_tool_dir):
        """测试完整性分析器基本功能"""
        # 创建测试ARXML文件
        arxml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
          <AR-PACKAGES>
            <AR-PACKAGE UUID="pkg-001">
              <SHORT-NAME>Test</SHORT-NAME>
              <ELEMENTS>
                <APPLICATION-SW-COMPONENT-TYPE UUID="swc-001">
                  <SHORT-NAME>TestSwc</SHORT-NAME>
                </APPLICATION-SW-COMPONENT-TYPE>
              </ELEMENTS>
            </AR-PACKAGE>
          </AR-PACKAGES>
        </AUTOSAR>'''
        
        arxml_file = Path(temp_dir) / "test.arxml"
        arxml_file.write_text(arxml_content)
        
        output_file = Path(temp_dir) / "report.json"
        
        # 运行分析器
        result = self.run_script(
            "integrity_analyzer.py",
            [str(arxml_file), "-o", str(output_file), "--format", "json"]
        )
        
        # 验证结果
        assert result.returncode in [0, 1]  # 0=成功无错误, 1=有错误
        assert output_file.exists()
        
        # 验证JSON报告格式
        import json
        with open(output_file) as f:
            report = json.load(f)
        
        assert "file_path" in report
        assert "total_issues" in report
        assert "issues" in report
    
    def test_integrity_analyzer_strict_mode(self, temp_dir, arxml_tool_dir):
        """测试严格模式"""
        arxml_content = '''<?xml version="1.0" encoding="UTF-8"?>
        <AUTOSAR xmlns="http://autosar.org/schema/r4.0">
          <AR-PACKAGES>
            <AR-PACKAGE UUID="pkg-001">
              <SHORT-NAME>Test</SHORT-NAME>
              <ELEMENTS>
                <APPLICATION-SW-COMPONENT-TYPE UUID="swc-001">
                  <SHORT-NAME>123Invalid</SHORT-NAME>
                </APPLICATION-SW-COMPONENT-TYPE>
              </ELEMENTS>
            </AR-PACKAGE>
          </AR-PACKAGES>
        </AUTOSAR>'''
        
        arxml_file = Path(temp_dir) / "test.arxml"
        arxml_file.write_text(arxml_content)
        
        # 测试宽松模式
        result_loose = self.run_script(
            "integrity_analyzer.py",
            [str(arxml_file), "--format", "json"]
        )
        
        # 测试严格模式
        result_strict = self.run_script(
            "integrity_analyzer.py",
            [str(arxml_file), "--strict", "--format", "json"]
        )
        
        # 严格模式可能返回更多警告
        assert result_loose.returncode in [0, 1]
        assert result_strict.returncode in [0, 1]
    
    def test_invalid_input_file(self, arxml_tool_dir):
        """测试无效输入文件"""
        result = self.run_script(
            "integrity_analyzer.py",
            ["/nonexistent/file.arxml"]
        )
        
        # 应该返回错误退出码
        assert result.returncode != 0 or "错误" in result.stdout or "Error" in result.stdout


class TestCLIIntegration:
    """CLI集成测试"""
    
    @pytest.fixture
    def examples_dir(self):
        """获取示例目录"""
        return Path(__file__).parent.parent / "examples"
    
    def test_example_files_analysis(self, examples_dir):
        """测试示例文件分析"""
        if not examples_dir.exists():
            pytest.skip("示例目录不存在")
        
        arxml_files = list(examples_dir.glob("*.arxml"))
        if not arxml_files:
            pytest.skip("没有找到ARXML文件")
        
        for arxml_file in arxml_files:
            result = subprocess.run(
                [sys.executable, "-m", "src.integrity_analyzer", str(arxml_file)],
                capture_output=True,
                text=True,
                cwd=examples_dir.parent
            )
            
            print(f"\n分析: {arxml_file.name}")
            print(f"  返回码: {result.returncode}")
            
            if "invalid" in arxml_file.name.lower():
                # 无效文件应该有错误
                print(f"  预期: 有错误")
            else:
                print(f"  预期: 可能有警告")
    
    def test_example_json_generation(self, examples_dir, tmp_path):
        """测试示例JSON配置生成"""
        if not examples_dir.exists():
            pytest.skip("示例目录不存在")
        
        json_files = list(examples_dir.glob("*.json"))
        if not json_files:
            pytest.skip("没有找到JSON文件")
        
        for json_file in json_files:
            output_dir = tmp_path / f"output_{json_file.stem}"
            
            result = subprocess.run(
                [
                    sys.executable, "-m", "src.config_generator",
                    "-i", str(json_file),
                    "-o", str(output_dir),
                    "-f", "json"
                ],
                capture_output=True,
                text=True,
                cwd=examples_dir.parent
            )
            
            print(f"\n生成: {json_file.name}")
            print(f"  返回码: {result.returncode}")
            print(f"  输出: {result.stdout[:200] if result.stdout else 'None'}")
            
            assert result.returncode == 0, f"生成失败: {result.stderr}"
            assert output_dir.exists()
            
            # 检查生成的文件
            generated_files = list(output_dir.glob("*"))
            print(f"  生成文件: {[f.name for f in generated_files]}")
            assert len(generated_files) >= 4, "应该生成至少4个文件"


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
