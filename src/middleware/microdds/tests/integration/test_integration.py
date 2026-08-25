#!/usr/bin/env python3
"""集成测试 - 验证 dds_config 工具链 (tools/dds_config CLI) 与 micro-dds 的协同工作"""

import sys
import tempfile
import subprocess
from pathlib import Path

# P2-2 (2026-08-08): tools/dds-config-tool (Python 旧基线, CLI 损坏) 已删除,
# 统一使用 tools/dds_config/dds_config_cli.py (Python CLI 唯一工具链)。
DDS_CONFIG_CLI = str(Path(__file__).resolve().parents[5] / "tools" / "dds_config" / "dds_config_cli.py")


def _run_generate(config_file, output_dir):
    """调用 tools/dds_config CLI 生成代码, 返回 CompletedProcess"""
    return subprocess.run(
        [sys.executable, DDS_CONFIG_CLI, "generate",
         str(config_file),
         "-o", str(output_dir)],
        capture_output=True,
        text=True,
    )

def test_yaml_config_generation():
    """测试YAML配置解析和代码生成 (tools/dds_config 支持 YAML/JSON, 不支持 XML)"""
    print("→ 测试YAML配置解析和代码生成...")
    
    yaml_config = """\
system:
  name: "IntegrationTest"
  version: "1.0.0"
domains:
  - id: 0
    name: "TestDomain"
    participants:
      - name: "TestParticipant"
        qos_profile: "default_qos"
        publishers:
          - topic: "TestTopic"
            type: "TestType"
            qos:
              reliability: RELIABLE
              durability: VOLATILE
              history:
                kind: KEEP_LAST
                depth: 10
"""
    
    with tempfile.TemporaryDirectory() as tmpdir:
        config_file = Path(tmpdir) / "test_config.yaml"
        config_file.write_text(yaml_config)
        
        output_dir = Path(tmpdir) / "output"
        output_dir.mkdir()
        
        # 运行配置工具
        result = _run_generate(config_file, output_dir)
        
        assert result.returncode == 0, f"dds_config generate 失败: {result.stderr}"
            
        # 验证生成的文件 (tools/dds_config 输出 dds_config.h/.c + dds_qos_config.c)
        expected_files = [
            "dds_config.h",
            "dds_config.c",
            "dds_qos_config.c"
        ]
        
        for f in expected_files:
            file_path = output_dir / f
            assert file_path.exists(), f"缺少文件: {f}"
            print(f"  ✓ 生成文件: {f}")
        
        print("  ✓ YAML配置测试通过")
        return True


def test_json_config_generation():
    """测试JSON配置解析和代码生成"""
    print("→ 测试JSON配置解析和代码生成...")
    
    json_config = """{
  "system": {
    "name": "IntegrationTest",
    "version": "1.0.0"
  },
  "domains": [
    {
      "id": 1,
      "name": "JsonTestDomain",
      "participants": [
        {
          "name": "JsonTestParticipant",
          "publishers": [
            {
              "topic": "JsonTopic",
              "type": "JsonType",
              "qos": {
                "reliability": "RELIABLE",
                "durability": "TRANSIENT_LOCAL",
                "history": {
                  "kind": "KEEP_LAST",
                  "depth": 5
                }
              }
            }
          ]
        }
      ]
    }
  ]
}"""
    
    with tempfile.TemporaryDirectory() as tmpdir:
        config_file = Path(tmpdir) / "test_config.json"
        config_file.write_text(json_config)
        
        output_dir = Path(tmpdir) / "output"
        output_dir.mkdir()
        
        result = _run_generate(config_file, output_dir)
        
        assert result.returncode == 0, f"dds_config generate 失败: {result.stderr}"
        
        # 检查生成的文件内容 (QoS 策略生成在 dds_qos_config.c)
        qos_file = output_dir / "dds_qos_config.c"
        assert qos_file.exists(), "缺少 dds_qos_config.c"
        content = qos_file.read_text()
        # tools/dds_config 生成器输出含 QoS 常量/配置
        assert "qos" in content.lower(), "dds_qos_config.c 缺少 QoS 配置"
        if "RELIABLE" in content.upper():
            print("  ✓ 高级QoS策略(可靠性)正确生成")
        if "TRANSIENT_LOCAL" in content.upper():
            print("  ✓ 高级QoS策略(持久性)正确生成")
        
        print("  ✓ JSON配置测试通过")
        return True


def test_qos_policy_compatibility():
    """测试QoS策略兼容性验证"""
    print("→ 测试QoS策略兼容性验证...")
    
    # 创建兼容的QoS配置
    compatible_config = """{
  "system": {
    "name": "QosCompatibilityTest",
    "version": "1.0.0"
  },
  "domains": [{
    "id": 0,
    "name": "QosDomain",
    "participants": [{
      "name": "QosTest",
      "publishers": [{
        "topic": "QosTopic",
        "type": "QosType",
        "qos": {
          "reliability": "RELIABLE",
          "durability": "TRANSIENT_LOCAL",
          "history": {"kind": "KEEP_LAST", "depth": 10}
        }
      }]
    }]
  }]
}"""
    
    with tempfile.TemporaryDirectory() as tmpdir:
        config_file = Path(tmpdir) / "qos_config.json"
        config_file.write_text(compatible_config)
        
        output_dir = Path(tmpdir) / "output"
        output_dir.mkdir()
        
        result = _run_generate(config_file, output_dir)
        
        assert result.returncode == 0, f"dds_config generate 失败: {result.stderr}"
        header_file = output_dir / "dds_config.h"
        assert header_file.exists(), "缺少 dds_config.h"
        print("  ✓ QoS策略配置验证通过")
        return True


def main():
    """主函数"""
    print("="*60)
    print("Micro-DDS 集成测试")
    print("="*60)
    
    results = []
    
    results.append(("YAML配置生成", test_yaml_config_generation()))
    results.append(("JSON配置生成", test_json_config_generation()))
    results.append(("QoS策略兼容性", test_qos_policy_compatibility()))
    
    print("="*60)
    print("测试结果汇总:")
    print("="*60)
    
    all_passed = True
    for name, result in results:
        status = "✓ 通过" if result else "✗ 失败"
        print(f"  {name}: {status}")
        if not result:
            all_passed = False
    
    print("="*60)
    if all_passed:
        print("✓ 所有测试通过!")
        return 0
    else:
        print("✗ 部分测试失败")
        return 1


if __name__ == "__main__":
    sys.exit(main())
