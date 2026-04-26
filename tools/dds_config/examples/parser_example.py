#!/usr/bin/env python3
"""
DDS Configuration Parser Example

This example demonstrates how to use the DDS configuration parser module:
- Parse YAML configuration files
- Validate configuration
- Convert between YAML and ARXML formats
"""

import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from parser import (
    DDSConfig, SystemInfo, Domain, Participant, Topic, QoS,
    parse_yaml_file, write_yaml_file,
    SchemaValidator, validate_yaml_file,
    ARXMLConverter, yaml_to_arxml, arxml_to_yaml,
    ValidationReport
)


def example_1_parse_yaml():
    """Example 1: Parse YAML configuration file"""
    print("=" * 60)
    print("Example 1: Parse YAML Configuration")
    print("=" * 60)
    
    yaml_path = "example_dds_config.yaml"
    
    try:
        config, warnings = parse_yaml_file(yaml_path)
        print(f"\n✓ Successfully parsed: {yaml_path}")
        print(f"  System: {config.system.name} v{config.system.version}")
        print(f"  Domains: {len(config.domains)}")
        print(f"  Participants: {config.get_participant_count()}")
        print(f"  Topics: {len(config.get_all_topics())}")
        
        if warnings:
            print(f"\n  Warnings ({len(warnings)}):")
            for warning in warnings:
                print(f"    - {warning}")
        
        return config
    except Exception as e:
        print(f"\n✗ Error parsing YAML: {e}")
        return None


def example_2_validate_config(config):
    """Example 2: Validate configuration"""
    print("\n" + "=" * 60)
    print("Example 2: Validate Configuration")
    print("=" * 60)
    
    validator = SchemaValidator()
    report = validator.validate(config)
    
    print(f"\nValidation Result: {'VALID' if report.is_valid else 'INVALID'}")
    print(f"  Errors: {len(report.errors)}")
    print(f"  Warnings: {len(report.warnings)}")
    
    if report.errors:
        print("\n  Errors:")
        for error in report.errors:
            print(f"    - {error}")
    
    if report.warnings:
        print("\n  Warnings:")
        for warning in report.warnings:
            print(f"    - {warning}")
    
    if report.stats:
        print(f"\n  Statistics:")
        for key, value in report.stats.items():
            print(f"    - {key}: {value}")
    
    return report


def example_3_yaml_to_arxml(config):
    """Example 3: Convert YAML to ARXML"""
    print("\n" + "=" * 60)
    print("Example 3: Convert YAML to ARXML")
    print("=" * 60)
    
    try:
        arxml = yaml_to_arxml(config, package_name="DDS_ADAS_Configuration")
        print("\n✓ Successfully converted to ARXML")
        print("\n  ARXML Output (first 1000 chars):")
        print("  " + "-" * 50)
        print(arxml[:1000])
        print("  " + "-" * 50)
        print(f"  ... (total {len(arxml)} characters)")
        
        # Save to file
        output_path = "output_config.arxml"
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(arxml)
        print(f"\n  Saved to: {output_path}")
        
        return arxml
    except Exception as e:
        print(f"\n✗ Error converting to ARXML: {e}")
        import traceback
        traceback.print_exc()
        return None


def example_4_arxml_to_yaml(arxml_string):
    """Example 4: Convert ARXML back to YAML"""
    print("\n" + "=" * 60)
    print("Example 4: Convert ARXML back to YAML")
    print("=" * 60)
    
    try:
        config = arxml_to_yaml(arxml_string)
        print("\n✓ Successfully converted ARXML to DDSConfig")
        print(f"  System: {config.system.name}")
        print(f"  Domains: {len(config.domains)}")
        
        # Convert back to YAML
        output_path = "output_config.yaml"
        write_yaml_file(config, output_path, include_comments=True)
        print(f"\n  Saved to: {output_path}")
        
        return config
    except Exception as e:
        print(f"\n✗ Error converting from ARXML: {e}")
        import traceback
        traceback.print_exc()
        return None


def example_5_create_config_programmatically():
    """Example 5: Create configuration programmatically"""
    print("\n" + "=" * 60)
    print("Example 5: Create Configuration Programmatically")
    print("=" * 60)
    
    from parser.config_model import Publisher, Subscriber, TSN, TSNStream
    
    config = DDSConfig(
        system=SystemInfo(
            name="GeneratedSystem",
            version="2.0.0",
            description="Programmatically generated DDS configuration"
        ),
        domains=[
            Domain(
                id=0,
                name="DefaultDomain",
                participants=[
                    Participant(
                        name="Node1",
                        publishers=[
                            Publisher(
                                topic="Temperature",
                                type="Float64",
                                qos=QoS(reliability="BEST_EFFORT")
                            )
                        ]
                    ),
                    Participant(
                        name="Node2",
                        subscribers=[
                            Subscriber(
                                topic="Temperature",
                                type="Float64",
                                qos=QoS(reliability="BEST_EFFORT")
                            )
                        ]
                    )
                ]
            )
        ],
        tsn=TSN(
            enabled=True,
            streams=[
                TSNStream(
                    name="SensorStream",
                    priority=5,
                    bandwidth=1000000,
                    max_latency=50000
                )
            ]
        )
    )
    
    print("\n✓ Configuration created programmatically")
    print(f"  System: {config.system.name}")
    print(f"  Participants: {config.get_participant_count()}")
    
    # Save to YAML
    output_path = "generated_config.yaml"
    write_yaml_file(config, output_path)
    print(f"\n  Saved to: {output_path}")
    
    return config


def example_6_validate_file():
    """Example 6: Validate YAML file directly"""
    print("\n" + "=" * 60)
    print("Example 6: Validate YAML File Directly")
    print("=" * 60)
    
    yaml_path = "example_dds_config.yaml"
    report = validate_yaml_file(yaml_path)
    
    print(f"\nValidation Report:")
    print(report.get_summary())
    
    return report


def main():
    """Run all examples"""
    print("\nDDS Configuration Parser Examples")
    print("=" * 60)
    print()
    
    # Example 1: Parse YAML
    config = example_1_parse_yaml()
    if not config:
        print("\nFailed to parse configuration. Exiting.")
        return 1
    
    # Example 2: Validate
    report = example_2_validate_config(config)
    
    # Example 3: YAML to ARXML
    arxml = example_3_yaml_to_arxml(config)
    
    # Example 4: ARXML back to YAML
    if arxml:
        example_4_arxml_to_yaml(arxml)
    
    # Example 5: Create programmatically
    example_5_create_config_programmatically()
    
    # Example 6: Validate file
    example_6_validate_file()
    
    print("\n" + "=" * 60)
    print("All examples completed!")
    print("=" * 60)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
