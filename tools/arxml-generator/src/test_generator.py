#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML ECUC Generator 测试脚本

验证EcucConfigModel和ArxmlEcucGenerator的功能
"""

import sys
import os

# 添加src目录到路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from ecuc_config_model import (
    EcucModuleConfigurationValues,
    EcucContainerValue,
    EcucDefinitionRef,
    EcucBooleanParamValue,
    EcucIntegerParamValue,
    EcucStringParamValue,
    EcucEnumParamValue,
    EcucReferenceValue,
    create_module_config,
    create_container,
    create_boolean_param,
    create_integer_param,
    create_string_param,
    create_enum_param,
    create_reference_value,
    create_float_param,
    create_function_name_param,
)

from arxml_ecuc_generator import ArxmlEcucGenerator


def test_config_model():
    """测试配置模型"""
    print("\n[Test 1] 测试EcucConfigModel...")
    
    # 创建模块
    mcal_module = create_module_config(
        short_name="Mcu",
        module_def_path="/AUTOSAR/EcucDefs/Mcu",
        package_name="MCAL"
    )
    
    assert mcal_module.short_name == "Mcu"
    assert mcal_module.package_name == "MCAL"
    assert mcal_module.implementation_config_variant == "VARIANT-PRE-COMPILE"
    print("  ✓ 模块创建正确")
    
    # 创建容器
    mcu_general = create_container(
        def_path="/AUTOSAR/EcucDefs/Mcu/McuGeneralConfiguration",
        short_name="McuGeneral"
    )
    
    # 添加参数
    mcu_general.add_parameter(create_boolean_param(
        def_path="/AUTOSAR/EcucDefs/Mcu/McuGeneralConfiguration/McuDevErrorDetect",
        value=True
    ))
    mcu_general.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Mcu/McuGeneralConfiguration/McuClockSetting",
        value=3
    ))
    mcu_general.add_parameter(create_enum_param(
        def_path="/AUTOSAR/EcucDefs/Mcu/McuGeneralConfiguration/McuClockSrcFailureNotification",
        value="ENABLED"
    ))
    
    mcal_module.add_container(mcu_general)
    
    assert len(mcal_module.containers) == 1
    assert len(mcal_module.containers[0].parameter_values) == 3
    print("  ✓ 容器和参数添加正确")
    
    # 测试嵌套容器
    mcu_clock = create_container(
        def_path="/AUTOSAR/EcucDefs/Mcu/McuClockSettingConfig",
        short_name="McuClockSettingConfig_0"
    )
    mcu_clock.add_parameter(create_float_param(
        def_path="/AUTOSAR/EcucDefs/Mcu/McuClockSettingConfig/McuClockReferencePointFrequency",
        value=8000000.0
    ))
    
    # 子容器
    pll_config = create_container(
        def_path="/AUTOSAR/EcucDefs/Mcu/McuClockSettingConfig/McuPllConfig",
        short_name="McuPllConfig"
    )
    pll_config.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Mcu/McuClockSettingConfig/McuPllConfig/McuPllMultiplier",
        value=8
    ))
    
    mcu_clock.add_sub_container(pll_config)
    mcal_module.add_container(mcu_clock)
    
    assert len(mcal_module.containers) == 2
    assert len(mcal_module.containers[1].sub_containers) == 1
    print("  ✓ 嵌套容器正确")
    
    return mcal_module


def test_arxml_generator():
    """测试ARXML生成器"""
    print("\n[Test 2] 测试ArxmlEcucGenerator...")
    
    # 创建生成器
    generator = ArxmlEcucGenerator(
        company="YuleTech",
        author="Test Suite"
    )
    
    assert generator.AUTOSAR_NS == "http://autosar.org/schema/r4.0"
    print("  ✓ 命名空间配置正确")
    
    # 创建模块配置
    port_module = create_module_config(
        short_name="Port",
        module_def_path="/AUTOSAR/EcucDefs/Port",
        package_name="MCAL"
    )
    
    # 添加配置容器
    port_config = create_container(
        def_path="/AUTOSAR/EcucDefs/Port/PortConfigSet",
        short_name="PortConfigSet"
    )
    
    # 添加引用
    port_config.add_reference(create_reference_value(
        def_path="/AUTOSAR/EcucDefs/Port/PortConfigSet/PortGeneralRef",
        value_ref="/MCAL/Port/PortGeneral"
    ))
    
    port_module.add_container(port_config)
    generator.register_module(port_module)
    
    assert len(generator.generated_modules) == 1
    print("  ✓ 模块注册正确")
    
    # 生成XML
    root = generator.generate()
    
    assert root is not None
    assert root.tag.endswith("AUTOSAR")
    print("  ✓ XML生成成功")
    
    # 输出字符串
    xml_str = generator.to_string(pretty=True)
    
    assert "AUTOSAR" in xml_str
    assert "ECUC-MODULE-CONFIGURATION-VALUES" in xml_str
    assert "DEFINITION-REF" in xml_str
    assert "xmlns=\"http://autosar.org/schema/r4.0\"" in xml_str
    print("  ✓ XML格式正确")
    
    return generator


def test_all_param_types():
    """测试所有参数类型"""
    print("\n[Test 3] 测试所有参数类型...")
    
    module = create_module_config("TestModule", "/AUTOSAR/EcucDefs/Test")
    
    container = create_container(
        def_path="/AUTOSAR/EcucDefs/Test/TestContainer",
        short_name="TestContainer"
    )
    
    # 测试各种参数类型
    container.add_parameter(create_boolean_param(
        "/AUTOSAR/EcucDefs/Test/TestContainer/BoolParam", True))
    container.add_parameter(create_integer_param(
        "/AUTOSAR/EcucDefs/Test/TestContainer/IntParam", 42))
    container.add_parameter(create_float_param(
        "/AUTOSAR/EcucDefs/Test/TestContainer/FloatParam", 3.14159))
    container.add_parameter(create_string_param(
        "/AUTOSAR/EcucDefs/Test/TestContainer/StringParam", "Hello AUTOSAR"))
    container.add_parameter(create_enum_param(
        "/AUTOSAR/EcucDefs/Test/TestContainer/EnumParam", "OPTION_A"))
    container.add_parameter(create_function_name_param(
        "/AUTOSAR/EcucDefs/Test/TestContainer/FuncParam", "MyCallbackFunction"))
    
    module.add_container(container)
    
    generator = ArxmlEcucGenerator()
    generator.register_module(module)
    
    xml_str = generator.to_string(pretty=True)
    
    assert "ECUC-NUMERICAL-PARAM-VALUE" in xml_str
    assert "ECUC-TEXTUAL-PARAM-VALUE" in xml_str
    assert "ECUC-FUNCTION-NAME-DEF" in xml_str
    assert "true" in xml_str
    assert "42" in xml_str
    assert "3.141590" in xml_str
    assert "Hello AUTOSAR" in xml_str
    assert "OPTION_A" in xml_str
    print("  ✓ 所有参数类型生成正确")
    
    return generator


def test_complex_structure():
    """测试复杂嵌套结构"""
    print("\n[Test 4] 测试复杂嵌套结构...")
    
    # 创建Os模块
    os_module = create_module_config("Os", "/AUTOSAR/EcucDefs/Os")
    
    # 创建多层嵌套结构
    os_app = create_container(
        def_path="/AUTOSAR/EcucDefs/Os/OsApplication",
        short_name="OsApplication_App1"
    )
    
    # 添加子容器 - Task
    os_task = create_container(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask",
        short_name="OsTask_Task1"
    )
    os_task.add_parameter(create_integer_param(
        "/AUTOSAR/EcucDefs/Os/OsTask/OsTaskPriority", 5))
    os_task.add_parameter(create_boolean_param(
        "/AUTOSAR/EcucDefs/Os/OsTask/OsTaskAutostart", True))
    
    # 添加子子容器 - Autostart
    autostart = create_container(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask/OsTaskAutostartRef",
        short_name="OsTaskAutostart"
    )
    autostart.add_reference(create_reference_value(
        "/AUTOSAR/EcucDefs/Os/OsTask/OsTaskAutostartRef/OsTaskAppModeRef",
        "/ECUC/Os/OsAppMode/OSDEFAULTAPPMODE"
    ))
    
    os_task.add_sub_container(autostart)
    os_app.add_sub_container(os_task)
    
    # 添加更多任务
    os_task2 = create_container(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask",
        short_name="OsTask_Task2"
    )
    os_task2.add_parameter(create_integer_param(
        "/AUTOSAR/EcucDefs/Os/OsTask/OsTaskPriority", 3))
    os_app.add_sub_container(os_task2)
    
    os_module.add_container(os_app)
    
    generator = ArxmlEcucGenerator()
    generator.register_module(os_module)
    
    xml_str = generator.to_string(pretty=True)
    
    assert "ECUC-CONTAINER-VALUE" in xml_str
    assert "SUB-CONTAINERS" in xml_str
    assert "OsApplication_App1" in xml_str
    assert "OsTask_Task1" in xml_str
    assert "OsTaskAutostart" in xml_str
    assert "OsTask_Task2" in xml_str
    print("  ✓ 嵌套结构生成正确")
    
    return generator


def test_output_file():
    """测试文件输出"""
    print("\n[Test 5] 测试文件输出...")
    
    dio_module = create_module_config("Dio", "/AUTOSAR/EcucDefs/Dio")
    dio_config = create_container(
        def_path="/AUTOSAR/EcucDefs/Dio/DioConfig",
        short_name="DioConfig"
    )
    dio_config.add_parameter(create_boolean_param(
        "/AUTOSAR/EcucDefs/Dio/DioConfig/DioDevErrorDetect", False))
    dio_config.add_parameter(create_integer_param(
        "/AUTOSAR/EcucDefs/Dio/DioConfig/DioPortCount", 6))
    dio_module.add_container(dio_config)
    
    generator = ArxmlEcucGenerator()
    generator.register_module(dio_module)
    
    # 保存到文件
    output_path = "/tmp/test_ecuc_config.arxml"
    generator.save(output_path)
    
    assert os.path.exists(output_path)
    with open(output_path, 'r') as f:
        content = f.read()
        assert "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" in content
        assert "Dio" in content
        assert "DioConfig" in content
    
    print(f"  ✓ 文件保存成功: {output_path}")
    
    # 清理
    os.remove(output_path)
    print("  ✓ 测试文件已清理")


def run_all_tests():
    """运行所有测试"""
    print("=" * 60)
    print("ARXML ECUC Generator Test Suite")
    print("=" * 60)
    
    try:
        test_config_model()
        test_arxml_generator()
        test_all_param_types()
        test_complex_structure()
        test_output_file()
        
        print("\n" + "=" * 60)
        print("✅ 所有测试通过!")
        print("=" * 60)
        return True
        
    except AssertionError as e:
        print(f"\n❌ 测试失败: {e}")
        return False
    except Exception as e:
        print(f"\n❌ 测试异常: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)
