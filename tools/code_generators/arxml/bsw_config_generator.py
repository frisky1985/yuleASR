#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
BSW Service Layer Configuration Generator

生成基础软件服务层（BSW）配置的ARXML文件
支持的BSW模块: Com, PduR, CanIf, CanTp, NvM, Dcm, Dem, EcuM
"""

from typing import Dict, List, Optional, Any
try:
    from ecuc_config_model import (
        EcucModuleConfigurationValues,
        EcucContainerValue,
        EcucBooleanParamValue,
        EcucIntegerParamValue,
        EcucFloatParamValue,
        EcucStringParamValue,
        EcucEnumParamValue,
        EcucFunctionNameParamValue,
        EcucReferenceValue,
        EcucDefinitionRef
    )
except ImportError:
    from .ecuc_config_model import (
        EcucModuleConfigurationValues,
        EcucContainerValue,
        EcucBooleanParamValue,
        EcucIntegerParamValue,
        EcucFloatParamValue,
        EcucStringParamValue,
        EcucEnumParamValue,
        EcucFunctionNameParamValue,
        EcucReferenceValue,
        EcucDefinitionRef
    )


class BSWConfigGenerator:
    """
    BSW配置生成器基类
    提供Vector Configurator风格的API来生成BSW模块配置
    """
    
    # AUTOSAR ECUC定义路径前缀
    ECUC_DEFS_PREFIX = "/AUTOSAR/EcucDefs"
    
    def __init__(self, module_name: str, ecu_name: str = "ECU0"):
        """
        初始化BSW配置生成器
        
        Args:
            module_name: 模块名称 (Com, PduR, NvM, 等)
            ecu_name: ECU实例名称
        """
        self.module_name = module_name
        self.ecu_name = ecu_name
        
        # 创建模块配置
        module_def = EcucDefinitionRef(
            dest="ECUC-MODULE-DEF",
            value=f"{self.ECUC_DEFS_PREFIX}/{module_name}"
        )
        self.module_config = EcucModuleConfigurationValues(
            short_name=module_name,
            definition_ref=module_def
        )
    
    def _create_def_ref(self, path: str, dest: str = "ECUC-PARAM-CONF-CONTAINER-DEF") -> EcucDefinitionRef:
        """创建定义引用"""
        full_path = f"{self.ECUC_DEFS_PREFIX}/{self.module_name}/{path}"
        return EcucDefinitionRef(dest=dest, value=full_path)
    
    def _create_param_def_ref(self, path: str, param_type: str) -> EcucDefinitionRef:
        """创建参数定义引用"""
        type_mapping = {
            "BOOLEAN": "ECUC-BOOLEAN-PARAM-DEF",
            "INTEGER": "ECUC-INTEGER-PARAM-DEF",
            "FLOAT": "ECUC-FLOAT-PARAM-DEF",
            "STRING": "ECUC-STRING-PARAM-DEF",
            "ENUM": "ECUC-ENUMERATION-PARAM-DEF",
            "FUNCTION-NAME": "ECUC-FUNCTION-NAME-DEF",
            "LINKER-SYMBOL": "ECUC-LINKER-SYMBOL-DEF",
            "MACRO": "ECUC-MACRO-DEF"
        }
        full_path = f"{self.ECUC_DEFS_PREFIX}/{self.module_name}/{path}"
        return EcucDefinitionRef(dest=type_mapping.get(param_type, "ECUC-PARAM-CONF-CONTAINER-DEF"), value=full_path)
    
    def get_module_config(self) -> EcucModuleConfigurationValues:
        """获取模块配置对象"""
        return self.module_config
    
    def to_arxml(self) -> str:
        """生成ARXML字符串"""
        try:
            from arxml_ecuc_generator import ArxmlEcucGenerator
        except ImportError:
            from .arxml_ecuc_generator import ArxmlEcucGenerator
        generator = ArxmlEcucGenerator()
        return generator.generate_module_config(self.module_config)


class ComConfigGenerator(BSWConfigGenerator):
    """
    COM通信服务配置生成器
    支持IPDU、Signal、SignalGroup配置
    """
    
    def __init__(self, ecu_name: str = "ECU0"):
        super().__init__("Com", ecu_name)
    
    def add_general_config(self,
                          dev_error_detect: bool = True,
                          enable_update_bit_check: bool = True,
                          signal_change_check: bool = True) -> "ComConfigGenerator":
        """添加通用配置"""
        container = EcucContainerValue(
            short_name="ComGeneral",
            definition_ref=self._create_def_ref("ComGeneral")
        )
        
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("ComGeneral/ComDevErrorDetect", "BOOLEAN"),
            dev_error_detect
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("ComGeneral/ComEnableUpdateBitCheck", "BOOLEAN"),
            enable_update_bit_check
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("ComGeneral/ComEnableSignalCheck", "BOOLEAN"),
            signal_change_check
        ))
        
        self.module_config.add_container(container)
        return self
    
    def add_ipdu_config(self,
                       ipdu_name: str,
                       pdu_id: int = 0,
                       length: int = 8,
                       direction: str = "SEND",
                       transmission_mode: str = "DIRECT") -> "ComConfigGenerator":
        """
        添加IPDU配置
        
        Args:
            ipdu_name: IPDU名称
            pdu_id: PDU ID
            length: PDU长度（字节）
            direction: 方向 (SEND, RECEIVE)
            transmission_mode: 传输模式 (DIRECT, PERIODIC, MIXED)
        """
        config = EcucContainerValue(
            short_name="ComConfig",
            definition_ref=self._create_def_ref("ComConfig")
        )
        
        ipdu = EcucContainerValue(
            short_name=ipdu_name,
            definition_ref=self._create_def_ref("ComIPdu")
        )
        
        ipdu.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("ComIPdu/ComIPduHandleId", "INTEGER"),
            pdu_id
        ))
        ipdu.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("ComIPdu/ComIPduLength", "INTEGER"),
            length
        ))
        ipdu.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("ComIPdu/ComIPduDirection", "ENUM"),
            direction
        ))
        ipdu.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("ComIPdu/ComIPduType", "ENUM"),
            "NORMAL"
        ))
        
        # 传输模式配置
        tx_mode = EcucContainerValue(
            short_name="ComTxMode",
            definition_ref=self._create_def_ref("ComTxMode")
        )
        tx_mode.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("ComTxMode/ComTxModeMode", "ENUM"),
            transmission_mode
        ))
        
        ipdu.add_sub_container(tx_mode)
        config.add_sub_container(ipdu)
        self.module_config.add_container(config)
        
        return self
    
    def add_signal_config(self,
                         signal_name: str,
                         ipdu_ref: str,
                         start_bit: int = 0,
                         bit_length: int = 8,
                         endianness: str = "LITTLE_ENDIAN",
                         init_value: int = 0) -> "ComConfigGenerator":
        """
        添加信号配置
        
        Args:
            signal_name: 信号名称
            ipdu_ref: 所属IPDU名称
            start_bit: 起始位
            bit_length: 位长度
            endianness: 字节序
            init_value: 初始值
        """
        config = [c for c in self.module_config.containers if c.short_name == "ComConfig"]
        if not config:
            config = EcucContainerValue(
                short_name="ComConfig",
                definition_ref=self._create_def_ref("ComConfig")
            )
            self.module_config.add_container(config)
        else:
            config = config[0]
        
        signal = EcucContainerValue(
            short_name=signal_name,
            definition_ref=self._create_def_ref("ComSignal")
        )
        
        signal.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("ComSignal/ComBitPosition", "INTEGER"),
            start_bit
        ))
        signal.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("ComSignal/ComBitSize", "INTEGER"),
            bit_length
        ))
        signal.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("ComSignal/ComSignalEndianness", "ENUM"),
            endianness
        ))
        signal.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("ComSignal/ComSignalInitValue", "INTEGER"),
            init_value
        ))
        
        config.add_sub_container(signal)
        return self


class PduRConfigGenerator(BSWConfigGenerator):
    """
    PDU Router配置生成器
    支持路由表和路由路径配置
    """
    
    def __init__(self, ecu_name: str = "ECU0"):
        super().__init__("PduR", ecu_name)
    
    def add_general_config(self,
                          dev_error_detect: bool = True,
                          retry_transmit: bool = True,
                          meta_data_support: bool = False) -> "PduRConfigGenerator":
        """添加通用配置"""
        container = EcucContainerValue(
            short_name="PduRGeneral",
            definition_ref=self._create_def_ref("PduRGeneral")
        )
        
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("PduRGeneral/PduRDevErrorDetect", "BOOLEAN"),
            dev_error_detect
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("PduRGeneral/PduRRetryTransmit", "BOOLEAN"),
            retry_transmit
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("PduRGeneral/PduRSupportMetaData", "BOOLEAN"),
            meta_data_support
        ))
        
        self.module_config.add_container(container)
        return self
    
    def add_routing_path(self,
                        path_name: str,
                        src_module: str = "COM",
                        dest_module: str = "CANIF",
                        pdu_id: int = 0) -> "PduRConfigGenerator":
        """
        添加PDU路由路径
        
        Args:
            path_name: 路径名称
            src_module: 源模块 (COM, CANTP, 等)
            dest_module: 目标模块 (CANIF, CANTP, 等)
            pdu_id: PDU ID
        """
        routing_tables = EcucContainerValue(
            short_name="PduRRoutingTables",
            definition_ref=self._create_def_ref("PduRRoutingTables")
        )
        
        path = EcucContainerValue(
            short_name=path_name,
            definition_ref=self._create_def_ref("PduRRoutingPath")
        )
        
        # 源布局
        src = EcucContainerValue(
            short_name="PduRSrcPdu",
            definition_ref=self._create_def_ref("PduRSrcPdu")
        )
        src.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("PduRSrcPdu/PduRSourcePduHandleId", "INTEGER"),
            pdu_id
        ))
        path.add_sub_container(src)
        
        # 目标布局
        dest = EcucContainerValue(
            short_name="PduRDestPdu",
            definition_ref=self._create_def_ref("PduRDestPdu")
        )
        dest.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("PduRDestPdu/PduRDestPduHandleId", "INTEGER"),
            pdu_id
        ))
        path.add_sub_container(dest)
        
        routing_tables.add_sub_container(path)
        self.module_config.add_container(routing_tables)
        
        return self


class CanIfConfigGenerator(BSWConfigGenerator):
    """
    CAN Interface配置生成器
    支持控制器和硬件接收/Transmission Handle配置
    """
    
    def __init__(self, ecu_name: str = "ECU0"):
        super().__init__("CanIf", ecu_name)
    
    def add_general_config(self,
                          dev_error_detect: bool = True,
                          software_filter: bool = True,
                          meta_data_support: bool = False) -> "CanIfConfigGenerator":
        """添加通用配置"""
        container = EcucContainerValue(
            short_name="CanIfGeneral",
            definition_ref=self._create_def_ref("CanIfGeneral")
        )
        
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("CanIfGeneral/CanIfDevErrorDetect", "BOOLEAN"),
            dev_error_detect
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("CanIfGeneral/CanIfSoftwareFilter", "BOOLEAN"),
            software_filter
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("CanIfGeneral/CanIfMetaDataSupport", "BOOLEAN"),
            meta_data_support
        ))
        
        self.module_config.add_container(container)
        return self
    
    def add_controller_config(self,
                             controller_id: int = 0,
                             controller_ref: str = "CanController_0") -> "CanIfConfigGenerator":
        """
        添加控制器配置
        
        Args:
            controller_id: 控制器ID
            controller_ref: CAN驱动控制器引用
        """
        config = EcucContainerValue(
            short_name="CanIfCtrlCfg",
            definition_ref=self._create_def_ref("CanIfCtrlCfg")
        )
        
        config.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("CanIfCtrlCfg/CanIfCtrlId", "INTEGER"),
            controller_id
        ))
        
        # 添加到CanIfInitCfg
        init_cfg = [c for c in self.module_config.containers if c.short_name == "CanIfInitCfg"]
        if not init_cfg:
            init_cfg = EcucContainerValue(
                short_name="CanIfInitCfg",
                definition_ref=self._create_def_ref("CanIfInitCfg")
            )
            self.module_config.add_container(init_cfg)
        else:
            init_cfg = init_cfg[0]
        
        init_cfg.add_sub_container(config)
        return self
    
    def add_hrh_config(self,
                      hrh_name: str,
                      controller_id: int = 0,
                      hoh_ref: str = "CanHardwareObject_0") -> "CanIfConfigGenerator":
        """
        添加硬件接收Handle配置
        
        Args:
            hrh_name: HRH名称
            controller_id: 控制器ID
            hoh_ref: 硬件对象引用
        """
        init_cfg = [c for c in self.module_config.containers if c.short_name == "CanIfInitCfg"]
        if not init_cfg:
            init_cfg = EcucContainerValue(
                short_name="CanIfInitCfg",
                definition_ref=self._create_def_ref("CanIfInitCfg")
            )
            self.module_config.add_container(init_cfg)
        else:
            init_cfg = init_cfg[0]
        
        hrh = EcucContainerValue(
            short_name=hrh_name,
            definition_ref=self._create_def_ref("CanIfHrhCfg")
        )
        
        hrh.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("CanIfHrhCfg/CanIfHrhIdSymRef", "INTEGER"),
            controller_id
        ))
        hrh.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("CanIfHrhCfg/CanIfHrhSoftwareFilter", "ENUM"),
            "TRUE"
        ))
        
        init_cfg.add_sub_container(hrh)
        return self


class NvMConfigGenerator(BSWConfigGenerator):
    """
    NVRAM Manager配置生成器
    支持块描述符配置
    """
    
    def __init__(self, ecu_name: str = "ECU0"):
        super().__init__("NvM", ecu_name)
    
    def add_common_config(self,
                         api_config_class: str = "NVM_API_CONFIG_CLASS_3",
                         compiled_config_id: int = 0,
                         crc_num_bytes: int = 4,
                         dev_error_detect: bool = True,
                         main_function_period: float = 10.0) -> "NvMConfigGenerator":
        """
        添加通用配置
        
        Args:
            api_config_class: API配置类 (NVM_API_CONFIG_CLASS_1/2/3)
            compiled_config_id: 编译配置ID
            crc_num_bytes: CRC字节数
            dev_error_detect: 开发错误检测
            main_function_period: 主函数周期(ms)
        """
        container = EcucContainerValue(
            short_name="NvMCommon",
            definition_ref=self._create_def_ref("NvMCommon")
        )
        
        container.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("NvMCommon/NvMApiConfigClass", "ENUM"),
            api_config_class
        ))
        container.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("NvMCommon/NvMCompiledConfigId", "INTEGER"),
            compiled_config_id
        ))
        container.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("NvMCommon/NvMCrcNumOfBytes", "INTEGER"),
            crc_num_bytes
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("NvMCommon/NvMDevErrorDetect", "BOOLEAN"),
            dev_error_detect
        ))
        container.add_parameter(EcucFloatParamValue(
            self._create_param_def_ref("NvMCommon/NvMMainFunctionPeriod", "FLOAT"),
            main_function_period
        ))
        
        self.module_config.add_container(container)
        return self
    
    def add_block_descriptor(self,
                            block_name: str,
                            block_id: int = 0,
                            block_size: int = 32,
                            crc_type: str = "NVM_CRC32",
                            job_priority: int = 1,
                            management_type: str = "NVM_BLOCK_NATIVE",
                            write_retries: int = 3) -> "NvMConfigGenerator":
        """
        添加NVRAM块描述符
        
        Args:
            block_name: 块名称
            block_id: 块ID
            block_size: 块大小
            crc_type: CRC类型 (NVM_CRC8, NVM_CRC16, NVM_CRC32, NVM_CRC_NONE)
            job_priority: 作业优先级 (0=immediate, 1=high, 2=low)
            management_type: 管理类型
            write_retries: 写重试次数
        """
        block_desc = EcucContainerValue(
            short_name=block_name,
            definition_ref=self._create_def_ref("NvMBlockDescriptor")
        )
        
        block_desc.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("NvMBlockDescriptor/NvMBlockBaseNumber", "INTEGER"),
            block_id
        ))
        block_desc.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("NvMBlockDescriptor/NvMBlockSize", "INTEGER"),
            block_size
        ))
        block_desc.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("NvMBlockDescriptor/NvMBlockCrcType", "ENUM"),
            crc_type
        ))
        block_desc.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("NvMBlockDescriptor/NvMBlockJobPriority", "INTEGER"),
            job_priority
        ))
        block_desc.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("NvMBlockDescriptor/NvMBlockManagementType", "ENUM"),
            management_type
        ))
        block_desc.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("NvMBlockDescriptor/NvMMaxNumOfWriteRetries", "INTEGER"),
            write_retries
        ))
        
        self.module_config.add_container(block_desc)
        return self


# 工厂函数

def create_com_config(ecu_name: str = "ECU0") -> ComConfigGenerator:
    """创建COM配置生成器"""
    return ComConfigGenerator(ecu_name)


def create_pdur_config(ecu_name: str = "ECU0") -> PduRConfigGenerator:
    """创建PduR配置生成器"""
    return PduRConfigGenerator(ecu_name)


def create_canif_config(ecu_name: str = "ECU0") -> CanIfConfigGenerator:
    """创建CanIf配置生成器"""
    return CanIfConfigGenerator(ecu_name)


def create_nvm_config(ecu_name: str = "ECU0") -> NvMConfigGenerator:
    """创建NvM配置生成器"""
    return NvMConfigGenerator(ecu_name)
