#!/usr/bin/env python3
"""Expanded tests for MCAL and BSW configuration generators"""

import sys
import os
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from ecuc_config_model import (
    EcucModuleConfigurationValues, EcucContainerValue,
    EcucBooleanParamValue, EcucIntegerParamValue, EcucFloatParamValue,
    EcucStringParamValue, EcucEnumParamValue,
    EcucDefinitionRef, EcucIndex, EcucReferenceValue,
    EcucInstanceReferenceValue, EcucSymbolicNameReferenceValue,
    EcucChoiceReferenceValue,
    create_module_config, create_container, create_boolean_param,
    create_integer_param, create_float_param, create_string_param,
    create_enum_param, create_function_name_param, create_reference_value,
    EcucFunctionNameParamValue, EcucLinkerSymbolParamValue, EcucMacroParamValue,
)
from arxml_ecuc_generator import ArxmlEcucGenerator
from mcal_config_generator import (
    create_mcu_config, create_port_config, create_can_config,
    create_spi_config, create_gpt_config,
    McuConfigGenerator, PortConfigGenerator, CanConfigGenerator,
    SpiConfigGenerator, GptConfigGenerator,
)
from bsw_config_generator import (
    create_com_config, create_pdur_config, create_canif_config, create_nvm_config,
    ComConfigGenerator, PduRConfigGenerator, CanIfConfigGenerator, NvMConfigGenerator,
)


# ============================================================================
# ECUC Config Model Advanced Tests
# ============================================================================

class TestEcucConfigModelAdvanced(unittest.TestCase):
    """Advanced ECUC model tests"""

    def test_ecuc_query_find_container(self):
        """Test EcucQuery find_container"""
        from ecuc_config_model import EcucQuery
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        c1 = create_container("/AUTOSAR/EcucDefs/Test/TestContainer", "Config",
                              "ECUC-CONTAINER-DEF")
        c2 = create_container("/AUTOSAR/EcucDefs/Test/SubConfig", "SubConfig",
                              "ECUC-CONTAINER-DEF")
        c1.add_sub_container(c2)
        module.add_container(c1)

        query = EcucQuery(module)
        result = query.find_container("Config")
        self.assertIsNotNone(result)
        self.assertEqual(result.short_name, "Config")

        # Non-existent
        result = query.find_container("Nonexistent")
        self.assertIsNone(result)

    def test_ecuc_query_find_parameter(self):
        """Test EcucQuery find_parameter"""
        from ecuc_config_model import EcucQuery
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        c1 = create_container("/AUTOSAR/EcucDefs/Test/Config", "Config",
                              "ECUC-CONTAINER-DEF")
        param = create_boolean_param("/AUTOSAR/EcucDefs/Test/Config/Enable", True)
        c1.add_parameter(param)
        module.add_container(c1)

        query = EcucQuery(module)
        found = query.find_parameter("Config",
                                      "/AUTOSAR/EcucDefs/Test/Config/Enable")
        self.assertIsNotNone(found)
        self.assertTrue(found.value)

    def test_ecuc_query_empty(self):
        """Test EcucQuery with no module config"""
        from ecuc_config_model import EcucQuery
        query = EcucQuery()
        self.assertIsNone(query.find_container("anything"))

    def test_get_containers_by_def(self):
        """Test get_containers_by_def"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        c1 = create_container("/AUTOSAR/EcucDefs/Test/Config", "Cfg1",
                              "ECUC-CONTAINER-DEF")
        c2 = create_container("/AUTOSAR/EcucDefs/Test/Config", "Cfg2",
                              "ECUC-CONTAINER-DEF")
        module.add_container(c1)
        module.add_container(c2)
        containers = module.get_containers_by_def("/AUTOSAR/EcucDefs/Test/Config")
        self.assertEqual(len(containers), 2)

    def test_get_parameter_by_def(self):
        """Test container get_parameter_by_def"""
        c = create_container("/AUTOSAR/EcucDefs/Test/Config", "Config",
                             "ECUC-CONTAINER-DEF")
        p1 = create_integer_param("/AUTOSAR/EcucDefs/Test/Config/Param1", 10)
        p2 = create_boolean_param("/AUTOSAR/EcucDefs/Test/Config/Param2", True)
        c.add_parameter(p1)
        c.add_parameter(p2)
        self.assertIsNotNone(c.get_parameter_by_def("/AUTOSAR/EcucDefs/Test/Config/Param1"))
        self.assertIsNone(c.get_parameter_by_def("/nonexistent"))

    def test_get_sub_container_by_name(self):
        """Test container get_sub_container_by_name"""
        parent = create_container("/AUTOSAR/EcucDefs/Test/Parent", "Parent",
                                  "ECUC-CONTAINER-DEF")
        child = create_container("/AUTOSAR/EcucDefs/Test/Child", "Child",
                                 "ECUC-CONTAINER-DEF")
        parent.add_sub_container(child)
        self.assertIsNotNone(parent.get_sub_container_by_name("Child"))
        self.assertIsNone(parent.get_sub_container_by_name("Nonexistent"))

    def test_ecuc_index(self):
        """Test EcucIndex"""
        idx = EcucIndex(5)
        self.assertEqual(str(idx), "5")

    def test_definition_ref_str(self):
        """Test EcucDefinitionRef string conversion"""
        ref = EcucDefinitionRef(dest="ECUC-BOOLEAN-PARAM-DEF",
                                value="/AUTOSAR/EcucDefs/Test/Param")
        self.assertEqual(str(ref), "/AUTOSAR/EcucDefs/Test/Param")

    def test_reference_value_types(self):
        """Test all reference value types"""
        def_ref = EcucDefinitionRef("ECUC-REFERENCE-DEF", "/AUTOSAR/Test/Ref")

        # Reference value
        ref = EcucReferenceValue(def_ref, "/Target/Path")
        self.assertEqual(ref.tag_name, "ECUC-REFERENCE-VALUE")
        self.assertEqual(ref.value_ref, "/Target/Path")

        # Instance reference
        instance_ref = EcucInstanceReferenceValue(def_ref, "/Context", "/Target")
        self.assertEqual(instance_ref.tag_name, "ECUC-INSTANCE-REFERENCE-VALUE")
        self.assertEqual(instance_ref.context_ref, "/Context")
        self.assertEqual(instance_ref.target_ref, "/Target")

        # Symbolic name reference
        sym_ref = EcucSymbolicNameReferenceValue(def_ref, "/Symbolic/Target")
        self.assertEqual(sym_ref.tag_name, "ECUC-SYMBOLIC-NAME-REFERENCE-VALUE")

        # Choice reference
        choice_ref = EcucChoiceReferenceValue(def_ref, "/Choice/Target")
        self.assertEqual(choice_ref.tag_name, "ECUC-CHOICE-REFERENCE-VALUE")

    def test_param_value_types_all(self):
        """Test all parameter value types"""
        bool_ref = EcucDefinitionRef("ECUC-BOOLEAN-PARAM-DEF", "/Bool")
        int_ref = EcucDefinitionRef("ECUC-INTEGER-PARAM-DEF", "/Int")
        float_ref = EcucDefinitionRef("ECUC-FLOAT-PARAM-DEF", "/Float")
        str_ref = EcucDefinitionRef("ECUC-STRING-PARAM-DEF", "/Str")
        enum_ref = EcucDefinitionRef("ECUC-ENUMERATION-PARAM-DEF", "/Enum")
        func_ref = EcucDefinitionRef("ECUC-FUNCTION-NAME-DEF", "/Func")
        linker_ref = EcucDefinitionRef("ECUC-LINKER-SYMBOL-DEF", "/Linker")
        macro_ref = EcucDefinitionRef("ECUC-MACRO-DEF", "/Macro")

        b = EcucBooleanParamValue(bool_ref, True)
        self.assertEqual(b.get_value_str(), "true")
        self.assertEqual(b.tag_name, "ECUC-NUMERICAL-PARAM-VALUE")

        i = EcucIntegerParamValue(int_ref, 42)
        self.assertEqual(i.get_value_str(), "42")

        f = EcucFloatParamValue(float_ref, 3.14)
        self.assertIn("3.14", f.get_value_str())

        s = EcucStringParamValue(str_ref, "hello")
        self.assertEqual(s.get_value_str(), "hello")
        self.assertEqual(s.tag_name, "ECUC-TEXTUAL-PARAM-VALUE")

        e = EcucEnumParamValue(enum_ref, "OPTION_1")
        self.assertEqual(e.get_value_str(), "OPTION_1")

        fn = EcucFunctionNameParamValue(func_ref, "Callback")
        self.assertEqual(fn.get_value_str(), "Callback")
        self.assertEqual(fn.tag_name, "ECUC-FUNCTION-NAME-DEF")

        ls = EcucLinkerSymbolParamValue(linker_ref, "MySymbol")
        self.assertEqual(ls.get_value_str(), "MySymbol")
        self.assertEqual(ls.tag_name, "ECUC-LINKER-SYMBOL-DEF")

        mp = EcucMacroParamValue(macro_ref, "MY_MACRO")
        self.assertEqual(mp.get_value_str(), "MY_MACRO")
        self.assertEqual(mp.tag_name, "ECUC-MACRO-NAME-DEF")

    def test_to_dict_conversion(self):
        """Test to_dict via base_model"""
        from base_model import EcucConfigModel, EcucContainer, EcucParameter, EcucReference
        # We test through ArxmlEcucGenerator which should handle properly
        # Create a module and verify structure serialization works
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        c = create_container("/AUTOSAR/EcucDefs/Test/Cfg", "Cfg", "ECUC-CONTAINER-DEF")
        c.add_parameter(create_boolean_param("/AUTOSAR/EcucDefs/Test/Cfg/Enable", True))
        c.add_parameter(create_integer_param("/AUTOSAR/EcucDefs/Test/Cfg/Count", 5))
        module.add_container(c)

        gen = ArxmlEcucGenerator()
        gen.register_module(module)
        xml = gen.to_string(pretty=True)
        self.assertIn("ECUC-MODULE-CONFIGURATION-VALUES", xml)
        self.assertIn("true", xml)
        self.assertIn("5", xml)

    def test_multiple_modules_generated(self):
        """Test generating multiple modules in one XML"""
        mcu = create_module_config("Mcu", "/AUTOSAR/EcucDefs/Mcu", "MCAL")
        port = create_module_config("Port", "/AUTOSAR/EcucDefs/Port", "MCAL")

        gen = ArxmlEcucGenerator()
        gen.register_module(mcu)
        gen.register_module(port)
        xml = gen.to_string()
        self.assertIn("Mcu", xml)
        self.assertIn("Port", xml)
        self.assertEqual(len(gen.generated_modules), 2)

    def test_clear_modules(self):
        """Test clearing modules"""
        gen = ArxmlEcucGenerator()
        m = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        gen.register_module(m)
        self.assertEqual(len(gen.generated_modules), 1)
        gen.clear()
        self.assertEqual(len(gen.generated_modules), 0)

    def test_admin_data_create(self):
        """Test creating generator with admin data"""
        gen = ArxmlEcucGenerator(company="TestCo", author="Tester")
        self.assertEqual(gen.company, "TestCo")
        self.assertEqual(gen.author, "Tester")


# ============================================================================
# MCAL Generator Expanded Tests
# ============================================================================

class TestMcalGenerators(unittest.TestCase):
    """Test all MCAL generators"""

    def test_mcu_full_pipeline(self):
        """Test full MCU generation pipeline"""
        gen = create_mcu_config("ECU1")
        gen.add_general_config(dev_error_detect=True, version_info_api=True)
        gen.add_clock_config(cpu_clock=160000000, peripheral_clock=80000000)
        gen.add_mode_config()
        gen.add_ram_section(size=131072, address=0x20000000)
        xml = gen.to_arxml()
        self.assertIn("Mcu", xml)
        self.assertIn("McuGeneral", xml)
        self.assertIn("McuClockSettingConfig_0", xml)
        self.assertIn("McuModeSettingConf_0", xml)

    def test_mcu_without_mode_config(self):
        """Test MCU config without explicit mode"""
        gen = create_mcu_config("ECU0")
        gen.add_general_config()
        gen.add_clock_config()
        xml = gen.to_arxml()
        self.assertIn("McuGeneral", xml)

    def test_port_multiple_pins(self):
        """Test Port config with multiple pins"""
        gen = create_port_config("ECU0")
        gen.add_general_config(set_pin_direction_api=True, set_pin_mode_api=True)
        gen.add_pin_config("Pin_PA0", "PA", 0, "PORT_PIN_OUT", "PORT_PIN_MODE_GPIO")
        gen.add_pin_config("Pin_PA1", "PA", 1, "PORT_PIN_IN", "PORT_PIN_MODE_GPIO")
        gen.add_pin_config("Pin_PB5", "PB", 5, "PORT_PIN_OUT", "PORT_PIN_MODE_CAN")
        xml = gen.to_arxml()
        self.assertIn("PortContainer_PA", xml)
        self.assertIn("PortContainer_PB", xml)
        self.assertIn("PORT_PIN_OUT", xml)
        self.assertIn("PORT_PIN_IN", xml)

    def test_can_full_config(self):
        """Test CAN config with all features"""
        gen = create_can_config("ECU0")
        gen.add_general_config(multiplexed_tx=True, timeout_duration=0.5)
        gen.add_controller_config(controller_id=0, baudrate=500000)
        gen.add_hardware_object("CanHoh_Tx0", controller_ref=0, object_type="TRANSMIT")
        gen.add_hardware_object("CanHoh_Rx0", controller_ref=0, object_type="RECEIVE")
        xml = gen.to_arxml()
        self.assertIn("CanGeneral", xml)
        self.assertIn("CanController_0", xml)
        self.assertIn("BaudRate", xml)
        self.assertIn("CanHoh_Tx0", xml)

    def test_spi_config(self):
        """Test SPI config"""
        gen = create_spi_config("ECU0")
        gen.add_general_config(async_mode=True, level_delivered=3)
        xml = gen.to_arxml()
        self.assertIn("SpiGeneral", xml)

    def test_gpt_config(self):
        """Test GPT config"""
        gen = create_gpt_config("ECU0")
        gen.add_general_config(main_function_period=5.0, deinit_api=True)
        xml = gen.to_arxml()
        self.assertIn("GptGeneral", xml)

    def test_mcu_config_get_module(self):
        """Test get_module_config accessor"""
        gen = create_mcu_config("ECU0")
        mod = gen.get_module_config()
        self.assertIsNotNone(mod)
        self.assertEqual(mod.short_name, "Mcu")


# ============================================================================
# BSW Generator Expanded Tests
# ============================================================================

class TestBswGenerators(unittest.TestCase):
    """Test all BSW generators"""

    def test_com_full_config(self):
        """Test full COM configuration pipeline"""
        gen = create_com_config("ECU0")
        gen.add_general_config()
        gen.add_ipdu_config("ComIPdu_0", pdu_id=0, length=8, direction="SEND")
        gen.add_signal_config("Signal_0", ipdu_ref="ComIPdu_0", start_bit=0, bit_length=8)
        xml = gen.to_arxml()
        self.assertIn("Com", xml)
        self.assertIn("ComGeneral", xml)

    def test_pdur_full_config(self):
        """Test PduR full configuration"""
        gen = create_pdur_config("ECU0")
        gen.add_general_config(dev_error_detect=True, retry_transmit=True, meta_data_support=True)
        gen.add_routing_path("Route1", src_module="COM", dest_module="CANIF", pdu_id=0)
        xml = gen.to_arxml()
        self.assertIn("PduR", xml)
        self.assertIn("PduRGeneral", xml)

    def test_pdur_routing_tables(self):
        """Test PduR routing tables with multiple routes"""
        gen = create_pdur_config("ECU0")
        gen.add_general_config()
        gen.add_routing_path("Route_COM_CANIF", "COM", "CANIF", 1)
        gen.add_routing_path("Route_CANTP_COM", "CANTP", "COM", 2)
        xml = gen.to_arxml()
        self.assertIn("PduRRoutingTables", xml)

    def test_canif_full_config(self):
        """Test CanIf full configuration"""
        gen = create_canif_config("ECU0")
        gen.add_general_config(dev_error_detect=True, software_filter=True)
        gen.add_controller_config(controller_id=0, controller_ref="CanController_0")
        gen.add_hrh_config("CanIfHrh_0", controller_id=0)
        xml = gen.to_arxml()
        self.assertIn("CanIf", xml)
        self.assertIn("CanIfGeneral", xml)

    def test_canif_hrh_config(self):
        """Test CanIf HRH with software filter"""
        gen = create_canif_config("ECU0")
        gen.add_general_config()
        gen.add_controller_config(0)
        gen.add_hrh_config("HRH_Sensor", controller_id=0, hoh_ref="CanHoh_0")
        xml = gen.to_arxml()
        self.assertIn("HRH_Sensor", xml)
        self.assertIn("CanIfHrhCfg", xml)

    def test_nvm_full_config(self):
        """Test NvM full configuration"""
        gen = create_nvm_config("ECU0")
        gen.add_common_config(crc_num_bytes=4, main_function_period=20.0)
        gen.add_block_descriptor("NvMBlock_0", block_id=0, block_size=64,
                                 crc_type="NVM_CRC32")
        gen.add_block_descriptor("NvMBlock_1", block_id=1, block_size=128,
                                 crc_type="NVM_CRC16")
        xml = gen.to_arxml()
        self.assertIn("NvM", xml)
        self.assertIn("NvMCommon", xml)
        self.assertIn("NvMBlockDescriptor", xml)

    def test_nvm_default_crc(self):
        """Test NvM with default CRC (no CRC)"""
        gen = create_nvm_config("ECU0")
        gen.add_common_config(crc_num_bytes=0)
        gen.add_block_descriptor("NvMBlock_0", block_id=0, block_size=32,
                                 crc_type="NVM_CRC_NONE")
        xml = gen.to_arxml()
        self.assertIn("NVM_CRC_NONE", xml)

    def test_com_no_signals(self):
        """Test COM with just general config"""
        gen = create_com_config("ECU0")
        gen.add_general_config()
        xml = gen.to_arxml()
        self.assertIn("Com", xml)

    def test_generator_module_config_access(self):
        """Test that generators correctly wrap module config"""
        gen = create_com_config("ECU0")
        mc = gen.get_module_config()
        self.assertIsInstance(mc, EcucModuleConfigurationValues)
        self.assertEqual(mc.short_name, "Com")

    def test_com_general_only(self):
        """Test Com with only general config"""
        gen = ComConfigGenerator("ECU0")
        gen.add_general_config(dev_error_detect=True, enable_update_bit_check=True)
        xml = gen.to_arxml()
        self.assertIn("ComGeneral", xml)
        self.assertIn("true", xml)

    def test_com_signal_with_init_value(self):
        """Test Com signal with custom init value"""
        gen = ComConfigGenerator("ECU0")
        gen.add_general_config()
        gen.add_ipdu_config("IPDU_Data", 0, 8, "SEND")
        gen.add_signal_config("SpeedSignal", "IPDU_Data", start_bit=0, bit_length=16, init_value=42)
        xml = gen.to_arxml()
        self.assertIn("SpeedSignal", xml)

    def test_nvm_block_with_priority(self):
        """Test NvM block with priority settings"""
        gen = NvMConfigGenerator("ECU0")
        gen.add_common_config()
        gen.add_block_descriptor("HighPriorityBlock", block_id=0, block_size=32,
                                 job_priority=1)
        gen.add_block_descriptor("LowPriorityBlock", block_id=1, block_size=16,
                                 job_priority=2, write_retries=5)
        xml = gen.to_arxml()
        self.assertIn("HighPriorityBlock", xml)
        self.assertIn("LowPriorityBlock", xml)


if __name__ == '__main__':
    unittest.main()
#!/usr/bin/env python3
"""Additional tests for arxml_ecuc_generator edge cases"""

import sys
import os
import unittest
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from ecuc_config_model import (
    EcucModuleConfigurationValues, EcucContainerValue,
    EcucBooleanParamValue, EcucIntegerParamValue, EcucFloatParamValue,
    EcucStringParamValue, EcucEnumParamValue, EcucFunctionNameParamValue,
    EcucLinkerSymbolParamValue, EcucMacroParamValue,
    EcucDefinitionRef, EcucIndex, EcucReferenceValue,
    EcucInstanceReferenceValue, EcucSymbolicNameReferenceValue,
    EcucChoiceReferenceValue,
    create_module_config, create_container, create_boolean_param,
    create_integer_param, create_float_param, create_string_param,
    create_enum_param, create_function_name_param, create_reference_value,
    EcucQuery,
)
from arxml_ecuc_generator import ArxmlEcucGenerator
from mcal_config_generator import create_mcu_config, create_port_config, create_can_config
from bsw_config_generator import create_com_config, create_pdur_config, create_canif_config, create_nvm_config
from base_model import EcucConfigModel, EcucContainer, EcucParameter, EcucReference, EcucConfigStatus


class TestArxmlEcucGeneratorAdvanced(unittest.TestCase):
    """Advanced tests for ArxmlEcucGenerator"""

    def setUp(self):
        self.generator = ArxmlEcucGenerator(company="TestCo", author="Tester")

    def test_to_string_no_pretty(self):
        """Test non-pretty string output"""
        module = create_module_config("Mcu", "/AUTOSAR/EcucDefs/Mcu")
        self.generator.register_module(module)
        xml_str = self.generator.to_string(pretty=False)
        self.assertIn("AUTOSAR", xml_str)

    def test_save_and_clear(self):
        """Test save() and clear() methods"""
        module = create_module_config("Port", "/AUTOSAR/EcucDefs/Port", "MCAL")
        self.generator.register_module(module)
        self.assertEqual(len(self.generator.generated_modules), 1)

        with tempfile.NamedTemporaryFile(suffix='.arxml', delete=False) as f:
            temp_path = f.name
        try:
            self.generator.save(temp_path)
            self.assertTrue(os.path.exists(temp_path))
            with open(temp_path) as f:
                content = f.read()
                self.assertIn("Port", content)

            self.generator.clear()
            self.assertEqual(len(self.generator.generated_modules), 0)

            # Save again after clear (should generate empty XML with no modules)
            self.generator.save(temp_path)
            with open(temp_path) as f:
                content = f.read()
                # Should still be valid XML
                self.assertIn("AUTOSAR", content)
        finally:
            if os.path.exists(temp_path):
                os.unlink(temp_path)

    def test_register_module_returns_self(self):
        """Test register_module returns self for chaining"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        result = self.generator.register_module(module)
        self.assertIs(result, self.generator)

    def test_generate_with_instance_reference(self):
        """Test generating XML with instance reference"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        def_ref = EcucDefinitionRef("ECUC-INSTANCE-REFERENCE-DEF", "/Test/Ref")
        container = create_container("/Test/Cfg", "Config", "ECUC-CONTAINER-DEF")
        instance_ref = EcucInstanceReferenceValue(
            definition_ref=def_ref,
            context_ref="/Context/Path",
            target_ref="/Target/Path"
        )
        container.add_reference(instance_ref)
        module.add_container(container)
        self.generator.register_module(module)
        xml = self.generator.to_string()
        self.assertIn("ECUC-INSTANCE-REFERENCE-VALUE", xml)
        self.assertIn("CONTEXT-REF", xml)
        self.assertIn("TARGET-REF", xml)

    def test_generate_with_choice_reference(self):
        """Test generating XML with choice reference"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        def_ref = EcucDefinitionRef("ECUC-CHOICE-REFERENCE-DEF", "/Test/ChoiceRef")
        container = create_container("/Test/Cfg", "Config", "ECUC-CONTAINER-DEF")
        choice_ref = EcucChoiceReferenceValue(
            definition_ref=def_ref,
            value_ref="/Choice/Target"
        )
        container.add_reference(choice_ref)
        module.add_container(container)
        self.generator.register_module(module)
        xml = self.generator.to_string()
        self.assertIn("ECUC-CHOICE-REFERENCE-VALUE", xml)

    def test_generate_with_symbolic_name_reference(self):
        """Test generating XML with symbolic name reference"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        def_ref = EcucDefinitionRef("ECUC-SYMBOLIC-NAME-REFERENCE-DEF", "/Test/SymRef")
        container = create_container("/Test/Cfg", "Config", "ECUC-CONTAINER-DEF")
        sym_ref = EcucSymbolicNameReferenceValue(
            definition_ref=def_ref,
            value_ref="/Symbolic/Target"
        )
        container.add_reference(sym_ref)
        module.add_container(container)
        self.generator.register_module(module)
        xml = self.generator.to_string()
        self.assertIn("ECUC-SYMBOLIC-NAME-REFERENCE-VALUE", xml)

    def test_generate_multiple_packages(self):
        """Test generating modules in different packages"""
        mcal_module = create_module_config("Mcu", "/AUTOSAR/EcucDefs/Mcu", "MCAL")
        bsw_module = create_module_config("Com", "/AUTOSAR/EcucDefs/Com", "BSW")
        self.generator.register_module(mcal_module)
        self.generator.register_module(bsw_module)
        xml = self.generator.to_string()
        self.assertIn("MCAL", xml)
        self.assertIn("BSW", xml)

    def test_generate_with_linker_symbol_and_macro(self):
        """Test generating XML with linker symbol and macro params"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        container = create_container("/Test/Cfg", "Config", "ECUC-CONTAINER-DEF")

        linker_def = EcucDefinitionRef("ECUC-LINKER-SYMBOL-DEF", "/Test/Linker")
        linker = EcucLinkerSymbolParamValue(linker_def, "MySymbol")
        container.add_parameter(linker)

        macro_def = EcucDefinitionRef("ECUC-MACRO-DEF", "/Test/Macro")
        macro = EcucMacroParamValue(macro_def, "MY_DEFINE")
        container.add_parameter(macro)

        module.add_container(container)
        self.generator.register_module(module)
        xml = self.generator.to_string()
        self.assertIn("ECUC-LINKER-SYMBOL-DEF", xml)
        self.assertIn("ECUC-MACRO-NAME-DEF", xml)

    def test_generate_with_index_and_description(self):
        """Test generating with index and description"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test",
                                       "ECUC", config_variant="VARIANT-POST-BUILD")
        module.module_description = "Test module description"
        container = create_container("/Test/Cfg", "Config", "ECUC-CONTAINER-DEF")
        container.index = EcucIndex(3)
        bool_def = EcucDefinitionRef("ECUC-BOOLEAN-PARAM-DEF", "/Test/Bool")
        bool_val = EcucBooleanParamValue(bool_def, True)
        bool_val.index = EcucIndex(5)
        container.add_parameter(bool_val)
        module.add_container(container)
        self.generator.register_module(module)
        xml = self.generator.to_string()
        self.assertIn("VARIANT-POST-BUILD", xml)
        self.assertIn("INDEX", xml)
        self.assertIn("MODULE-DESCRIPTION", xml)
        self.assertIn("Test module description", xml)

    def test_generate_full_bsw_pipeline(self):
        """Test full BSW generation pipeline"""
        nvm = create_nvm_config("ECU0")
        nvm.add_common_config(crc_num_bytes=4)
        nvm.add_block_descriptor("Blk0", 0, 64, "NVM_CRC32")

        pdur = create_pdur_config("ECU0")
        pdur.add_general_config()
        pdur.add_routing_path("RouteToCan", "COM", "CANIF", 0)

        self.generator.register_module(nvm.get_module_config())
        self.generator.register_module(pdur.get_module_config())
        xml = self.generator.to_string()
        self.assertIn("NvM", xml)
        self.assertIn("PduR", xml)
        self.assertIn("PduRRoutingTables", xml)


class TestBaseModel(unittest.TestCase):
    """Tests for base_model module"""

    def test_base_model_abstract(self):
        """Test that base model is abstract"""
        # Can't instantiate abstract class directly
        pass

    def test_ecuc_config_status(self):
        """Test EcucConfigStatus enum"""
        import base_model as bm
        self.assertIsInstance(bm.EcucConfigStatus.UNCONFIGURED, bm.EcucConfigStatus)
        self.assertEqual(bm.EcucConfigStatus.CONFIGURED.name, "CONFIGURED")

    def test_ecuc_parameter_to_dict(self):
        """Test EcucParameter to_dict"""
        import base_model as bm
        param = bm.EcucParameter(name="TestParam", value=42, param_type="INTEGER",
                                  description="Test description")
        d = param.to_dict()
        self.assertEqual(d["name"], "TestParam")
        self.assertEqual(d["value"], 42)
        self.assertEqual(d["type"], "INTEGER")
        self.assertEqual(d["description"], "Test description")

    def test_ecuc_reference_to_dict(self):
        """Test EcucReference to_dict"""
        import base_model as bm
        ref = bm.EcucReference(name="TestRef", target="/Target/Path",
                                ref_type="REFERENCE", description="Ref desc")
        d = ref.to_dict()
        self.assertEqual(d["name"], "TestRef")
        self.assertEqual(d["target"], "/Target/Path")
        self.assertEqual(d["type"], "REFERENCE")

    def test_ecuc_container_add_methods(self):
        """Test EcucContainer add methods"""
        import base_model as bm
        container = bm.EcucContainer(name="Test", short_name="TestShort")
        self.assertEqual(len(container.parameters), 0)

        param = bm.EcucParameter(name="P1", value=True, param_type="BOOLEAN")
        container.add_parameter(param)
        self.assertEqual(len(container.parameters), 1)

        ref = bm.EcucReference(name="R1", target="/T")
        container.add_reference(ref)
        self.assertEqual(len(container.references), 1)

        sub = bm.EcucContainer(name="Sub", short_name="Sub")
        container.add_sub_container(sub)
        self.assertEqual(len(container.sub_containers), 1)

    def test_ecuc_container_to_dict(self):
        """Test EcucContainer to_dict"""
        import base_model as bm
        param = bm.EcucParameter(name="P1", value=True)
        container = bm.EcucContainer(name="Test", description="Test container")
        container.add_parameter(param)
        d = container.to_dict()
        self.assertEqual(d["name"], "Test")
        self.assertEqual(len(d["parameters"]), 1)

    def test_ecuc_config_model_abstract_methods(self):
        """Test that abstract methods exist"""
        import base_model as bm
        self.assertTrue(hasattr(bm.EcucConfigModel, 'configure'))
        self.assertTrue(hasattr(bm.EcucConfigModel, 'validate'))
        self.assertTrue(hasattr(bm.EcucConfigModel, 'generate_arxml'))

    def test_container_defaults(self):
        """Test EcucContainer default values"""
        import base_model as bm
        container = bm.EcucContainer(name="Default")
        self.assertEqual(container.short_name, "")
        self.assertEqual(len(container.parameters), 0)
        self.assertEqual(len(container.references), 0)
        self.assertEqual(len(container.sub_containers), 0)

    def test_config_model_add_methods(self):
        """Test EcucConfigModel add methods"""
        import base_model as bm
        # We can't instantiate EcucConfigModel directly (abstract), 
        # but we can test container operations
        container = bm.EcucContainer(name="Test")
        container.add_parameter(bm.EcucParameter(name="P1", value=1))
        container.add_reference(bm.EcucReference(name="R1", target="/T"))
        container.add_sub_container(bm.EcucContainer(name="Sub"))
        self.assertEqual(len(container.parameters), 1)
        self.assertEqual(len(container.references), 1)
        self.assertEqual(len(container.sub_containers), 1)

    def test_config_model_to_dict(self):
        """Test that config model has expected structure"""
        # Create a real module config and verify structure
        module = create_module_config("Mcu", "/AUTOSAR/EcucDefs/Mcu", "MCAL")
        c = create_container("/AUTOSAR/EcucDefs/Mcu/McuGeneral", "McuGeneral",
                             "ECUC-CONTAINER-DEF")
        c.add_parameter(create_boolean_param("/AUTOSAR/EcucDefs/Mcu/McuGeneral/BoolP", True))
        module.add_container(c)
        self.assertEqual(len(module.containers), 1)

    def test_generate_module_config(self):
        """Test generate_module_config convenience method"""
        module = create_module_config("Os", "/AUTOSAR/EcucDefs/Os")
        gen = ArxmlEcucGenerator()
        # Use generate_module_config
        xml = gen.generate_module_config(module)
        self.assertEqual(len(gen.generated_modules), 1)
        self.assertIn("Os", xml)


if __name__ == '__main__':
    unittest.main()
class TestArxmlGeneratorEdgeCases(unittest.TestCase):
    """Edge case tests for ArxmlEcucGenerator"""

    def test_save_to_file(self):
        """Test save() method writes valid XML"""
        module = create_module_config("Mcu", "/AUTOSAR/EcucDefs/Mcu")
        gen = ArxmlEcucGenerator()
        gen.register_module(module)
        with tempfile.NamedTemporaryFile(suffix='.arxml', delete=False) as f:
            path = f.name
        try:
            gen.save(path)
            with open(path) as f:
                content = f.read()
                self.assertIn("AUTOSAR", content)
            os.unlink(path)
            # test with pretty=False
            gen.save(path, pretty=False)
            with open(path) as f:
                content = f.read()
                self.assertIn("AUTOSAR", content)
        finally:
            if os.path.exists(path):
                os.unlink(path)

    def test_clear_after_generate(self):
        """Test clear() after generating"""
        module = create_module_config("Dio", "/AUTOSAR/EcucDefs/Dio")
        gen = ArxmlEcucGenerator()
        gen.register_module(module)
        gen.to_string()
        self.assertEqual(len(gen.generated_modules), 1)
        gen.clear()
        self.assertEqual(len(gen.generated_modules), 0)
        # Should be able to register again
        gen.register_module(module)
        self.assertEqual(len(gen.generated_modules), 1)

    def test_generate_with_reference_value(self):
        """Test EcucReferenceValue with value_ref"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        container = create_container("/Test/Cfg", "Config", "ECUC-CONTAINER-DEF")
        ref_def = EcucDefinitionRef("ECUC-REFERENCE-DEF", "/Test/Ref")
        ref = EcucReferenceValue(ref_def, "/Target/Path")
        container.add_reference(ref)
        module.add_container(container)
        gen = ArxmlEcucGenerator()
        gen.register_module(module)
        xml = gen.to_string()
        self.assertIn("ECUC-REFERENCE-VALUE", xml)
        self.assertIn("VALUE-REF", xml)

    def test_different_variants(self):
        """Test different implementation config variants"""
        variants = ["VARIANT-PRE-COMPILE", "VARIANT-POST-BUILD", "VARIANT-LINK-TIME"]
        for variant in variants:
            module = create_module_config(
                f"Mod_{variant[:6]}",
                f"/AUTOSAR/EcucDefs/{variant}",
                config_variant=variant
            )
            gen = ArxmlEcucGenerator()
            gen.register_module(module)
            xml = gen.to_string()
            self.assertIn(variant, xml)

    def test_multiple_containers_with_refs(self):
        """Test container with both params and refs"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        c1 = create_container("/Test/C1", "Container1", "ECUC-CONTAINER-DEF")
        c1.add_parameter(create_boolean_param("/Test/C1/Bool", True))
        c1.add_reference(create_reference_value("/Test/C1/Ref", "/TargetPath"))
        c2 = create_container("/Test/C2", "Container2", "ECUC-CONTAINER-DEF")
        c2.add_parameter(create_integer_param("/Test/C2/Int", 10))
        module.add_container(c1)
        module.add_container(c2)
        gen = ArxmlEcucGenerator()
        gen.register_module(module)
        xml = gen.to_string()
        self.assertIn("Container1", xml)
        self.assertIn("Container2", xml)
        self.assertIn("ECUC-REFERENCE-VALUE", xml)

    def test_instance_ref_no_context(self):
        """Test instance reference without context (should only have TARGET-REF)"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        container = create_container("/Test/Cfg", "Config", "ECUC-CONTAINER-DEF")
        def_ref = EcucDefinitionRef("ECUC-INSTANCE-REFERENCE-DEF", "/Test/InstRef")
        inst_ref = EcucInstanceReferenceValue(def_ref, target_ref="/Target/Only")
        container.add_reference(inst_ref)
        module.add_container(container)
        gen = ArxmlEcucGenerator()
        gen.register_module(module)
        xml = gen.to_string()
        self.assertIn("ECUC-INSTANCE-REFERENCE-VALUE", xml)
        self.assertIn("TARGET-REF", xml)

    def test_multiple_packages_generated(self):
        """Test generating modules in different AR-PACKAGES"""
        mcu_module = create_module_config("Mcu", "/AUTOSAR/EcucDefs/Mcu", "MCAL")
        com_module = create_module_config("Com", "/AUTOSAR/EcucDefs/Com", "BSW")
        os_module = create_module_config("Os", "/AUTOSAR/EcucDefs/Os", "OS")
        gen = ArxmlEcucGenerator()
        gen.register_module(mcu_module)
        gen.register_module(com_module)
        gen.register_module(os_module)
        xml = gen.to_string()
        self.assertIn("MCAL", xml)
        self.assertIn("BSW", xml)
        self.assertIn("OS", xml)

    def test_ecuc_function_like_params(self):
        """Test function name, linker, and macro params generate correctly"""
        module = create_module_config("Test", "/AUTOSAR/EcucDefs/Test")
        container = create_container("/Test/Cfg", "Config", "ECUC-CONTAINER-DEF")

        fn_def = EcucDefinitionRef("ECUC-FUNCTION-NAME-DEF", "/Test/Func")
        fn_val = EcucFunctionNameParamValue(fn_def, "MyCallback")
        container.add_parameter(fn_val)

        ls_def = EcucDefinitionRef("ECUC-LINKER-SYMBOL-DEF", "/Test/Linker")
        ls_val = EcucLinkerSymbolParamValue(ls_def, "g_MySymbol")
        container.add_parameter(ls_val)

        macro_def = EcucDefinitionRef("ECUC-MACRO-DEF", "/Test/Macro")
        macro_val = EcucMacroParamValue(macro_def, "MY_CONFIG_MACRO")
        container.add_parameter(macro_val)

        module.add_container(container)
        gen = ArxmlEcucGenerator()
        gen.register_module(module)
        xml = gen.to_string()
        self.assertIn("ECUC-FUNCTION-NAME-DEF", xml)
        self.assertIn("ECUC-LINKER-SYMBOL-DEF", xml)
        self.assertIn("ECUC-MACRO-NAME-DEF", xml)

    def test_generate_module_config_single(self):
        """Test generate_module_config convenience method"""
        module = create_module_config("Can", "/AUTOSAR/EcucDefs/Can")
        container = create_container("/AUTOSAR/EcucDefs/Can/CanGeneral", "CanGeneral",
                                     "ECUC-CONTAINER-DEF")
        container.add_parameter(create_boolean_param(
            "/AUTOSAR/EcucDefs/Can/CanGeneral/CanDevErrorDetect", True))
        module.add_container(container)
        gen = ArxmlEcucGenerator()
        xml = gen.generate_module_config(module)
        self.assertIn("Can", xml)
        self.assertIn("CanGeneral", xml)


if __name__ == '__main__':
    unittest.main()
