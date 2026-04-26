#!/usr/bin/env python3
"""
Demo script for DDS Configuration Tool.

This script demonstrates the usage of diagnostic, E2E, and OTA configuration modules.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from diagnostic_config import DiagnosticConfig, DCMParams, DiagServiceConfig, DiagPriority
from diagnostic_config import DiagSessionType, DIDDefinition, create_common_did
from e2e_config import E2EConfig, E2EProfileConfig, DataIDMapping, E2EProfile
from ota_config import OTAConfig, ABPartitionLayout, OTACampaign, ECUUpdate
from code_generator import CCodeGenerator, ConfigValidator


def demo_diagnostic_config():
    """Demonstrate diagnostic configuration."""
    print("=" * 60)
    print("Diagnostic Configuration Demo")
    print("=" * 60)
    
    config = DiagnosticConfig()
    
    # Set DCM parameters
    config.dcm_params = DCMParams(
        ecu_id=0x7E0,
        logical_address=0x0001,
        functional_address=0x7DF,
        p2_timeout_ms=50,
        p2_star_timeout_ms=5000
    )
    print(f"ECU ID: 0x{config.dcm_params.ecu_id:04X}")
    print(f"P2 Timeout: {config.dcm_params.p2_timeout_ms} ms")
    
    # Add services
    for svc in config.get_default_services():
        config.add_service(svc)
    print(f"Added {len(config.services)} diagnostic services")
    
    # Add DIDs
    did = create_common_did(0xF190)  # VIN
    if did:
        config.add_did(did)
        print(f"Added DID: {did.name} (0x{did.did:04X})")
    
    # Validate
    errors = config.validate()
    if errors:
        print(f"Validation errors: {errors}")
    else:
        print("Configuration is valid!")
    
    return config


def demo_e2e_config():
    """Demonstrate E2E configuration."""
    print("\n" + "=" * 60)
    print("E2E Protection Configuration Demo")
    print("=" * 60)
    
    config = E2EConfig()
    
    # Add E2E profile
    profile = E2EProfileConfig(
        profile=E2EProfile.PROFILE_5,
        data_id=0x0100,
        data_length=16
    )
    config.add_profile(profile)
    print(f"Added E2E Profile {profile.profile.value}")
    
    # Add Data ID mappings
    mappings = [
        (0x0100, "VehicleSpeed", "Sensor_ECU", "Powertrain_ECU", "ASIL_B"),
        (0x0101, "EngineTorque", "Engine_ECU", "Trans_ECU", "ASIL_D"),
        (0x0200, "BrakePressure", "Brake_ECU", "Chassis_ECU", "ASIL_D"),
    ]
    
    for data_id, name, src, tgt, safety in mappings:
        mapping = DataIDMapping(
            data_id=data_id,
            name=name,
            source_component=src,
            target_component=tgt,
            data_type="uint32",
            signal_name=name,
            profile=E2EProfile.PROFILE_5,
            safety_level=safety
        )
        config.add_data_id_mapping(mapping)
        print(f"  Mapped 0x{data_id:04X}: {name} ({safety})")
    
    # Validate
    errors = config.validate()
    if errors:
        print(f"Validation errors: {errors}")
    else:
        print("Configuration is valid!")
    
    return config


def demo_ota_config():
    """Demonstrate OTA configuration."""
    print("\n" + "=" * 60)
    print("OTA Update Configuration Demo")
    print("=" * 60)
    
    config = OTAConfig()
    
    # Add partition layout
    layout = config.get_default_partition_layout(flash_size_mb=8)
    config.add_partition_layout(layout)
    print(f"Added partition layout: {layout.name}")
    print(f"  Active bank: {layout.active_bank}")
    print(f"  Partitions: {len(layout.bank_a_partitions) + len(layout.bank_b_partitions) + len(layout.shared_partitions)}")
    
    # Add campaign
    campaign = OTACampaign(
        campaign_id="CAMP-2026-001",
        name="Q2 Firmware Update",
        description="Quarterly firmware update",
        priority="high"
    )
    
    # Add ECU update
    ecu_update = ECUUpdate(
        ecu_id=0x0101,
        name="Infotainment_HU",
        hardware_version="2.1.0",
        firmware_version_from="3.2.1",
        firmware_version_to="3.3.0",
        package_file="ecu_0101.epkg",
        package_size=52428800,
        package_hash="sha256:abc123..."
    )
    campaign.ecu_updates.append(ecu_update)
    config.add_campaign(campaign)
    
    print(f"Added campaign: {campaign.campaign_id}")
    print(f"  ECU updates: {len(campaign.ecu_updates)}")
    
    # Set security
    config.security_verification.root_cert_hash = "sha256:a1b2c3..."
    
    # Validate
    errors = config.validate()
    if errors:
        print(f"Validation errors: {errors}")
    else:
        print("Configuration is valid!")
    
    return config


def demo_code_generation(diag_config, e2e_config, ota_config):
    """Demonstrate code generation."""
    print("\n" + "=" * 60)
    print("Code Generation Demo")
    print("=" * 60)
    
    generator = CCodeGenerator()
    
    # Generate headers
    diag_header = generator.generate_diagnostic_header(diag_config)
    e2e_header = generator.generate_e2e_header(e2e_config)
    ota_header = generator.generate_ota_header(ota_config)
    
    print(f"Diagnostic header: {len(diag_header)} characters")
    print(f"E2E header: {len(e2e_header)} characters")
    print(f"OTA header: {len(ota_header)} characters")
    
    # Show snippet of diagnostic header
    print("\n--- Diagnostic Header Snippet ---")
    lines = diag_header.split('\n')[:20]
    for line in lines:
        print(line)
    print("...")


def main():
    """Main demo function."""
    print("DDS Configuration Tool Demo")
    print("Automotive Diagnostic / E2E / OTA Configuration")
    
    # Run demos
    diag_config = demo_diagnostic_config()
    e2e_config = demo_e2e_config()
    ota_config = demo_ota_config()
    
    demo_code_generation(diag_config, e2e_config, ota_config)
    
    print("\n" + "=" * 60)
    print("Demo completed successfully!")
    print("=" * 60)
    print("\nTo run the GUI:")
    print("  python3 main_window.py")
    print("\nTo run tests:")
    print("  python3 run_tests.py")


if __name__ == "__main__":
    main()
