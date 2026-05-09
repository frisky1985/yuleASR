#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
AUTOSAR Com模块配置生成器

根据DBC或CAN Matrix生成Com_Cfg.h和Com_Cfg.c配置文件
"""

from typing import List, Dict, Any, Optional
from pathlib import Path
from jinja2 import Template


# Com_Cfg.h 模板
COM_CFG_H_TEMPLATE = """/**
 * @file Com_Cfg.h
 * @brief AUTOSAR Com模块配置头文件
 * @note 自动生成于 {{ timestamp }}
 * @note 生成工具: yuleASR CAN Config Tool
 */

#ifndef COM_CFG_H
#define COM_CFG_H

#include "ComStack_Types.h"
#include "Com_Types.h"

/*==================[version check]==========================================*/
#define COM_CFG_MAJOR_VERSION    1
#define COM_CFG_MINOR_VERSION    0
#define COM_CFG_PATCH_VERSION    0

/*==================[general configuration]==================================*/
/**
 * @brief 开发错误检测使能
 */
#define COM_DEV_ERROR_DETECT     {{ 'STD_ON' if dev_error_detect else 'STD_OFF' }}

/**
 * @brief 版本信息API使能
 */
#define COM_VERSION_INFO_API     {{ 'STD_ON' if version_info_api else 'STD_OFF' }}

/**
 * @brief Update位检查使能
 */
#define COM_ENABLE_UPDATE_BIT_CHECK  {{ 'STD_ON' if enable_update_bit_check else 'STD_OFF' }}

/**
 * @brief 信号改变检查
 */
#define COM_SIGNAL_CHANGE_CHECK  {{ 'STD_ON' if signal_change_check else 'STD_OFF' }}

/*==================[IPDU configuration]=====================================*/
/**
 * @brief IPDU数量
 */
#define COM_NUM_OF_IPDUS     {{ ipdus|length }}

/**
 * @brief 信号数量
 */
#define COM_NUM_OF_SIGNALS   {{ signals|length }}

/**
 * @brief 信号组数量
 */
#define COM_NUM_OF_SIGNAL_GROUPS   {{ signal_groups|length }}

{% for ipdu in ipdus %}
/**
 * @brief {{ ipdu.name }} - CAN ID: 0x{{ "%04X" % ipdu.message_id }}
 */
#define {{ ipdu.name }}_ID          {{ ipdu.index }}
#define {{ ipdu.name }}_CANID       (0x{{ "%04X" % ipdu.message_id }}U)
#define {{ ipdu.name }}_DLC         {{ ipdu.dlc }}U
#define {{ ipdu.name }}_CYCLE       {{ ipdu.cycle_time }}U

{% endfor %}

/*==================[signal configuration]===================================*/
{% for signal in signals %}
/**
 * @brief {{ signal.name }}
 * @details IPDU: {{ signal.ipdu }}, StartBit: {{ signal.start_bit }}, Length: {{ signal.bit_length }}
 */
#define ComConf_ComSignal_{{ signal.name }}    {{ signal.index }}

{% endfor %}

/*==================[data type definitions]==================================*/
{% for dtype in unique_data_types %}
typedef {{ dtype.c_type }} {{ dtype.name }};
{% endfor %}

/*==================[external declarations]==================================*/
extern const Com_ConfigType Com_Config;

#endif /* COM_CFG_H */
"""

# Com_Cfg.c 模板
COM_CFG_C_TEMPLATE = """/**
 * @file Com_Cfg.c
 * @brief AUTOSAR Com模块配置源文件
 * @note 自动生成于 {{ timestamp }}
 * @note 生成工具: yuleASR CAN Config Tool
 */

#include "Com_Cfg.h"
#include "Com.h"

/*==================[internal data]==========================================*/
{% for ipdu in ipdus %}
/**
 * @brief {{ ipdu.name }} 数据缓冲区
 */
static uint8 {{ ipdu.name }}_Buffer[{{ ipdu.dlc }}] = {0};

{% endfor %}

{% if signals|length > 0 %}
/**
 * @brief 信号初始值
 */
static const uint8 Com_SignalInitValues[{{ signals|length }}] = {
{% for signal in signals %}
    {{ signal.init_value }}U,  /* {{ signal.name }} */
{% endfor %}
};

/**
 * @brief 信号因子数组
 */
static const float32 Com_SignalFactors[{{ signals|length }}] = {
{% for signal in signals %}
    {{ signal.factor }}F,  /* {{ signal.name }} */
{% endfor %}
};

/**
 * @brief 信号偏移数组
 */
static const float32 Com_SignalOffsets[{{ signals|length }}] = {
{% for signal in signals %}
    {{ signal.offset }}F,  /* {{ signal.name }} */
{% endfor %}
};
{% endif %}

/*==================[IPDU configuration]=====================================*/
{% for ipdu in ipdus %}
/**
 * @brief {{ ipdu.name }}信号配置
 */
static const Com_SignalConfigType {{ ipdu.name }}_Signals[{{ ipdu.signals|length }}] = {
{% for sig_name in ipdu.signals %}
{% set signal = signals|selectattr("name", "equalto", sig_name)|first %}
    {
        /* {{ signal.name }} */
        .ComBitPosition = {{ signal.start_bit }}U,
        .ComBitSize = {{ signal.bit_length }}U,
        .ComByteDirection = COM_{{ signal.byte_order }},
        .ComDataType = COM_{{ signal.data_type }},
        .ComFactor = {{ signal.factor }}F,
        .ComOffset = {{ signal.offset }}F,
        .ComMinimum = {{ signal.minimum }}F,
        .ComMaximum = {{ signal.maximum }}F,
        .ComInitValue = {{ signal.init_value }}U,
        .ComSignalUpdated = FALSE,
    },
{% endfor %}
};

{% endfor %}

/**
 * @brief IPDU配置表
 */
const Com_IPduConfigType Com_IPduConfig[COM_NUM_OF_IPDUS] = {
{% for ipdu in ipdus %}
    {
        /* {{ ipdu.name }} - CAN ID: 0x{{ "%04X" % ipdu.message_id }} */
        .ComIPduHandleId = {{ ipdu.name }}_ID,
        .ComIPduDirection = COM_{{ ipdu.direction }},
        .ComIPduSize = {{ ipdu.dlc }}U,
        .ComIPduCounterSize = 0U,
        .ComIPduCycleTime = {{ ipdu.cycle_time }}U,
        .ComIPduBufferRef = {{ ipdu.name }}_Buffer,
        .ComIPduNumOfSignals = {{ ipdu.signals|length }}U,
        .ComIPduSignalConfigRef = {{ ipdu.name }}_Signals,
    },
{% endfor %}
};

/*==================[signal configuration]===================================*/
/**
 * @brief 信号配置表
 */
const Com_SignalConfigType Com_SignalConfig[COM_NUM_OF_SIGNALS] = {
{% for signal in signals %}
    {
        /* {{ signal.name }} */
        .ComBitPosition = {{ signal.start_bit }}U,
        .ComBitSize = {{ signal.bit_length }}U,
        .ComByteDirection = COM_{{ signal.byte_order }},
        .ComDataType = COM_{{ signal.data_type }},
        .ComFactor = {{ signal.factor }}F,
        .ComOffset = {{ signal.offset }}F,
        .ComMinimum = {{ signal.minimum }}F,
        .ComMaximum = {{ signal.maximum }}F,
        .ComInitValue = {{ signal.init_value }}U,
    },
{% endfor %}
};

/*==================[global configuration]===================================*/
/**
 * @brief Com模块全局配置
 */
const Com_ConfigType Com_Config = {
    .ComGeneral = {
        .ComDevErrorDetect = COM_DEV_ERROR_DETECT,
        .ComVersionInfoApi = COM_VERSION_INFO_API,
        .ComEnableUpdateBitCheck = COM_ENABLE_UPDATE_BIT_CHECK,
        .ComSignalChangeCheck = COM_SIGNAL_CHANGE_CHECK,
    },
    .ComIPdu = {
        .ComIPduConfig = Com_IPduConfig,
        .ComNumOfIPdus = COM_NUM_OF_IPDUS,
    },
    .ComSignal = {
        .ComSignalConfig = Com_SignalConfig,
        .ComNumOfSignals = COM_NUM_OF_SIGNALS,
    },
};

/*==================[end of file]============================================*/
"""


class ComConfigGenerator:
    """Com模块配置生成器"""
    
    # 数据类型映射表
    DATA_TYPE_MAP = {
        'UINT8': {'c_type': 'uint8', 'size': 1},
        'UINT16': {'c_type': 'uint16', 'size': 2},
        'UINT32': {'c_type': 'uint32', 'size': 4},
        'UINT64': {'c_type': 'uint64', 'size': 8},
        'SINT8': {'c_type': 'sint8', 'size': 1},
        'SINT16': {'c_type': 'sint16', 'size': 2},
        'SINT32': {'c_type': 'sint32', 'size': 4},
        'SINT64': {'c_type': 'sint64', 'size': 8},
        'FLOAT32': {'c_type': 'float32', 'size': 4},
        'FLOAT64': {'c_type': 'float64', 'size': 8},
    }
    
    def __init__(self, config: Dict[str, Any]):
        """
        初始化生成器
        
        Args:
            config: 配置数据字典
        """
        self.config = config
        self._prepare_config()
    
    def _prepare_config(self):
        """预处理配置数据"""
        # 为IPDU分配索引
        for idx, ipdu in enumerate(self.config.get('ipdus', [])):
            ipdu['index'] = idx
            
        # 为信号分配索引
        for idx, signal in enumerate(self.config.get('signals', [])):
            signal['index'] = idx
            
        # 收集唯一数据类型
        unique_types = set()
        for signal in self.config.get('signals', []):
            dtype = signal.get('data_type', 'UINT32')
            if dtype in self.DATA_TYPE_MAP:
                unique_types.add(dtype)
                
        self.config['unique_data_types'] = [
            {'name': t, 'c_type': self.DATA_TYPE_MAP[t]['c_type']}
            for t in sorted(unique_types)
        ]
        
        # 添加默认值
        self.config.setdefault('dev_error_detect', True)
        self.config.setdefault('version_info_api', False)
        self.config.setdefault('enable_update_bit_check', True)
        self.config.setdefault('signal_change_check', True)
        
        # 添加时间戳
        from datetime import datetime
        self.config['timestamp'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    def generate_cfg_h(self) -> str:
        """
        生成Com_Cfg.h文件内容
        
        Returns:
            str: 头文件内容
        """
        template = Template(COM_CFG_H_TEMPLATE)
        return template.render(**self.config)
    
    def generate_cfg_c(self) -> str:
        """
        生成Com_Cfg.c文件内容
        
        Returns:
            str: 源文件内容
        """
        template = Template(COM_CFG_C_TEMPLATE)
        return template.render(**self.config)
    
    def generate(self, output_dir: str, prefix: str = ""):
        """
        生成配置文件
        
        Args:
            output_dir: 输出目录
            prefix: 文件前缀
        """
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        # 生成头文件
        cfg_h_content = self.generate_cfg_h()
        cfg_h_path = output_path / f"{prefix}Com_Cfg.h"
        with open(cfg_h_path, 'w', encoding='utf-8') as f:
            f.write(cfg_h_content)
            
        # 生成源文件
        cfg_c_content = self.generate_cfg_c()
        cfg_c_path = output_path / f"{prefix}Com_Cfg.c"
        with open(cfg_c_path, 'w', encoding='utf-8') as f:
            f.write(cfg_c_content)
            
        return cfg_h_path, cfg_c_path
    
    def generate_summary(self) -> str:
        """
        生成配置摘要
        
        Returns:
            str: 摘要信息
        """
        ipdus = self.config.get('ipdus', [])
        signals = self.config.get('signals', [])
        
        lines = [
            "=" * 60,
            "  Com模块配置摘要",
            "=" * 60,
            f"",
            f"  ECU名称: {self.config.get('ecu_name', 'ECU0')}",
            f"  IPDU数量: {len(ipdus)}",
            f"  信号数量: {len(signals)}",
            f"",
            "  IPDU列表:",
        ]
        
        for ipdu in ipdus:
            direction = "发送" if ipdu.get('direction') == 'SEND' else "接收"
            lines.append(f"    • {ipdu['name']}: ID=0x{ipdu['message_id']:04X}, "
                        f"DLC={ipdu['dlc']}, {direction}, "
                        f"信号数={len(ipdu.get('signals', []))}")
        
        lines.extend([
            "",
            "  信号列表:",
        ])
        
        for signal in signals[:10]:  # 只显示前10个
            lines.append(f"    • {signal['name']}: {signal['data_type']}, "
                        f"Start={signal['start_bit']}, Len={signal['bit_length']}")
        
        if len(signals) > 10:
            lines.append(f"    ... 还有 {len(signals) - 10} 个信号")
        
        lines.append("=" * 60)
        
        return '\n'.join(lines)


if __name__ == "__main__":
    # 测试示例
    test_config = {
        'ecu_name': 'ECU0',
        'ipdus': [
            {
                'name': 'IPDU_EngineData',
                'message_id': 0x100,
                'dlc': 8,
                'direction': 'SEND',
                'cycle_time': 100,
                'signals': ['EngineSpeed', 'EngineTemp', 'EngineStatus']
            },
            {
                'name': 'IPDU_VehicleSpeed',
                'message_id': 0x200,
                'dlc': 4,
                'direction': 'SEND',
                'cycle_time': 50,
                'signals': ['Speed', 'SpeedValid']
            },
        ],
        'signals': [
            {
                'name': 'EngineSpeed',
                'ipdu': 'IPDU_EngineData',
                'start_bit': 0,
                'bit_length': 16,
                'byte_order': 'LITTLE_ENDIAN',
                'data_type': 'UINT16',
                'factor': 0.125,
                'offset': 0.0,
                'minimum': 0.0,
                'maximum': 8000.0,
                'init_value': 0,
            },
            {
                'name': 'EngineTemp',
                'ipdu': 'IPDU_EngineData',
                'start_bit': 16,
                'bit_length': 8,
                'byte_order': 'LITTLE_ENDIAN',
                'data_type': 'SINT8',
                'factor': 1.0,
                'offset': -40.0,
                'minimum': -40.0,
                'maximum': 215.0,
                'init_value': 0,
            },
            {
                'name': 'EngineStatus',
                'ipdu': 'IPDU_EngineData',
                'start_bit': 24,
                'bit_length': 2,
                'byte_order': 'LITTLE_ENDIAN',
                'data_type': 'UINT8',
                'factor': 1.0,
                'offset': 0.0,
                'minimum': 0.0,
                'maximum': 3.0,
                'init_value': 0,
            },
            {
                'name': 'Speed',
                'ipdu': 'IPDU_VehicleSpeed',
                'start_bit': 0,
                'bit_length': 16,
                'byte_order': 'LITTLE_ENDIAN',
                'data_type': 'UINT16',
                'factor': 0.01,
                'offset': 0.0,
                'minimum': 0.0,
                'maximum': 300.0,
                'init_value': 0,
            },
            {
                'name': 'SpeedValid',
                'ipdu': 'IPDU_VehicleSpeed',
                'start_bit': 16,
                'bit_length': 1,
                'byte_order': 'LITTLE_ENDIAN',
                'data_type': 'UINT8',
                'factor': 1.0,
                'offset': 0.0,
                'minimum': 0.0,
                'maximum': 1.0,
                'init_value': 0,
            },
        ],
        'signal_groups': []
    }
    
    gen = ComConfigGenerator(test_config)
    
    print("🚀 测试Com配置生成器")
    print(gen.generate_summary())
    
    # 生成文件
    import tempfile
    with tempfile.TemporaryDirectory() as tmpdir:
        cfg_h, cfg_c = gen.generate(tmpdir)
        print(f"\n✅ 已生成配置文件:")
        print(f"   • {cfg_h}")
        print(f"   • {cfg_c}")
        
        # 显示头文件内容预览
        print("\n📄 Com_Cfg.h 预览:")
        print("-" * 60)
        with open(cfg_h, 'r') as f:
            print(f.read()[:1000])
        print("...")
