"""
DDS Configuration Data Models

This module defines dataclasses for DDS configuration entities.
All configuration elements are represented as typed Python dataclasses
for type safety and IDE support.
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Any, Union
from enum import Enum, auto


class ReliabilityKind(Enum):
    """DDS Reliability QoS kind"""
    BEST_EFFORT = "BEST_EFFORT"
    RELIABLE = "RELIABLE"


class DurabilityKind(Enum):
    """DDS Durability QoS kind"""
    VOLATILE = "VOLATILE"
    TRANSIENT_LOCAL = "TRANSIENT_LOCAL"
    TRANSIENT = "TRANSIENT"
    PERSISTENT = "PERSISTENT"


class HistoryKind(Enum):
    """DDS History QoS kind"""
    KEEP_LAST = "KEEP_LAST"
    KEEP_ALL = "KEEP_ALL"


class LivelinessKind(Enum):
    """DDS Liveliness QoS kind"""
    AUTOMATIC = "AUTOMATIC"
    MANUAL_BY_TOPIC = "MANUAL_BY_TOPIC"
    MANUAL_BY_PARTICIPANT = "MANUAL_BY_PARTICIPANT"


class DestinationOrderKind(Enum):
    """DDS DestinationOrder QoS kind"""
    BY_RECEPTION_TIMESTAMP = "BY_RECEPTION_TIMESTAMP"
    BY_SOURCE_TIMESTAMP = "BY_SOURCE_TIMESTAMP"


class TransportKind(Enum):
    """DDS Transport kind"""
    UDP = "UDP"
    TCP = "TCP"
    SHM = "SHM"
    TSN = "TSN"


@dataclass
class Deadline:
    """DDS Deadline QoS policy"""
    sec: int = 0
    nanosec: int = 0
    
    def to_duration(self) -> Dict[str, int]:
        """Convert to DDS duration format"""
        return {"sec": self.sec, "nanosec": self.nanosec}


@dataclass
class LatencyBudget:
    """DDS LatencyBudget QoS policy"""
    sec: int = 0
    nanosec: int = 0


@dataclass
class History:
    """DDS History QoS policy"""
    kind: HistoryKind = HistoryKind.KEEP_LAST
    depth: int = 1


@dataclass
class Lifespan:
    """DDS Lifespan QoS policy"""
    sec: int = -1
    nanosec: int = 0
    
    def is_infinite(self) -> bool:
        return self.sec < 0


@dataclass
class QoS:
    """DDS QoS profile configuration"""
    reliability: ReliabilityKind = ReliabilityKind.RELIABLE
    durability: DurabilityKind = DurabilityKind.VOLATILE
    history: History = field(default_factory=History)
    deadline: Optional[Deadline] = None
    latency_budget: Optional[LatencyBudget] = None
    lifespan: Optional[Lifespan] = None
    liveliness: LivelinessKind = LivelinessKind.AUTOMATIC
    destination_order: DestinationOrderKind = DestinationOrderKind.BY_RECEPTION_TIMESTAMP
    ownership: str = "SHARED"
    
    # Additional QoS parameters
    resource_limits: Optional[Dict[str, int]] = None
    partition: Optional[List[str]] = None
    presentation: Optional[Dict[str, Any]] = None
    time_based_filter: Optional[Dict[str, int]] = None
    user_data: Optional[str] = None
    group_data: Optional[str] = None
    topic_data: Optional[str] = None
    
    def __post_init__(self):
        """Convert string enum values to proper enum types"""
        if isinstance(self.reliability, str):
            self.reliability = ReliabilityKind(self.reliability)
        if isinstance(self.durability, str):
            self.durability = DurabilityKind(self.durability)
        if isinstance(self.liveliness, str):
            self.liveliness = LivelinessKind(self.liveliness)
        if isinstance(self.destination_order, str):
            self.destination_order = DestinationOrderKind(self.destination_order)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert QoS to dictionary"""
        result = {
            "reliability": self.reliability.value,
            "durability": self.durability.value,
            "history": {
                "kind": self.history.kind.value,
                "depth": self.history.depth
            },
            "liveliness": self.liveliness.value,
            "destination_order": self.destination_order.value,
            "ownership": self.ownership,
        }
        
        if self.deadline:
            result["deadline"] = self.deadline.to_duration()
        if self.latency_budget:
            result["latency_budget"] = {
                "sec": self.latency_budget.sec,
                "nanosec": self.latency_budget.nanosec
            }
        if self.lifespan and not self.lifespan.is_infinite():
            result["lifespan"] = {
                "sec": self.lifespan.sec,
                "nanosec": self.lifespan.nanosec
            }
        if self.resource_limits:
            result["resource_limits"] = self.resource_limits
        if self.partition:
            result["partition"] = self.partition
        if self.user_data:
            result["user_data"] = self.user_data
        if self.group_data:
            result["group_data"] = self.group_data
        if self.topic_data:
            result["topic_data"] = self.topic_data
            
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "QoS":
        """Create QoS from dictionary"""
        kwargs = {
            "reliability": ReliabilityKind(data.get("reliability", "RELIABLE")),
            "durability": DurabilityKind(data.get("durability", "VOLATILE")),
            "liveliness": LivelinessKind(data.get("liveliness", "AUTOMATIC")),
            "destination_order": DestinationOrderKind(data.get("destination_order", "BY_RECEPTION_TIMESTAMP")),
            "ownership": data.get("ownership", "SHARED"),
        }
        
        # Parse history
        if "history" in data:
            history_data = data["history"]
            if isinstance(history_data, dict):
                kwargs["history"] = History(
                    kind=HistoryKind(history_data.get("kind", "KEEP_LAST")),
                    depth=history_data.get("depth", 1)
                )
            else:
                kwargs["history"] = History(kind=HistoryKind(history_data))
        
        # Parse deadline
        if "deadline" in data:
            deadline_data = data["deadline"]
            if isinstance(deadline_data, dict):
                kwargs["deadline"] = Deadline(
                    sec=deadline_data.get("sec", 0),
                    nanosec=deadline_data.get("nanosec", 0)
                )
            else:
                kwargs["deadline"] = Deadline(nanosec=int(deadline_data * 1e9) if deadline_data < 1 else int(deadline_data))
        
        # Parse latency_budget
        if "latency_budget" in data:
            lb_data = data["latency_budget"]
            kwargs["latency_budget"] = LatencyBudget(
                sec=lb_data.get("sec", 0),
                nanosec=lb_data.get("nanosec", 0)
            )
        
        # Parse lifespan
        if "lifespan" in data:
            ls_data = data["lifespan"]
            kwargs["lifespan"] = Lifespan(
                sec=ls_data.get("sec", -1),
                nanosec=ls_data.get("nanosec", 0)
            )
        
        # Copy other fields
        for field_name in ["resource_limits", "partition", "presentation", 
                          "time_based_filter", "user_data", "group_data", "topic_data"]:
            if field_name in data:
                kwargs[field_name] = data[field_name]
        
        return cls(**kwargs)


@dataclass
class Topic:
    """DDS Topic configuration"""
    name: str
    type: str
    qos: QoS = field(default_factory=QoS)
    description: Optional[str] = None
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            "name": self.name,
            "type": self.type,
            "qos": self.qos.to_dict()
        }
        if self.description:
            result["description"] = self.description
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Topic":
        return cls(
            name=data["name"],
            type=data["type"],
            qos=QoS.from_dict(data.get("qos", {})),
            description=data.get("description")
        )


@dataclass
class Publisher:
    """DDS Publisher configuration"""
    topic: str
    type: Optional[str] = None
    qos: QoS = field(default_factory=QoS)
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            "topic": self.topic,
            "qos": self.qos.to_dict()
        }
        if self.type:
            result["type"] = self.type
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Publisher":
        return cls(
            topic=data["topic"],
            type=data.get("type"),
            qos=QoS.from_dict(data.get("qos", {}))
        )


@dataclass
class Subscriber:
    """DDS Subscriber configuration"""
    topic: str
    type: Optional[str] = None
    qos: QoS = field(default_factory=QoS)
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            "topic": self.topic,
            "qos": self.qos.to_dict()
        }
        if self.type:
            result["type"] = self.type
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Subscriber":
        return cls(
            topic=data["topic"],
            type=data.get("type"),
            qos=QoS.from_dict(data.get("qos", {}))
        )


@dataclass
class Participant:
    """DDS DomainParticipant configuration"""
    name: str
    qos_profile: Optional[str] = None
    publishers: List[Publisher] = field(default_factory=list)
    subscribers: List[Subscriber] = field(default_factory=list)
    domain_id: Optional[int] = None
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            "name": self.name,
            "publishers": [p.to_dict() for p in self.publishers],
            "subscribers": [s.to_dict() for s in self.subscribers]
        }
        if self.qos_profile:
            result["qos_profile"] = self.qos_profile
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Participant":
        return cls(
            name=data["name"],
            qos_profile=data.get("qos_profile"),
            publishers=[Publisher.from_dict(p) for p in data.get("publishers", [])],
            subscribers=[Subscriber.from_dict(s) for s in data.get("subscribers", [])],
            domain_id=data.get("domain_id")
        )


@dataclass
class Domain:
    """DDS Domain configuration"""
    id: int
    name: str
    participants: List[Participant] = field(default_factory=list)
    description: Optional[str] = None
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            "id": self.id,
            "name": self.name,
            "participants": [p.to_dict() for p in self.participants]
        }
        if self.description:
            result["description"] = self.description
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Domain":
        return cls(
            id=data["id"],
            name=data["name"],
            participants=[Participant.from_dict(p) for p in data.get("participants", [])],
            description=data.get("description")
        )


@dataclass
class Transport:
    """DDS Transport configuration"""
    kind: TransportKind = TransportKind.UDP
    interface: Optional[str] = None
    multicast_address: Optional[str] = None
    port_base: int = 7400
    port_range: int = 100
    
    # Additional transport parameters
    buffer_size: int = 65535
    max_message_size: int = 65535
    heartbeat_period_ms: int = 1000
    
    def __post_init__(self):
        """Convert string kind to TransportKind enum if needed"""
        if isinstance(self.kind, str):
            self.kind = TransportKind(self.kind)
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            "kind": self.kind.value,
            "port_base": self.port_base,
            "port_range": self.port_range,
            "buffer_size": self.buffer_size,
            "max_message_size": self.max_message_size,
            "heartbeat_period_ms": self.heartbeat_period_ms
        }
        if self.interface:
            result["interface"] = self.interface
        if self.multicast_address:
            result["multicast_address"] = self.multicast_address
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Transport":
        return cls(
            kind=TransportKind(data.get("kind", "UDP")),
            interface=data.get("interface"),
            multicast_address=data.get("multicast_address"),
            port_base=data.get("port_base", 7400),
            port_range=data.get("port_range", 100),
            buffer_size=data.get("buffer_size", 65535),
            max_message_size=data.get("max_message_size", 65535),
            heartbeat_period_ms=data.get("heartbeat_period_ms", 1000)
        )


@dataclass
class TSNStream:
    """TSN Stream configuration"""
    name: str
    priority: int = 0
    bandwidth: int = 0  # bits per second
    max_latency: int = 0  # microseconds
    frame_size: int = 1500
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "name": self.name,
            "priority": self.priority,
            "bandwidth": self.bandwidth,
            "max_latency": self.max_latency,
            "frame_size": self.frame_size
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "TSNStream":
        return cls(
            name=data["name"],
            priority=data.get("priority", 0),
            bandwidth=data.get("bandwidth", 0),
            max_latency=data.get("max_latency", 0),
            frame_size=data.get("frame_size", 1500)
        )


@dataclass
class TSN:
    """TSN configuration"""
    enabled: bool = False
    streams: List[TSNStream] = field(default_factory=list)
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "enabled": self.enabled,
            "streams": [s.to_dict() for s in self.streams]
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "TSN":
        return cls(
            enabled=data.get("enabled", False),
            streams=[TSNStream.from_dict(s) for s in data.get("streams", [])]
        )


@dataclass
class Security:
    """DDS Security configuration"""
    enabled: bool = False
    authentication: bool = False
    encryption: bool = False
    access_control: bool = False
    logging: bool = False
    
    # Certificate paths
    identity_ca: Optional[str] = None
    permissions_ca: Optional[str] = None
    private_key: Optional[str] = None
    identity_certificate: Optional[str] = None
    permissions_xml: Optional[str] = None
    governance_xml: Optional[str] = None
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            "enabled": self.enabled,
            "authentication": self.authentication,
            "encryption": self.encryption,
            "access_control": self.access_control,
            "logging": self.logging
        }
        if self.identity_ca:
            result["identity_ca"] = self.identity_ca
        if self.permissions_ca:
            result["permissions_ca"] = self.permissions_ca
        if self.private_key:
            result["private_key"] = self.private_key
        if self.identity_certificate:
            result["identity_certificate"] = self.identity_certificate
        if self.permissions_xml:
            result["permissions_xml"] = self.permissions_xml
        if self.governance_xml:
            result["governance_xml"] = self.governance_xml
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Security":
        return cls(
            enabled=data.get("enabled", False),
            authentication=data.get("authentication", False),
            encryption=data.get("encryption", False),
            access_control=data.get("access_control", False),
            logging=data.get("logging", False),
            identity_ca=data.get("identity_ca"),
            permissions_ca=data.get("permissions_ca"),
            private_key=data.get("private_key"),
            identity_certificate=data.get("identity_certificate"),
            permissions_xml=data.get("permissions_xml"),
            governance_xml=data.get("governance_xml")
        )


@dataclass
class SystemInfo:
    """System information"""
    name: str
    version: str = "1.0.0"
    description: Optional[str] = None
    vendor: Optional[str] = None
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            "name": self.name,
            "version": self.version
        }
        if self.description:
            result["description"] = self.description
        if self.vendor:
            result["vendor"] = self.vendor
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "SystemInfo":
        return cls(
            name=data["name"],
            version=data.get("version", "1.0.0"),
            description=data.get("description"),
            vendor=data.get("vendor")
        )


@dataclass
class DDSConfig:
    """
    Root DDS Configuration class
    
    This is the main configuration container that holds all DDS configuration
    including system info, domains, participants, topics, transport, TSN and security.
    """
    system: SystemInfo
    domains: List[Domain] = field(default_factory=list)
    transport: Transport = field(default_factory=Transport)
    tsn: TSN = field(default_factory=TSN)
    security: Security = field(default_factory=Security)
    qos_profiles: Dict[str, QoS] = field(default_factory=dict)
    topics: List[Topic] = field(default_factory=list)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert configuration to dictionary"""
        result = {
            "system": self.system.to_dict(),
            "domains": [d.to_dict() for d in self.domains],
            "transport": self.transport.to_dict(),
            "tsn": self.tsn.to_dict(),
            "security": self.security.to_dict()
        }
        
        if self.qos_profiles:
            result["qos_profiles"] = {
                name: qos.to_dict() for name, qos in self.qos_profiles.items()
            }
        
        if self.topics:
            result["topics"] = [t.to_dict() for t in self.topics]
            
        return result
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "DDSConfig":
        """Create DDSConfig from dictionary"""
        return cls(
            system=SystemInfo.from_dict(data["system"]),
            domains=[Domain.from_dict(d) for d in data.get("domains", [])],
            transport=Transport.from_dict(data.get("transport", {})),
            tsn=TSN.from_dict(data.get("tsn", {})),
            security=Security.from_dict(data.get("security", {})),
            qos_profiles={
                name: QoS.from_dict(qos) 
                for name, qos in data.get("qos_profiles", {}).items()
            },
            topics=[Topic.from_dict(t) for t in data.get("topics", [])]
        )
    
    def get_all_topics(self) -> List[Topic]:
        """Get all topics from configuration"""
        topics = list(self.topics)
        
        # Also collect topics from publishers/subscribers
        for domain in self.domains:
            for participant in domain.participants:
                for pub in participant.publishers:
                    if pub.type:
                        existing = next((t for t in topics if t.name == pub.topic), None)
                        if not existing:
                            topics.append(Topic(name=pub.topic, type=pub.type))
                for sub in participant.subscribers:
                    if sub.type:
                        existing = next((t for t in topics if t.name == sub.topic), None)
                        if not existing:
                            topics.append(Topic(name=sub.topic, type=sub.type))
        
        return topics
    
    def get_participant_count(self) -> int:
        """Get total participant count across all domains"""
        return sum(len(d.participants) for d in self.domains)
    
    def validate(self) -> tuple[bool, List[str]]:
        """
        Basic validation of configuration
        
        Returns:
            Tuple of (is_valid, error_messages)
        """
        errors = []
        
        # Check system name
        if not self.system.name:
            errors.append("System name is required")
        
        # Check domains
        if not self.domains:
            errors.append("At least one domain is required")
        else:
            domain_ids = set()
            for domain in self.domains:
                if domain.id in domain_ids:
                    errors.append(f"Duplicate domain ID: {domain.id}")
                domain_ids.add(domain.id)
                
                if domain.id < 0 or domain.id > 232:
                    errors.append(f"Domain ID {domain.id} out of range (0-232)")
        
        return len(errors) == 0, errors
