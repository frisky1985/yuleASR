#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MCAL Configuration Generator

生成微控制器驱动层（MCAL）配置的ARXML文件
支持的MCAL模块: Mcu, Port, Dio, Can, Spi, Gpt, Pwm, Adc
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
        EcucDefinitionRef
    )


class MCALConfigGenerator:
    """
    MCAL配置生成器
    提供Vector Configurator风格的API来生成MCAL模块配置
    """
    
    # AUTOSAR ECUC定义路径前缀
    ECUC_DEFS_PREFIX = "/AUTOSAR/EcucDefs"
    
    def __init__(self, module_name: str, ecu_name: str = "ECU0"):
        """
        初始化MCAL配置生成器
        
        Args:
            module_name: 模块名称 (Mcu, Port, Can, 等)
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


class McuConfigGenerator(MCALConfigGenerator):
    """
    MCU驱动配置生成器
    支持时钟配置、复位配置、模式配置等
    """
    
    def __init__(self, ecu_name: str = "ECU0"):
        super().__init__("Mcu", ecu_name)
    
    def add_general_config(self, 
                          user_mode: bool = True,
                          init_clock: bool = True,
                          dev_error_detect: bool = True,
                          version_info_api: bool = False) -> "McuConfigGenerator":
        """
        添加通用配置
        
        Args:
            user_mode: 用户模式支持
            init_clock: 启用时钟初始化
            dev_error_detect: 开发错误检测
            version_info_api: 版本信息API
        """
        container = EcucContainerValue(
            short_name="McuGeneral",
            definition_ref=self._create_def_ref("McuGeneral")
        )
        
        # 添加参数
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("McuGeneral/McuUserModeSupport", "BOOLEAN"),
            user_mode
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("McuGeneral/McuInitClock", "BOOLEAN"),
            init_clock
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("McuGeneral/McuDevErrorDetect", "BOOLEAN"),
            dev_error_detect
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("McuGeneral/McuVersionInfoApi", "BOOLEAN"),
            version_info_api
        ))
        
        self.module_config.add_container(container)
        return self
    
    def add_clock_config(self,
                        clock_name: str = "McuClockSettingConfig_0",
                        cpu_clock: int = 80000000,
                        peripheral_clock: int = 40000000,
                        bus_clock: int = 40000000) -> "McuConfigGenerator":
        """
        添加时钟配置
        
        Args:
            clock_name: 时钟配置名称
            cpu_clock: CPU时钟频率 (Hz)
            peripheral_clock: 外设时钟频率 (Hz)
            bus_clock: 总线时钟频率 (Hz)
        """
        container = EcucContainerValue(
            short_name=clock_name,
            definition_ref=self._create_def_ref("McuClockSettingConfig")
        )
        
        container.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("McuClockSettingConfig/McuClockReferencePointFrequency", "INTEGER"),
            cpu_clock
        ))
        
        self.module_config.add_container(container)
        
        # 添加到McuModuleConfiguration容器
        mcu_config = EcucContainerValue(
            short_name="McuModuleConfiguration",
            definition_ref=self._create_def_ref("McuModuleConfiguration")
        )
        mcu_config.add_sub_container(container)
        
        # 检查是否已存在McuModuleConfiguration
        existing = [c for c in self.module_config.containers if c.short_name == "McuModuleConfiguration"]
        if not existing:
            self.module_config.add_container(mcu_config)
        else:
            existing[0].add_sub_container(container)
        
        return self
    
    def add_mode_config(self, mode_name: str = "McuModeSettingConf_0") -> "McuConfigGenerator":
        """添加MCU模式配置"""
        container = EcucContainerValue(
            short_name=mode_name,
            definition_ref=self._create_def_ref("McuModeSettingConf")
        )
        
        # 添加到McuModuleConfiguration
        existing = [c for c in self.module_config.containers if c.short_name == "McuModuleConfiguration"]
        if existing:
            existing[0].add_sub_container(container)
        else:
            mcu_config = EcucContainerValue(
                short_name="McuModuleConfiguration",
                definition_ref=self._create_def_ref("McuModuleConfiguration")
            )
            mcu_config.add_sub_container(container)
            self.module_config.add_container(mcu_config)
        
        return self
    
    def add_ram_section(self, section_name: str = "McuRamSectorSettingConf_0",
                       size: int = 65536, address: int = 0x20000000) -> "McuConfigGenerator":
        """添加RAM区段配置"""
        container = EcucContainerValue(
            short_name=section_name,
            definition_ref=self._create_def_ref("McuRamSectorSettingConf")
        )
        
        container.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("McuRamSectorSettingConf/McuRamSectorSize", "INTEGER"),
            size
        ))
        
        # 添加到McuModuleConfiguration
        existing = [c for c in self.module_config.containers if c.short_name == "McuModuleConfiguration"]
        if existing:
            existing[0].add_sub_container(container)
        
        return self


class PortConfigGenerator(MCALConfigGenerator):
    """
    Port驱动配置生成器
    支持引脚方向、模式、驱动能力等配置
    """
    
    def __init__(self, ecu_name: str = "ECU0"):
        super().__init__("Port", ecu_name)
    
    def add_general_config(self,
                          dev_error_detect: bool = True,
                          version_info_api: bool = False,
                          set_pin_direction_api: bool = True,
                          set_pin_mode_api: bool = True) -> "PortConfigGenerator":
        """添加通用配置"""
        container = EcucContainerValue(
            short_name="PortGeneral",
            definition_ref=self._create_def_ref("PortGeneral")
        )
        
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("PortGeneral/PortDevErrorDetect", "BOOLEAN"),
            dev_error_detect
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("PortGeneral/PortVersionInfoApi", "BOOLEAN"),
            version_info_api
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("PortGeneral/PortSetPinDirectionApi", "BOOLEAN"),
            set_pin_direction_api
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("PortGeneral/PortSetPinModeApi", "BOOLEAN"),
            set_pin_mode_api
        ))
        
        self.module_config.add_container(container)
        return self
    
    def add_pin_config(self, pin_name: str,
                      port_name: str = "PA",
                      pin_number: int = 0,
                      direction: str = "PORT_PIN_OUT",
                      mode: str = "PORT_PIN_MODE_GPIO",
                      initial_level: str = "PORT_PIN_LEVEL_LOW") -> "PortConfigGenerator":
        """
        添加单个引脚配置
        
        Args:
            pin_name: 引脚配置名称
            port_name: 端口名称 (PA, PB, PC, 等)
            pin_number: 引脚号 (0-15)
            direction: 方向 (PORT_PIN_IN, PORT_PIN_OUT)
            mode: 模式 (PORT_PIN_MODE_GPIO, PORT_PIN_MODE_CAN, 等)
            initial_level: 初始电平
        """
        # 查找或创建PortContainer
        container_name = f"PortContainer_{port_name}"
        existing = [c for c in self.module_config.containers if c.short_name == container_name]
        
        if existing:
            port_container = existing[0]
        else:
            port_container = EcucContainerValue(
                short_name=container_name,
                definition_ref=self._create_def_ref("PortContainer")
            )
            port_container.add_parameter(EcucStringParamValue(
                self._create_param_def_ref("PortContainer/PortContainerName", "STRING"),
                port_name
            ))
            self.module_config.add_container(port_container)
        
        # 创建引脚配置
        pin_config = EcucContainerValue(
            short_name=pin_name,
            definition_ref=self._create_def_ref("PortPin")
        )
        
        pin_config.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("PortPin/PortPinId", "INTEGER"),
            pin_number
        ))
        pin_config.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("PortPin/PortPinDirection", "ENUM"),
            direction
        ))
        pin_config.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("PortPin/PortPinInitialMode", "ENUM"),
            mode
        ))
        pin_config.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("PortPin/PortPinLevelValue", "ENUM"),
            initial_level
        ))
        
        port_container.add_sub_container(pin_config)
        return self


class CanConfigGenerator(MCALConfigGenerator):
    """
    CAN驱动配置生成器
    支持波特率、控制器、硬件对象配置
    """
    
    def __init__(self, ecu_name: str = "ECU0"):
        super().__init__("Can", ecu_name)
    
    def add_general_config(self,
                          dev_error_detect: bool = True,
                          index: int = 0,
                          main_function_period: float = 10.0,
                          multiplexed_tx: bool = True,
                          timeout_duration: float = 0.1) -> "CanConfigGenerator":
        """添加通用配置"""
        container = EcucContainerValue(
            short_name="CanGeneral",
            definition_ref=self._create_def_ref("CanGeneral")
        )
        
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("CanGeneral/CanDevErrorDetect", "BOOLEAN"),
            dev_error_detect
        ))
        container.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("CanGeneral/CanIndex", "INTEGER"),
            index
        ))
        container.add_parameter(EcucFloatParamValue(
            self._create_param_def_ref("CanGeneral/CanMainFunctionPeriod", "FLOAT"),
            main_function_period
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("CanGeneral/CanMultiplexedTransmission", "BOOLEAN"),
            multiplexed_tx
        ))
        container.add_parameter(EcucFloatParamValue(
            self._create_param_def_ref("CanGeneral/CanTimeoutDuration", "FLOAT"),
            timeout_duration
        ))
        
        self.module_config.add_container(container)
        return self
    
    def add_controller_config(self,
                            controller_id: int = 0,
                            baudrate: int = 500000,
                            prop_seg: int = 2,
                            phase_seg1: int = 6,
                            phase_seg2: int = 7,
                            sync_jump_width: int = 1) -> "CanConfigGenerator":
        """
        添加CAN控制器配置
        
        Args:
            controller_id: 控制器ID
            baudrate: 波特率 (bps)
            prop_seg: 传播段
            phase_seg1: 相位段1
            phase_seg2: 相位段2
            sync_jump_width: 同步跳跃宽度
        """
        config_set = EcucContainerValue(
            short_name="CanConfigSet",
            definition_ref=self._create_def_ref("CanConfigSet")
        )
        
        controller = EcucContainerValue(
            short_name=f"CanController_{controller_id}",
            definition_ref=self._create_def_ref("CanController")
        )
        
        controller.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("CanController/CanControllerId", "INTEGER"),
            controller_id
        ))
        controller.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("CanController/CanBusOffProcessing", "ENUM"),
            "INTERRUPT"
        ))
        controller.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("CanController/CanRxProcessing", "ENUM"),
            "INTERRUPT"
        ))
        controller.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("CanController/CanTxProcessing", "ENUM"),
            "INTERRUPT"
        ))
        controller.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("CanController/CanWakeupProcessing", "ENUM"),
            "INTERRUPT"
        ))
        
        # 波特率配置
        baudrate_config = EcucContainerValue(
            short_name="CanControllerBaudrateConfig",
            definition_ref=self._create_def_ref("CanControllerBaudrateConfig")
        )
        baudrate_config.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("CanControllerBaudrateConfig/CanControllerBaudRate", "INTEGER"),
            baudrate
        ))
        baudrate_config.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("CanControllerBaudrateConfig/CanControllerPropSeg", "INTEGER"),
            prop_seg
        ))
        baudrate_config.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("CanControllerBaudrateConfig/CanControllerSeg1", "INTEGER"),
            phase_seg1
        ))
        baudrate_config.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("CanControllerBaudrateConfig/CanControllerSeg2", "INTEGER"),
            phase_seg2
        ))
        baudrate_config.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("CanControllerBaudrateConfig/CanControllerSyncJumpWidth", "INTEGER"),
            sync_jump_width
        ))
        
        controller.add_sub_container(baudrate_config)
        config_set.add_sub_container(controller)
        self.module_config.add_container(config_set)
        
        return self
    
    def add_hardware_object(self,
                          hoh_name: str,
                          controller_ref: int = 0,
                          object_type: str = "TRANSMIT",
                          id_type: str = "STANDARD") -> "CanConfigGenerator":
        """
        添加CAN硬件对象配置 (HOH)
        
        Args:
            hoh_name: 硬件对象名称
            controller_ref: 控制器参考
            object_type: 类型 (TRANSMIT, RECEIVE)
            id_type: ID类型 (STANDARD, EXTENDED)
        """
        config_set = [c for c in self.module_config.containers if c.short_name == "CanConfigSet"]
        if not config_set:
            config_set = EcucContainerValue(
                short_name="CanConfigSet",
                definition_ref=self._create_def_ref("CanConfigSet")
            )
            self.module_config.add_container(config_set)
        else:
            config_set = config_set[0]
        
        hoh = EcucContainerValue(
            short_name=hoh_name,
            definition_ref=self._create_def_ref("CanHardwareObject")
        )
        
        hoh.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("CanHardwareObject/CanHandleType", "ENUM"),
            "FULL" if object_type == "TRANSMIT" else "BASIC"
        ))
        hoh.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("CanHardwareObject/CanObjectType", "ENUM"),
            object_type
        ))
        hoh.add_parameter(EcucEnumParamValue(
            self._create_param_def_ref("CanHardwareObject/CanIdType", "ENUM"),
            id_type
        ))
        
        config_set.add_sub_container(hoh)
        return self


class SpiConfigGenerator(MCALConfigGenerator):
    """
    SPI驱动配置生成器
    支持Channel、Job、Sequence配置
    """
    
    def __init__(self, ecu_name: str = "ECU0"):
        super().__init__("Spi", ecu_name)
    
    def add_general_config(self,
                          dev_error_detect: bool = True,
                          async_mode: bool = False,
                          level_delivered: int = 2) -> "SpiConfigGenerator":
        """添加通用配置"""
        container = EcucContainerValue(
            short_name="SpiGeneral",
            definition_ref=self._create_def_ref("SpiGeneral")
        )
        
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("SpiGeneral/SpiDevErrorDetect", "BOOLEAN"),
            dev_error_detect
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("SpiGeneral/SpiAsyncParallelTransmit", "BOOLEAN"),
            async_mode
        ))
        container.add_parameter(EcucIntegerParamValue(
            self._create_param_def_ref("SpiGeneral/SpiLevelDelivered", "INTEGER"),
            level_delivered
        ))
        
        self.module_config.add_container(container)
        return self


class GptConfigGenerator(MCALConfigGenerator):
    """
    GPT通用定时器配置生成器
    """
    
    def __init__(self, ecu_name: str = "ECU0"):
        super().__init__("Gpt", ecu_name)
    
    def add_general_config(self,
                          dev_error_detect: bool = True,
                          main_function_period: float = 10.0,
                          deinit_api: bool = True,
                          time_elapsed_api: bool = True) -> "GptConfigGenerator":
        """添加通用配置"""
        container = EcucContainerValue(
            short_name="GptGeneral",
            definition_ref=self._create_def_ref("GptGeneral")
        )
        
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("GptGeneral/GptDevErrorDetect", "BOOLEAN"),
            dev_error_detect
        ))
        container.add_parameter(EcucFloatParamValue(
            self._create_param_def_ref("GptGeneral/GptMainFunctionPeriod", "FLOAT"),
            main_function_period
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("GptGeneral/GptDeinitApi", "BOOLEAN"),
            deinit_api
        ))
        container.add_parameter(EcucBooleanParamValue(
            self._create_param_def_ref("GptGeneral/GptTimeElapsedApi", "BOOLEAN"),
            time_elapsed_api
        ))
        
        self.module_config.add_container(container)
        return self


# 工厂函数

def create_mcu_config(ecu_name: str = "ECU0") -> McuConfigGenerator:
    """创建MCU配置生成器"""
    return McuConfigGenerator(ecu_name)


def create_port_config(ecu_name: str = "ECU0") -> PortConfigGenerator:
    """创建Port配置生成器"""
    return PortConfigGenerator(ecu_name)


def create_can_config(ecu_name: str = "ECU0") -> CanConfigGenerator:
    """创建CAN配置生成器"""
    return CanConfigGenerator(ecu_name)


def create_spi_config(ecu_name: str = "ECU0") -> SpiConfigGenerator:
    """创建SPI配置生成器"""
    return SpiConfigGenerator(ecu_name)


def create_gpt_config(ecu_name: str = "ECU0") -> GptConfigGenerator:
    """创建GPT配置生成器"""
    return GptConfigGenerator(ecu_name)
