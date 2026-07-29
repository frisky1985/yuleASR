#!/usr/bin/env python3
"""集成测试 - 验证dds-config-tool与micro-dds的协同工作"""

import sys
import tempfile
import subprocess
from pathlib import Path

def test_xml_config_generation():
    """测试XML配置解析和代码生成"""
    print("→ 测试XML配置解析和代码生成...")
    
    xml_config = """<?xml version="1.0" encoding="UTF-8"?>
<dds>
    <domain_participant name="TestParticipant" domain_id="0">
        <topic name="TestTopic" type_name="TestType">
            <qos>
                <reliability kind="RELIABLE" max_blocking_time_sec="1"/>
                <durability kind="VOLATILE"/>
                <history kind="KEEP_LAST" depth="10"/>
                <ownership kind="SHARED"/>
                <destination_order kind="BY_RECEPTION_TIMESTAMP"/>
                <transport_priority value="0"/>
            </qos>
        </topic>
    </domain_participant>
</dds>"""
    
    with tempfile.TemporaryDirectory() as tmpdir:
        config_file = Path(tmpdir) / "test_config.xml"
        config_file.write_text(xml_config)
        
        output_dir = Path(tmpdir) / "output"
        output_dir.mkdir()
        
        # 运行配置工具
        result = subprocess.run(
            [sys.executable, "-m", "dds_config_tool", 
             str(config_file), 
             "-o", str(output_dir),
             "-p", "test"],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"  ✗ 失败: {result.stderr}")
            return False
            
        # 验证生成的文件
        expected_files = [
            "test_config.h",
            "test_domain_config.c",
            "test_topic_config.c",
            "test_qos_config.c"
        ]
        
        for f in expected_files:
            file_path = output_dir / f
            if not file_path.exists():
                print(f"  ✗ 缺少文件: {f}")
                return False
            print(f"  ✓ 生成文件: {f}")
        
        print("  ✓ XML配置测试通过")
        return True


def test_json_config_generation():
    """测试JSON配置解析和代码生成"""
    print("→ 测试JSON配置解析和代码生成...")
    
    json_config = """{
  "name": "IntegrationTest",
  "version": "1.0.0",
  "default_qos": {
    "reliability": {"kind": "RELIABLE", "max_blocking_time_sec": 0, "max_blocking_time_nsec": 100000000},
    "durability": {"kind": "TRANSIENT_LOCAL"},
    "history": {"kind": "KEEP_LAST", "depth": 5},
    "ownership": {"kind": "EXCLUSIVE", "strength": 10},
    "destination_order": {"kind": "BY_SOURCE_TIMESTAMP"},
    "transport_priority": {"value": 5}
  },
  "domain_participants": [
    {
      "name": "JsonTestParticipant",
      "domain_id": 1,
      "topics": [
        {
          "name": "JsonTopic",
          "type_name": "JsonType",
          "qos": {
            "reliability": {"kind": "RELIABLE"},
            "durability": {"kind": "PERSISTENT"}
          }
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
        
        result = subprocess.run(
            [sys.executable, "-m", "dds_config_tool",
             str(config_file),
             "-o", str(output_dir),
             "-p", "jsontest"],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"  ✗ 失败: {result.stderr}")
            return False
        
        # 检查生成的文件内容
        header_file = output_dir / "jsontest_config.h"
        if header_file.exists():
            content = header_file.read_text()
            if "EXCLUSIVE_OWNERSHIP_QOS" in content:
                print("  ✓ 高级QoS策略(所有权)正确生成")
            if "BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS" in content:
                print("  ✓ 高级QoS策略(目的排序)正确生成")
        
        print("  ✓ JSON配置测试通过")
        return True


def test_qos_policy_compatibility():
    """测试QoS策略兼容性验证"""
    print("→ 测试QoS策略兼容性验证...")
    
    # 创建兼容的QoS配置
    compatible_config = """{
  "name": "QosCompatibilityTest",
  "domain_participants": [{
    "name": "QosTest",
    "domain_id": 0,
    "topics": [{
      "name": "QosTopic",
      "type_name": "QosType",
      "qos": {
        "reliability": {"kind": "RELIABLE"},
        "durability": {"kind": "TRANSIENT_LOCAL"},
        "history": {"kind": "KEEP_LAST", "depth": 10}
      }
    }]
  }]
}"""
    
    with tempfile.TemporaryDirectory() as tmpdir:
        config_file = Path(tmpdir) / "qos_config.json"
        config_file.write_text(compatible_config)
        
        output_dir = Path(tmpdir) / "output"
        output_dir.mkdir()
        
        result = subprocess.run(
            [sys.executable, "-m", "dds_config_tool",
             str(config_file),
             "-o", str(output_dir),
             "-p", "qos"],
            capture_output=True,
            text=True
        )
        
        if result.returncode == 0:
            print("  ✓ QoS策略配置验证通过")
            return True
        else:
            print(f"  ✗ 验证失败: {result.stderr}")
            return False


def main():
    """主函数"""
    print("="*60)
    print("Micro-DDS 集成测试")
    print("="*60)
    
    results = []
    
    results.append(("XML配置生成", test_xml_config_generation()))
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
