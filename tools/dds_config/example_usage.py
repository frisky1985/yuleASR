#!/usr/bin/env python3
"""
DDS Config Generator Example Usage

Demonstrates how to use the DDS configuration code generator
to generate C header and source files from YAML configuration.
"""

import sys
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from dds_config.generator.code_optimizer import (
    SystemConfig, DomainConfig, ParticipantConfig, 
    TopicConfig, QoSProfile
)
from dds_config.generator.header_generator import HeaderGenerator, HeaderGenerationOptions
from dds_config.generator.source_generator import SourceGenerator, SourceGenerationOptions


def create_sample_config():
    """创建示例DDS配置"""
    
    # 定义QoS Profiles
    qos_sensor_reliable = QoSProfile(
        name="sensor_reliable",
        reliability="RELIABLE",
        durability="TRANSIENT_LOCAL",
        deadline_sec=0,
        deadline_nanosec=33000000,  # 30Hz
        latency_budget_ms=0,
        history_kind="KEEP_LAST",
        history_depth=10
    )
    
    qos_control_realtime = QoSProfile(
        name="control_realtime",
        reliability="RELIABLE",
        durability="VOLATILE",
        deadline_sec=0,
        deadline_nanosec=10000000,  # 100Hz
        latency_budget_ms=0,
        history_kind="KEEP_LAST",
        history_depth=1
    )
    
    # 定义Topics
    topic_camera = TopicConfig(
        name="CameraObjectDetection",
        type_name="ObjectDetectionData",
        qos_profile="sensor_reliable",
        domain_id=1
    )
    
    topic_lidar = TopicConfig(
        name="LidarPointCloud",
        type_name="PointCloudData",
        qos_profile="sensor_reliable",
        domain_id=1
    )
    
    topic_control = TopicConfig(
        name="VehicleControlCommand",
        type_name="ControlCommandData",
        qos_profile="control_realtime",
        domain_id=2
    )
    
    # 定义Participants
    camera_ecu = ParticipantConfig(
        name="CameraECU",
        domain_id=1,
        qos_profile="sensor_reliable",
        publishers=[topic_camera],
        subscribers=[]
    )
    
    lidar_ecu = ParticipantConfig(
        name="LidarECU",
        domain_id=1,
        qos_profile="sensor_reliable",
        publishers=[topic_lidar],
        subscribers=[]
    )
    
    fusion_ecu = ParticipantConfig(
        name="FusionECU",
        domain_id=1,
        qos_profile="sensor_reliable",
        publishers=[],
        subscribers=[topic_camera, topic_lidar]
    )
    
    control_ecu = ParticipantConfig(
        name="ControlECU",
        domain_id=2,
        qos_profile="control_realtime",
        publishers=[topic_control],
        subscribers=[]
    )
    
    # 定义Domains
    perception_domain = DomainConfig(
        id=1,
        name="PerceptionDomain",
        participants=[camera_ecu, lidar_ecu, fusion_ecu]
    )
    
    control_domain = DomainConfig(
        id=2,
        name="ControlDomain",
        participants=[control_ecu]
    )
    
    # 创建系统配置
    system = SystemConfig(
        name="ADAS_Sensor_Fusion",
        version="2.1.0",
        domains=[perception_domain, control_domain],
        qos_profiles={
            "sensor_reliable": qos_sensor_reliable,
            "control_realtime": qos_control_realtime
        }
    )
    
    return system


def main():
    """主函数"""
    print("=" * 70)
    print("DDS Config Generator Example")
    print("=" * 70)
    
    # 创建配置
    config = create_sample_config()
    print(f"\nSystem: {config.name} v{config.version}")
    print(f"Domains: {len(config.domains)}")
    print(f"QoS Profiles: {len(config.qos_profiles)}")
    
    # 创建生成器
    header_gen = HeaderGenerator(
        options=HeaderGenerationOptions(
            include_version_macro=True,
            generate_doxygen=True
        )
    )
    
    source_gen = SourceGenerator(
        options=SourceGenerationOptions(
            generate_error_check=True,
            generate_callbacks=True
        )
    )
    
    # 生成头文件
    print("\nGenerating header file...")
    header_content = header_gen.generate(config)
    
    # 生成源文件
    print("Generating source file...")
    source_content = source_gen.generate(config)
    
    # 保存文件
    output_dir = Path(__file__).parent / "generated"
    output_dir.mkdir(exist_ok=True)
    
    header_path = output_dir / "dds_generated_config.h"
    source_path = output_dir / "dds_generated_config.c"
    
    header_gen.save(header_content, header_path)
    source_gen.save(source_content, source_path)
    
    print(f"\nFiles generated:")
    print(f"  Header: {header_path}")
    print(f"  Source: {source_path}")
    
    # 显示头文件内容预览
    print("\n" + "=" * 70)
    print("Header File Preview (first 50 lines):")
    print("=" * 70)
    lines = header_content.split('\n')[:50]
    for line in lines:
        print(line)
    print("...")
    
    # 显示优化统计
    print("\n" + "=" * 70)
    print("Optimization Statistics:")
    print("=" * 70)
    stats = source_gen.optimizer.get_stats()
    for key, value in stats.items():
        print(f"  {key}: {value}")
    
    print("\nDone!")


if __name__ == "__main__":
    main()
