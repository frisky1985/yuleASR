"""
DDS Config Generator Unit Tests

Tests for the C code generation functionality.
"""

import unittest
import sys
from pathlib import Path
from datetime import datetime

# Add tools to path
sys.path.insert(0, str(Path(__file__).parent.parent.parent / "tools" / "dds_config"))

from generator.template_engine import TemplateEngine, CodeStyle
from generator.code_optimizer import (
    CodeOptimizer, NamingConvention, CommentGenerator,
    SystemConfig, DomainConfig, ParticipantConfig, TopicConfig, QoSProfile
)
from generator.header_generator import HeaderGenerator, HeaderGenerationOptions
from generator.source_generator import SourceGenerator, SourceGenerationOptions


class TestNamingConvention(unittest.TestCase):
    """Test naming convention conversion"""

    def setUp(self):
        self.naming = NamingConvention()

    def test_to_macro_name(self):
        self.assertEqual(
            self.naming.to_macro_name("CameraObjectDetection", "DDS"),
            "DDS_CAMERA_OBJECT_DETECTION"
        )
        self.assertEqual(
            self.naming.to_macro_name("perceptionDomain", "DDS"),
            "DDS_PERCEPTION_DOMAIN"
        )

    def test_to_variable_name(self):
        self.assertEqual(
            self.naming.to_variable_name("CameraECU", "g_"),
            "g_camera_ecu"
        )
        self.assertEqual(
            self.naming.to_variable_name("sensor_high_reliability", "g_"),
            "g_sensor_high_reliability"
        )

    def test_to_struct_name(self):
        self.assertEqual(
            self.naming.to_struct_name("ParticipantConfig"),
            "participant_config_t"
        )

    def test_to_function_name(self):
        self.assertEqual(
            self.naming.to_function_name("InitConfig", "dds"),
            "dds_init_config"
        )


class TestCommentGenerator(unittest.TestCase):
    """Test comment generation"""

    def setUp(self):
        self.gen = CommentGenerator()

    def test_file_header(self):
        header = self.gen.generate_file_header(
            "test.h",
            "Test header file",
            version="1.0"
        )
        self.assertIn("@file test.h", header)
        self.assertIn("@brief Test header file", header)
        self.assertIn("@version 1.0", header)

    def test_section_comment(self):
        comment = self.gen.generate_section_comment("Test Section")
        self.assertIn("Test Section", comment)
        self.assertIn("=", comment)

    def test_doxygen_comment(self):
        comment = self.gen.generate_doxygen_comment(
            "Test function",
            params={"arg1": "First argument"},
            returns="Success status"
        )
        self.assertIn("@brief Test function", comment)
        self.assertIn("@param arg1", comment)
        self.assertIn("@return Success status", comment)


class TestCodeOptimizer(unittest.TestCase):
    """Test code optimization"""

    def setUp(self):
        self.optimizer = CodeOptimizer()

    def _create_sample_config(self):
        """Create a sample system config"""
        qos1 = QoSProfile(
            name="sensor_reliable",
            reliability="RELIABLE",
            durability="TRANSIENT_LOCAL",
            deadline_sec=0,
            deadline_nanosec=33000000,
            history_depth=1
        )

        qos2 = QoSProfile(
            name="control_realtime",
            reliability="RELIABLE",
            durability="VOLATILE",
            deadline_sec=0,
            deadline_nanosec=10000000,
            history_depth=1
        )

        topic1 = TopicConfig(
            name="CameraObjectDetection",
            type_name="ObjectDetectionData",
            qos_profile="sensor_reliable",
            domain_id=1
        )

        participant = ParticipantConfig(
            name="CameraECU",
            domain_id=1,
            qos_profile="sensor_reliable",
            publishers=[topic1],
            subscribers=[]
        )

        domain = DomainConfig(
            id=1,
            name="PerceptionDomain",
            participants=[participant]
        )

        config = SystemConfig(
            name="ADAS_Sensor_Fusion",
            version="1.2.0",
            domains=[domain],
            qos_profiles={
                "sensor_reliable": qos1,
                "control_realtime": qos2
            }
        )

        return config

    def test_optimize_qos_profiles(self):
        config = self._create_sample_config()
        optimized = self.optimizer.optimize(config)

        # Check QoS profiles are preserved
        self.assertGreater(len(optimized.qos_profiles), 0)

        # Check naming convention applied
        for name in optimized.qos_profiles.keys():
            self.assertTrue(name.startswith("qos_") or name.startswith("g_"))

    def test_optimize_domains(self):
        config = self._create_sample_config()
        optimized = self.optimizer.optimize(config)

        # Check domains are preserved
        self.assertEqual(len(optimized.domains), 1)

    def test_generate_include_guard(self):
        guard = self.optimizer.generate_include_guard("dds_generated_config.h")
        self.assertEqual(guard, "DDS_GENERATED_CONFIG_H")


class TestTemplateEngine(unittest.TestCase):
    """Test template engine"""

    def setUp(self):
        self.engine = TemplateEngine()

    def test_c_identifier_filter(self):
        from generator.template_engine import _sanitize_c_identifier
        self.assertEqual(_sanitize_c_identifier("test-name"), "test_name")
        self.assertEqual(_sanitize_c_identifier("test.name"), "test_name")
        self.assertEqual(_sanitize_c_identifier("123test"), "_123test")

    def test_upper_snake_filter(self):
        from generator.template_engine import _to_upper_snake_case
        self.assertEqual(_to_upper_snake_case("TestName"), "TEST_NAME")
        self.assertEqual(_to_upper_snake_case("testName"), "TEST_NAME")

    def test_render_string(self):
        template = "#define {{ name | upper_snake }} {{ value }}"
        result = self.engine.render_string(template, {"name": "testValue", "value": 42})
        self.assertIn("TEST_VALUE", result)


class TestHeaderGenerator(unittest.TestCase):
    """Test header file generation"""

    def setUp(self):
        self.generator = HeaderGenerator()

    def _create_sample_config(self):
        qos = QoSProfile(
            name="sensor_reliable",
            reliability="RELIABLE",
            durability="TRANSIENT_LOCAL",
            deadline_sec=0,
            deadline_nanosec=33000000,
            history_depth=1
        )

        topic = TopicConfig(
            name="CameraObjectDetection",
            type_name="ObjectDetectionData",
            qos_profile="sensor_reliable",
            domain_id=1
        )

        participant = ParticipantConfig(
            name="CameraECU",
            domain_id=1,
            qos_profile="sensor_reliable",
            publishers=[topic],
            subscribers=[]
        )

        domain = DomainConfig(
            id=1,
            name="PerceptionDomain",
            participants=[participant]
        )

        return SystemConfig(
            name="ADAS_Test",
            version="1.0.0",
            domains=[domain],
            qos_profiles={"sensor_reliable": qos}
        )

    def test_generate_header(self):
        config = self._create_sample_config()
        content = self.generator.generate(config)

        # Check header structure
        self.assertIn("#ifndef", content)
        self.assertIn("#define", content)
        self.assertIn("#endif", content)

        # Check domain macro
        self.assertIn("DDS_DOMAIN_ID_DDS_DOMAIN_PERCEPTION_DOMAIN", content)

        # Check topic macro
        self.assertIn("DDS_TOPIC_G_CAMERA_OBJECT_DETECTION", content)

        # Check extern declarations
        self.assertIn("extern const dds_qos_t", content)
        self.assertIn("extern dds_domain_participant_t", content)

        # Check init functions
        self.assertIn("eth_status_t dds_init_generated_config(void)", content)
        self.assertIn("void dds_deinit_generated_config(void)", content)


class TestSourceGenerator(unittest.TestCase):
    """Test source file generation"""

    def setUp(self):
        self.generator = SourceGenerator()

    def _create_sample_config(self):
        qos = QoSProfile(
            name="sensor_reliable",
            reliability="RELIABLE",
            durability="TRANSIENT_LOCAL",
            deadline_sec=0,
            deadline_nanosec=33000000,
            history_depth=1
        )

        topic = TopicConfig(
            name="CameraObjectDetection",
            type_name="ObjectDetectionData",
            qos_profile="sensor_reliable",
            domain_id=1
        )

        participant = ParticipantConfig(
            name="CameraECU",
            domain_id=1,
            qos_profile="sensor_reliable",
            publishers=[topic],
            subscribers=[]
        )

        domain = DomainConfig(
            id=1,
            name="PerceptionDomain",
            participants=[participant]
        )

        return SystemConfig(
            name="ADAS_Test",
            version="1.0.0",
            domains=[domain],
            qos_profiles={"sensor_reliable": qos}
        )

    def test_generate_source(self):
        config = self._create_sample_config()
        content = self.generator.generate(config)

        # Check includes
        self.assertIn("#include", content)

        # Check QoS definitions
        self.assertIn("dds_qos_t", content)
        self.assertIn("DDS_QOS_RELIABILITY_RELIABLE", content)

        # Check participant variables
        self.assertIn("dds_domain_participant_t *g_camera_ecu = NULL", content)

        # Check init function
        self.assertIn("dds_init_generated_config", content)
        self.assertIn("dds_runtime_init", content)

        # Check deinit function
        self.assertIn("dds_deinit_generated_config", content)

    def test_generate_callbacks(self):
        config = self._create_sample_config()
        content = self.generator.generate(config)

        # Check callback functions exist
        self.assertIn("dds_generated_on_data_received", content)
        self.assertIn("dds_generated_on_write_complete", content)


class TestIntegration(unittest.TestCase):
    """Integration tests"""

    def test_full_generation_pipeline(self):
        """Test complete generation pipeline"""
        # Create complex config
        qos1 = QoSProfile(
            name="sensor_high_reliability",
            reliability="RELIABLE",
            durability="TRANSIENT_LOCAL",
            deadline_sec=0,
            deadline_nanosec=33000000,
            history_depth=10
        )

        qos2 = QoSProfile(
            name="control_realtime",
            reliability="RELIABLE",
            durability="VOLATILE",
            deadline_sec=0,
            deadline_nanosec=10000000,
            history_depth=1
        )

        topic1 = TopicConfig(
            name="CameraObjectDetection",
            type_name="ObjectDetectionData",
            qos_profile="sensor_high_reliability",
            domain_id=1
        )

        topic2 = TopicConfig(
            name="LidarPointCloud",
            type_name="PointCloudData",
            qos_profile="sensor_high_reliability",
            domain_id=1
        )

        participant1 = ParticipantConfig(
            name="CameraECU",
            domain_id=1,
            qos_profile="sensor_high_reliability",
            publishers=[topic1],
            subscribers=[]
        )

        participant2 = ParticipantConfig(
            name="FusionECU",
            domain_id=1,
            qos_profile="control_realtime",
            publishers=[],
            subscribers=[topic1, topic2]
        )

        domain = DomainConfig(
            id=1,
            name="PerceptionDomain",
            participants=[participant1, participant2]
        )

        config = SystemConfig(
            name="ADAS_Full_System",
            version="2.0.0",
            domains=[domain],
            qos_profiles={
                "sensor_high_reliability": qos1,
                "control_realtime": qos2
            }
        )

        # Generate header
        header_gen = HeaderGenerator()
        header_content = header_gen.generate(config)

        # Generate source
        source_gen = SourceGenerator()
        source_content = source_gen.generate(config)

        # Verify both files generated correctly
        self.assertIn("ADAS_Full_System", header_content)
        self.assertIn("ADAS_Full_System", source_content)
        self.assertIn("dds_init_generated_config", header_content)
        self.assertIn("dds_init_generated_config", source_content)


if __name__ == '__main__':
    unittest.main()
