"""
DDS Configuration Manager
"""
import json
import xml.etree.ElementTree as ET
from typing import Optional
from dds_models import DDSConfiguration, DDSDomain, DDSTopic, DDSParticipant
from dds_models import TopicQos, ReliabilityQosPolicy, DurabilityQosPolicy

class DDSConfigurator:
    """Main configuration manager for DDS"""
    
    def __init__(self):
        self.configurations: dict = {}
        self.current_config: Optional[DDSConfiguration] = None
    
    def create_configuration(self, name: str, version: str = "1.0.0") -> DDSConfiguration:
        """Create a new DDS configuration"""
        config = DDSConfiguration(name=name, version=version)
        self.configurations[name] = config
        self.current_config = config
        return config
    
    def add_domain(self, domain_id: int, name: str, description: str = "") -> DDSDomain:
        """Add a domain to current configuration"""
        if not self.current_config:
            raise ValueError("No configuration selected")
        
        domain = DDSDomain(
            domain_id=domain_id,
            name=name,
            description=description
        )
        self.current_config.domains.append(domain)
        return domain
    
    def add_topic(self, domain: DDSDomain, name: str, data_type: str, 
                  keyed: bool = False) -> DDSTopic:
        """Add a topic to a domain"""
        topic = DDSTopic(
            name=name,
            data_type=data_type,
            keyed=keyed
        )
        domain.topics.append(topic)
        return topic
    
    def set_topic_qos(self, topic: DDSTopic, reliability: str = None,
                      durability: str = None, history: str = None):
        """Set QoS policies for a topic"""
        if reliability:
            topic.qos.reliability.kind = reliability
        if durability:
            topic.qos.durability.kind = durability
        if history:
            topic.qos.history.kind = history
    
    def export_to_json(self, filename: str):
        """Export configuration to JSON file"""
        if not self.current_config:
            raise ValueError("No configuration to export")
        
        with open(filename, 'w') as f:
            json.dump(self.current_config.to_dict(), f, indent=2)
    
    def export_to_xml(self, filename: str):
        """Export configuration to DDS XML format"""
        if not self.current_config:
            raise ValueError("No configuration to export")
        
        root = ET.Element("dds")
        root.set("xmlns", "http://www.omg.org/dds/")
        
        for domain in self.current_config.domains:
            domain_elem = ET.SubElement(root, "domain")
            domain_elem.set("id", str(domain.domain_id))
            domain_elem.set("name", domain.name)
            
            topics_elem = ET.SubElement(domain_elem, "topics")
            for topic in domain.topics:
                topic_elem = ET.SubElement(topics_elem, "topic")
                topic_elem.set("name", topic.name)
                topic_elem.set("data_type", topic.data_type)
                topic_elem.set("keyed", str(topic.keyed).lower())
        
        tree = ET.ElementTree(root)
        tree.write(filename, encoding='utf-8', xml_declaration=True)
    
    def import_from_json(self, filename: str) -> DDSConfiguration:
        """Import configuration from JSON file"""
        with open(filename, 'r') as f:
            data = json.load(f)
        
        config = DDSConfiguration(
            name=data.get('name', 'imported'),
            version=data.get('version', '1.0.0')
        )
        
        # Parse domains
        for domain_data in data.get('domains', []):
            domain = self.add_domain(
                domain_id=domain_data['domain_id'],
                name=domain_data['name'],
                description=domain_data.get('description', '')
            )
            
            # Parse topics
            for topic_data in domain_data.get('topics', []):
                self.add_topic(
                    domain=domain,
                    name=topic_data['name'],
                    data_type=topic_data['data_type'],
                    keyed=topic_data.get('keyed', False)
                )
        
        self.current_config = config
        self.configurations[config.name] = config
        return config
    
    def validate(self) -> list:
        """Validate current configuration"""
        errors = []
        
        if not self.current_config:
            errors.append("No configuration selected")
            return errors
        
        # Check for duplicate topic names in same domain
        for domain in self.current_config.domains:
            topic_names = [t.name for t in domain.topics]
            if len(topic_names) != len(set(topic_names)):
                errors.append(f"Domain {domain.name} has duplicate topic names")
        
        # Check for duplicate domain IDs
        domain_ids = [d.domain_id for d in self.current_config.domains]
        if len(domain_ids) != len(set(domain_ids)):
            errors.append("Duplicate domain IDs found")
        
        return errors

# Singleton instance
_configurator = DDSConfigurator()

def get_configurator() -> DDSConfigurator:
    """Get the global DDS configurator instance"""
    return _configurator
