"""
DDS Configuration CLI Tool
"""
import argparse
import sys
import os

# Add src to path
sys.path.insert(0, os.path.dirname(__file__))

from dds_configurator import DDSConfigurator, get_configurator
from dds_models import ReliabilityQosPolicy, DurabilityQosPolicy
from code_generator import DDSCodeGenerator

def create_sample_config():
    """Create a sample DDS configuration"""
    configurator = get_configurator()
    
    # Create configuration
    config = configurator.create_configuration("VehicleDDS", "1.0.0")
    
    # Add domain
    domain = configurator.add_domain(
        domain_id=0,
        name="VehicleDomain",
        description="Main vehicle communication domain"
    )
    
    # Add topics
    configurator.add_topic(domain, "VehicleSpeed", "SpeedData", keyed=True)
    configurator.add_topic(domain, "EngineStatus", "EngineData")
    configurator.add_topic(domain, "GPSPosition", "GPSData", keyed=True)
    configurator.add_topic(domain, "SensorData", "SensorArray")
    
    # Set QoS for critical topics
    speed_topic = domain.topics[0]
    configurator.set_topic_qos(
        speed_topic,
        reliability="RELIABLE",
        durability="TRANSIENT_LOCAL"
    )
    
    return config

def main():
    parser = argparse.ArgumentParser(
        description='DDS Configuration Tool for yuleCommunity'
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Create command
    create_parser = subparsers.add_parser('create', help='Create a new configuration')
    create_parser.add_argument('--name', required=True, help='Configuration name')
    create_parser.add_argument('--version', default='1.0.0', help='Configuration version')
    
    # Add domain command
    domain_parser = subparsers.add_parser('add-domain', help='Add a domain')
    domain_parser.add_argument('--id', type=int, required=True, help='Domain ID')
    domain_parser.add_argument('--name', required=True, help='Domain name')
    
    # Add topic command
    topic_parser = subparsers.add_parser('add-topic', help='Add a topic')
    topic_parser.add_argument('--domain', type=int, required=True, help='Domain ID')
    topic_parser.add_argument('--name', required=True, help='Topic name')
    topic_parser.add_argument('--type', required=True, help='Data type')
    topic_parser.add_argument('--keyed', action='store_true', help='Topic is keyed')
    
    # Export command
    export_parser = subparsers.add_parser('export', help='Export configuration')
    export_parser.add_argument('--format', choices=['json', 'xml'], default='json',
                               help='Export format')
    export_parser.add_argument('--output', '-o', required=True, help='Output file')
    
    # Generate command
    gen_parser = subparsers.add_parser('generate', help='Generate code')
    gen_parser.add_argument('--output', '-o', required=True, help='Output directory')
    
    # Validate command
    subparsers.add_parser('validate', help='Validate configuration')
    
    # Sample command
    subparsers.add_parser('sample', help='Create sample configuration')
    
    args = parser.parse_args()
    
    if args.command is None:
        parser.print_help()
        return
    
    configurator = get_configurator()
    
    try:
        if args.command == 'create':
            config = configurator.create_configuration(args.name, args.version)
            print(f"Created configuration: {config.name} v{config.version}")
        
        elif args.command == 'add-domain':
            domain = configurator.add_domain(args.id, args.name)
            print(f"Added domain: {domain.name} (ID: {domain.domain_id})")
        
        elif args.command == 'add-topic':
            domain = None
            for d in configurator.current_config.domains:
                if d.domain_id == args.domain:
                    domain = d
                    break
            if not domain:
                print(f"Error: Domain {args.domain} not found")
                return
            
            topic = configurator.add_topic(domain, args.name, args.type, args.keyed)
            print(f"Added topic: {topic.name} ({topic.data_type})")
        
        elif args.command == 'export':
            if args.format == 'json':
                configurator.export_to_json(args.output)
            else:
                configurator.export_to_xml(args.output)
            print(f"Exported to: {args.output}")
        
        elif args.command == 'generate':
            if not configurator.current_config:
                print("Error: No configuration selected")
                return
            
            generator = DDSCodeGenerator(configurator.current_config)
            output_dir = generator.save_all(args.output)
            print(f"Generated code in: {output_dir}")
            print("Files generated:")
            for f in os.listdir(output_dir):
                print(f"  - {f}")
        
        elif args.command == 'validate':
            errors = configurator.validate()
            if errors:
                print("Validation errors:")
                for error in errors:
                    print(f"  - {error}")
            else:
                print("Configuration is valid!")
        
        elif args.command == 'sample':
            config = create_sample_config()
            print(f"Created sample configuration: {config.name}")
            print(f"  Domains: {len(config.domains)}")
            for domain in config.domains:
                print(f"    - {domain.name}: {len(domain.topics)} topics")
    
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
