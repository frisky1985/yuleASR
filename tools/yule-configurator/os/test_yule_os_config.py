#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
YuleTech OS Configuration Tool 测试
"""

import os
import sys
import tempfile
import shutil
from pathlib import Path

# 添加源代码目录到路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from yule_os_config import (
    ConfigParser, ConfigValidator, CodeGenerator, OSConfigTool,
    TaskConfig, AlarmConfig, ResourceConfig, EventConfig, ScheduleTableConfig,
    ScheduleType, AlarmActionType, ExpiryPoint
)


def test_task_config():
    """测试任务配置"""
    print("Testing TaskConfig...")
    
    task = TaskConfig(
        name="TestTask",
        priority=5,
        activation=2,
        autostart=True,
        schedule="FULL"
    )
    
    assert task.name == "TestTask"
    assert task.priority == 5
    assert task.activation == 2
    assert task.autostart == True
    assert task.schedule == ScheduleType.FULL
    print("  ✓ TaskConfig OK")


def test_alarm_config():
    """测试报警配置"""
    print("Testing AlarmConfig...")
    
    alarm = AlarmConfig(
        name="TestAlarm",
        counter="SystemCounter",
        action="ACTIVATETASK",
        task="TestTask",
        autostart=True,
        period=10
    )
    
    assert alarm.name == "TestAlarm"
    assert alarm.action == AlarmActionType.ACTIVATETASK
    assert alarm.task == "TestTask"
    print("  ✓ AlarmConfig OK")


def test_resource_config():
    """测试资源配置"""
    print("Testing ResourceConfig...")
    
    resource = ResourceConfig(
        name="TestResource",
        priority_ceiling=8
    )
    
    assert resource.name == "TestResource"
    assert resource.priority_ceiling == 8
    print("  ✓ ResourceConfig OK")


def test_event_config():
    """测试事件配置"""
    print("Testing EventConfig...")
    
    event = EventConfig(
        name="TestEvent",
        mask=0x00000001
    )
    
    assert event.name == "TestEvent"
    assert event.mask == 0x00000001
    print("  ✓ EventConfig OK")


def test_parser():
    """测试配置解析器"""
    print("Testing ConfigParser...")
    
    yaml_content = """
os:
  name: "TestOS"

tasks:
  - name: "Task1"
    priority: 10
    activation: 1
    autostart: true
    schedule: "FULL"
  - name: "Task2"
    priority: 5
    activation: 1
    autostart: false
    schedule: "NON"

alarms:
  - name: "Alarm1"
    counter: "Counter1"
    action: "ACTIVATETASK"
    task: "Task1"

resources:
  - name: "Res1"
    priority_ceiling: 8

events:
  - name: "Event1"
    mask: 1
"""
    
    # 创建临时 YAML 文件
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write(yaml_content)
        temp_path = f.name
    
    try:
        config = ConfigParser.load_config(temp_path)
        
        assert len(config.tasks) == 2
        assert config.tasks[0].name == "Task1"
        assert config.tasks[0].priority == 10
        
        assert len(config.alarms) == 1
        assert config.alarms[0].name == "Alarm1"
        
        assert len(config.resources) == 1
        assert len(config.events) == 1
        
        print("  ✓ ConfigParser OK")
    finally:
        os.unlink(temp_path)


def test_validator():
    """测试配置验证器"""
    print("Testing ConfigValidator...")
    
    # 创建有效配置
    config = OSConfigTool()
    
    # 测试通过的配置
    valid_yaml = """
tasks:
  - name: "Task1"
    priority: 10
    activation: 1
    autostart: true
    schedule: "FULL"
  
  - name: "Task2"
    priority: 5
    activation: 1
    autostart: false
    schedule: "NON"

alarms:
  - name: "Alarm1"
    counter: "Counter1"
    action: "ACTIVATETASK"
    task: "Task1"

resources:
  - name: "Res1"
    priority_ceiling: 8
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write(valid_yaml)
        temp_path = f.name
    
    try:
        os_config = ConfigParser.load_config(temp_path)
        validator = ConfigValidator()
        is_valid, errors, warnings = validator.validate(os_config)
        
        assert is_valid == True
        assert len(errors) == 0
        print("  ✓ Valid config passed")
    finally:
        os.unlink(temp_path)
    
    # 测试失败的配置 (重复任务名)
    invalid_yaml = """
tasks:
  - name: "Task1"
    priority: 10
    activation: 1
    autostart: true
    schedule: "FULL"
  
  - name: "Task1"
    priority: 5
    activation: 1
    autostart: false
    schedule: "NON"
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write(invalid_yaml)
        temp_path = f.name
    
    try:
        os_config = ConfigParser.load_config(temp_path)
        validator = ConfigValidator()
        is_valid, errors, warnings = validator.validate(os_config)
        
        assert is_valid == False
        assert any("重复" in e for e in errors)
        print("  ✓ Invalid config detected")
    finally:
        os.unlink(temp_path)


def test_code_generator():
    """测试代码生成器"""
    print("Testing CodeGenerator...")
    
    config = OSConfigTool()
    
    yaml_content = """
tasks:
  - name: "Init"
    priority: 10
    activation: 1
    autostart: true
    schedule: "FULL"
  
  - name: "Cyclic"
    priority: 5
    activation: 1
    autostart: false
    schedule: "FULL"

alarms:
  - name: "Alarm1"
    counter: "Counter1"
    action: "ACTIVATETASK"
    task: "Cyclic"

resources:
  - name: "Res1"
    priority_ceiling: 8

events:
  - name: "Event1"
    mask: 1
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write(yaml_content)
        temp_path = f.name
    
    output_dir = tempfile.mkdtemp()
    
    try:
        os_config = ConfigParser.load_config(temp_path)
        generator = CodeGenerator()
        
        header_path = os.path.join(output_dir, "Os_Cfg.h")
        source_path = os.path.join(output_dir, "Os_Cfg.c")
        
        generator.generate_header(os_config, header_path)
        generator.generate_source(os_config, source_path)
        
        # 验证文件生成
        assert os.path.exists(header_path)
        assert os.path.exists(source_path)
        
        # 验证内容
        with open(header_path, 'r') as f:
            header_content = f.read()
            assert "OS_CFG_H" in header_content
            assert "TASK_INIT" in header_content
            assert "TASK_CYCLIC" in header_content
        
        with open(source_path, 'r') as f:
            source_content = f.read()
            assert "AUTOGENERATED FILE" in source_content
            assert "Os_TaskConfig" in source_content
        
        print("  ✓ Code generation OK")
    finally:
        os.unlink(temp_path)
        shutil.rmtree(output_dir)


def test_full_workflow():
    """测试完整工作流"""
    print("Testing full workflow...")
    
    tool = OSConfigTool()
    
    yaml_content = """
tasks:
  - name: "Init"
    priority: 10
    activation: 1
    autostart: true
    schedule: "FULL"

alarms:
  - name: "Alarm1"
    counter: "Counter1"
    action: "ACTIVATETASK"
    task: "Init"

resources:
  - name: "Res1"
    priority_ceiling: 8
"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write(yaml_content)
        config_path = f.name
    
    output_dir = tempfile.mkdtemp()
    
    try:
        success = tool.process(config_path, output_dir)
        assert success == True
        
        # 验证生成的文件
        assert os.path.exists(os.path.join(output_dir, "Os_Cfg.h"))
        assert os.path.exists(os.path.join(output_dir, "Os_Cfg.c"))
        
        print("  ✓ Full workflow OK")
    finally:
        os.unlink(config_path)
        shutil.rmtree(output_dir)


def test_json_config():
    """测试 JSON 配置文件"""
    print("Testing JSON config...")
    
    json_content = """{
  "os": {"name": "TestOS"},
  "tasks": [
    {"name": "Task1", "priority": 10, "activation": 1, "autostart": true, "schedule": "FULL"}
  ],
  "alarms": [],
  "resources": [],
  "events": [],
  "schedule_tables": []
}"""
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
        f.write(json_content)
        temp_path = f.name
    
    try:
        config = ConfigParser.load_config(temp_path)
        assert len(config.tasks) == 1
        assert config.tasks[0].name == "Task1"
        print("  ✓ JSON config OK")
    finally:
        os.unlink(temp_path)


def run_all_tests():
    """运行所有测试"""
    print("=" * 60)
    print("YuleTech OS Configuration Tool Test Suite")
    print("=" * 60)
    
    tests = [
        test_task_config,
        test_alarm_config,
        test_resource_config,
        test_event_config,
        test_parser,
        test_validator,
        test_code_generator,
        test_full_workflow,
        test_json_config,
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            test()
            passed += 1
        except Exception as e:
            print(f"  ✗ {test.__name__} FAILED: {e}")
            failed += 1
    
    print("=" * 60)
    print(f"Results: {passed} passed, {failed} failed")
    print("=" * 60)
    
    return failed == 0


if __name__ == '__main__':
    success = run_all_tests()
    sys.exit(0 if success else 1)
