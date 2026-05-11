#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR ARXML Generator GUI - Web Server

可视化ARXML配置工具的Web后端API
"""

import sys
import json
from pathlib import Path
from flask import Flask, render_template, request, jsonify, send_file
from flask_cors import CORS
import io

# 添加父目录到路径
sys.path.insert(0, str(Path(__file__).parent.parent.parent / "src"))

from mcal_config_generator import (
    create_mcu_config, create_port_config, create_can_config,
    create_spi_config, create_gpt_config, create_pwm_config, create_adc_config
)
from bsw_config_generator import (
    create_com_config, create_pdur_config, create_nvm_config
)

app = Flask(__name__,
    template_folder=str(Path(__file__).parent.parent / "templates"),
    static_folder=str(Path(__file__).parent.parent / "static")
)
CORS(app)

# =============================================================================
# 页面路由
# =============================================================================

@app.route("/")
def index():
    """主页面"""
    return render_template("index.html")

# =============================================================================
# API 接口
# =============================================================================

@app.route("/api/modules", methods=["GET"])
def get_modules():
    """获取支持的模块列表"""
    modules = {
        "mcal": [
            {"id": "mcu", "name": "MCU", "description": "微控制器驱动", "icon": "cpu"},
            {"id": "port", "name": "Port", "description": "GPIO引脚配置", "icon": "plug"},
            {"id": "can", "name": "CAN", "description": "CAN通信驱动", "icon": "broadcast"},
            {"id": "spi", "name": "SPI", "description": "SPI串行通信", "icon": "exchange"},
            {"id": "gpt", "name": "Gpt", "description": "通用定时器", "icon": "clock"},
            {"id": "pwm", "name": "Pwm", "description": "脉宽调制", "icon": "wave-square"},
            {"id": "adc", "name": "Adc", "description": "模拟转换", "icon": "tachometer-alt"},
        ],
        "bsw": [
            {"id": "com", "name": "COM", "description": "通信服务", "icon": "comments"},
            {"id": "pdur", "name": "PduR", "description": "PDU路由", "icon": "route"},
            {"id": "nvm", "name": "NvM", "description": "NVRAM管理", "icon": "save"},
        ]
    }
    return jsonify(modules)


@app.route("/api/schema/<module_id>", methods=["GET"])
def get_module_schema(module_id):
    """获取模块的配置索引"""
    schemas = {
        "mcu": {
            "containers": [
                {
                    "name": "McuGeneral",
                    "description": "通用配置",
                    "parameters": [
                        {"name": "McuDevErrorDetect", "type": "boolean", "default": True, "label": "开发错误检测"},
                        {"name": "McuInitClock", "type": "boolean", "default": True, "label": "初始化时钟"},
                        {"name": "McuVersionInfoApi", "type": "boolean", "default": False, "label": "版本信息API"},
                    ]
                },
                {
                    "name": "McuClockSettingConfig",
                    "description": "时钟配置",
                    "parameters": [
                        {"name": "McuClockReferencePointFrequency", "type": "integer", "default": 80000000, "label": "CPU时钟频率(Hz)", "min": 1000000, "max": 200000000},
                        {"name": "McuClockPeripheralFrequency", "type": "integer", "default": 40000000, "label": "外设时钟频率(Hz)", "min": 1000000, "max": 100000000},
                    ]
                }
            ]
        },
        "port": {
            "containers": [
                {
                    "name": "PortGeneral",
                    "description": "通用配置",
                    "parameters": [
                        {"name": "PortDevErrorDetect", "type": "boolean", "default": True, "label": "开发错误检测"},
                        {"name": "PortSetPinDirectionApi", "type": "boolean", "default": True, "label": "设置引脚方向API"},
                        {"name": "PortVersionInfoApi", "type": "boolean", "default": False, "label": "版本信息API"},
                    ]
                },
                {
                    "name": "PortPin",
                    "description": "引脚配置",
                    "is_list": True,
                    "parameters": [
                        {"name": "PortPinName", "type": "string", "default": "PortPin_0", "label": "引脚名称"},
                        {"name": "PortPinId", "type": "integer", "default": 0, "label": "引脚ID", "min": 0, "max": 255},
                        {"name": "PortPinDirection", "type": "enum", "default": "PORT_PIN_OUT", "label": "方向", "options": ["PORT_PIN_IN", "PORT_PIN_OUT"]},
                        {"name": "PortPinMode", "type": "enum", "default": "PORT_PIN_MODE_GPIO", "label": "模式", "options": ["PORT_PIN_MODE_GPIO", "PORT_PIN_MODE_CAN", "PORT_PIN_MODE_SPI", "PORT_PIN_MODE_PWM"]},
                    ]
                }
            ]
        },
        "can": {
            "containers": [
                {
                    "name": "CanGeneral",
                    "description": "通用配置",
                    "parameters": [
                        {"name": "CanDevErrorDetect", "type": "boolean", "default": True, "label": "开发错误检测"},
                        {"name": "CanIndex", "type": "integer", "default": 0, "label": "控制器索引", "min": 0, "max": 255},
                        {"name": "CanMainFunctionPeriod", "type": "float", "default": 10.0, "label": "主函数周期(ms)", "min": 1.0, "max": 1000.0},
                        {"name": "CanMultiplexedTransmission", "type": "boolean", "default": True, "label": "多路复用传输"},
                        {"name": "CanTimeoutDuration", "type": "float", "default": 0.1, "label": "超时时间(s)", "min": 0.001, "max": 10.0},
                    ]
                },
                {
                    "name": "CanController",
                    "description": "控制器配置",
                    "parameters": [
                        {"name": "CanControllerId", "type": "integer", "default": 0, "label": "控制器ID", "min": 0, "max": 255},
                        {"name": "CanControllerBaudrate", "type": "integer", "default": 500000, "label": "波特率(bps)", "min": 10000, "max": 1000000},
                        {"name": "CanControllerTxObjects", "type": "integer", "default": 4, "label": "Tx对象数量", "min": 1, "max": 64},
                        {"name": "CanControllerRxObjects", "type": "integer", "default": 2, "label": "Rx对象数量", "min": 1, "max": 64},
                    ]
                }
            ]
        },
        "nvm": {
            "containers": [
                {
                    "name": "NvMCommon",
                    "description": "通用配置",
                    "parameters": [
                        {"name": "NvMApiConfigClass", "type": "enum", "default": "NVM_API_CONFIG_CLASS_3", "label": "API配置类", "options": ["NVM_API_CONFIG_CLASS_1", "NVM_API_CONFIG_CLASS_2", "NVM_API_CONFIG_CLASS_3"]},
                        {"name": "NvMCompiledConfigId", "type": "integer", "default": 0, "label": "编译配置ID", "min": 0, "max": 65535},
                        {"name": "NvMCrcNumOfBytes", "type": "integer", "default": 4, "label": "CRC字节数", "min": 0, "max": 4},
                        {"name": "NvMDevErrorDetect", "type": "boolean", "default": True, "label": "开发错误检测"},
                        {"name": "NvMMainFunctionPeriod", "type": "float", "default": 10.0, "label": "主函数周期(ms)", "min": 1.0, "max": 1000.0},
                    ]
                },
                {
                    "name": "NvMBlockDescriptor",
                    "description": "NVRAM块配置",
                    "is_list": True,
                    "parameters": [
                        {"name": "NvMBlockCrcType", "type": "enum", "default": "NVM_CRC32", "label": "CRC类型", "options": ["NVM_CRC8", "NVM_CRC16", "NVM_CRC32", "NVM_CRC_NONE"]},
                        {"name": "NvMBlockJobPriority", "type": "integer", "default": 1, "label": "任务优先级", "min": 0, "max": 255},
                    ]
                }
            ]
        },
        "com": {
            "containers": [
                {
                    "name": "ComGeneral",
                    "description": "通用配置",
                    "parameters": [
                        {"name": "ComDevErrorDetect", "type": "boolean", "default": True, "label": "开发错误检测"},
                        {"name": "ComEnableUpdateBitCheck", "type": "boolean", "default": True, "label": "启用Update位检查"},
                        {"name": "ComEnableSignalCheck", "type": "boolean", "default": True, "label": "启用信号检查"},
                    ]
                },
                {
                    "name": "ComIPdu",
                    "description": "IPDU配置",
                    "is_list": True,
                    "parameters": [
                        {"name": "ComIPduHandleId", "type": "integer", "default": 0, "label": "IPDU处理ID", "min": 0, "max": 65535},
                        {"name": "ComIPduLength", "type": "integer", "default": 8, "label": "IPDU长度(字节)", "min": 1, "max": 4095},
                        {"name": "ComIPduDirection", "type": "enum", "default": "SEND", "label": "方向", "options": ["SEND", "RECEIVE"]},
                        {"name": "ComIPduType", "type": "enum", "default": "NORMAL", "label": "类型", "options": ["NORMAL", "TP"]},
                    ]
                },
                {
                    "name": "ComSignal",
                    "description": "信号配置",
                    "is_list": True,
                    "parameters": [
                        {"name": "ComSignalName", "type": "string", "default": "Signal_0", "label": "信号名称"},
                        {"name": "ComSignalStartBit", "type": "integer", "default": 0, "label": "起始位", "min": 0, "max": 2047},
                        {"name": "ComSignalBitLength", "type": "integer", "default": 8, "label": "位长度", "min": 1, "max": 64},
                        {"name": "ComSignalEndianness", "type": "enum", "default": "LITTLE_ENDIAN", "label": "字节序", "options": ["BIG_ENDIAN", "LITTLE_ENDIAN"]},
                    ]
                }
            ]
        }
    }
    
    if module_id not in schemas:
        return jsonify({"error": f"未知模块: {module_id}"}), 404
    
    return jsonify(schemas[module_id])


@app.route("/api/generate", methods=["POST"])
def generate_arxml():
    """生成ARXML配置"""
    data = request.json
    module_id = data.get("module")
    ecu_name = data.get("ecu", "ECU0")
    config = data.get("config", {})
    
    try:
        # 根据模块类型创建配置生成器
        if module_id == "mcu":
            gen = create_mcu_config(ecu_name)
            general = config.get("McuGeneral", {})
            gen.add_general_config(
                dev_error_detect=general.get("McuDevErrorDetect", True),
                init_clock=general.get("McuInitClock", True),
                version_info_api=general.get("McuVersionInfoApi", False)
            )
            clock = config.get("McuClockSettingConfig", {})
            gen.add_clock_config(
                cpu_clock=clock.get("McuClockReferencePointFrequency", 80000000),
                peripheral_clock=clock.get("McuClockPeripheralFrequency", 40000000)
            )
            
        elif module_id == "port":
            gen = create_port_config(ecu_name)
            general = config.get("PortGeneral", {})
            gen.add_general_config(
                dev_error_detect=general.get("PortDevErrorDetect", True),
                set_pin_direction_api=general.get("PortSetPinDirectionApi", True),
                version_info_api=general.get("PortVersionInfoApi", False)
            )
            # 添加引脚配置
            pins = config.get("PortPin", [])
            for i, pin in enumerate(pins):
                gen.add_pin_config(
                    pin_name=pin.get("PortPinName", f"PortPin_{i}"),
                    pin_id=pin.get("PortPinId", i),
                    direction=pin.get("PortPinDirection", "PORT_PIN_OUT"),
                    mode=pin.get("PortPinMode", "PORT_PIN_MODE_GPIO")
                )
                
        elif module_id == "can":
            gen = create_can_config(ecu_name)
            general = config.get("CanGeneral", {})
            gen.add_general_config(
                dev_error_detect=general.get("CanDevErrorDetect", True),
                index=general.get("CanIndex", 0),
                main_function_period=general.get("CanMainFunctionPeriod", 10.0),
                multiplexed_transmission=general.get("CanMultiplexedTransmission", True),
                timeout_duration=general.get("CanTimeoutDuration", 0.1)
            )
            controller = config.get("CanController", {})
            gen.add_controller_config(
                controller_id=controller.get("CanControllerId", 0),
                baudrate=controller.get("CanControllerBaudrate", 500000),
                tx_objects=controller.get("CanControllerTxObjects", 4),
                rx_objects=controller.get("CanControllerRxObjects", 2)
            )
            
        elif module_id == "nvm":
            gen = create_nvm_config(ecu_name)
            common = config.get("NvMCommon", {})
            crc_bytes = {"NVM_CRC8": 1, "NVM_CRC16": 2, "NVM_CRC32": 4, "NVM_CRC_NONE": 0}
            gen.add_common_config(
                api_config_class=common.get("NvMApiConfigClass", "NVM_API_CONFIG_CLASS_3"),
                compiled_config_id=common.get("NvMCompiledConfigId", 0),
                crc_num_bytes=crc_bytes.get(common.get("NvMBlockCrcType", "NVM_CRC32"), 4),
                dev_error_detect=common.get("NvMDevErrorDetect", True),
                main_function_period=common.get("NvMMainFunctionPeriod", 10.0)
            )
            # 添加NVRAM块
            blocks = config.get("NvMBlockDescriptor", [])
            for i, block in enumerate(blocks):
                gen.add_block_descriptor(
                    block_name=f"NvMBlockDescriptor_{i}",
                    block_id=i,
                    block_size=32 + i * 16,
                    crc_type=block.get("NvMBlockCrcType", "NVM_CRC32"),
                    job_priority=block.get("NvMBlockJobPriority", 1)
                )
                
        elif module_id == "com":
            gen = create_com_config(ecu_name)
            general = config.get("ComGeneral", {})
            gen.add_general_config(
                dev_error_detect=general.get("ComDevErrorDetect", True),
                enable_update_bit_check=general.get("ComEnableUpdateBitCheck", True),
                signal_change_check=general.get("ComEnableSignalCheck", True)
            )
            # 添加IPDU
            ipdus = config.get("ComIPdu", [])
            for ipdu in ipdus:
                gen.add_ipdu_config(
                    ipdu_name=f"IPDU_{ipdu.get('ComIPduHandleId', 0)}",
                    pdu_id=ipdu.get("ComIPduHandleId", 0),
                    length=ipdu.get("ComIPduLength", 8),
                    direction=ipdu.get("ComIPduDirection", "SEND"),
                    transmission_mode="DIRECT"
                )
            # 添加信号
            signals = config.get("ComSignal", [])
            for i, signal in enumerate(signals):
                gen.add_signal_config(
                    signal_name=signal.get("ComSignalName", f"Signal_{i}"),
                    ipdu_ref=f"IPDU_{i}",
                    start_bit=signal.get("ComSignalStartBit", 0),
                    bit_length=signal.get("ComSignalBitLength", 8),
                    endianness=signal.get("ComSignalEndianness", "LITTLE_ENDIAN")
                )
        else:
            return jsonify({"error": f"暂不支持的模块: {module_id}"}), 400
        
        # 生成ARXML
        arxml_content = gen.to_arxml()
        
        return jsonify({
            "success": True,
            "module": module_id,
            "ecu": ecu_name,
            "arxml": arxml_content
        })
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route("/api/download", methods=["POST"])
def download_arxml():
    """下载ARXML文件"""
    data = request.json
    arxml_content = data.get("arxml", "")
    filename = data.get("filename", "config.arxml")
    
    if not arxml_content:
        return jsonify({"error": "ARXML内容为空"}), 400
    
    # 创建字节流
    buffer = io.BytesIO(arxml_content.encode('utf-8'))
    buffer.seek(0)
    
    return send_file(
        buffer,
        mimetype='application/xml',
        as_attachment=True,
        download_name=filename
    )


# =============================================================================
# 启动服务器
# =============================================================================

def main():
    print("🚀 启动 yuleASR ARXML Generator GUI Server...")
    print("📁 访问 http://localhost:5000 使用可视化配置工具")
    app.run(host='0.0.0.0', port=5000, debug=True)


if __name__ == "__main__":
    main()
