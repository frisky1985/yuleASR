"""
DDS Configuration Data Models
"""
from dataclasses import dataclass, field
from typing import List, Optional, Dict, Any
from enum import Enum

class ReliabilityKind(Enum):
    BEST_EFFORT = "BEST_EFFORT"
    RELIABLE = "RELIABLE"

class DurabilityKind(Enum):
    VOLATILE = "VOLATILE"
    TRANSIENT_LOCAL = "TRANSIENT_LOCAL"
    TRANSIENT = "TRANSIENT"
    PERSISTENT = "PERSISTENT"

class HistoryKind(Enum):
    KEEP_LAST = "KEEP_LAST"
    KEEP_ALL = "KEEP_ALL"

class LivelinessKind(Enum):
    AUTOMATIC = "AUTOMATIC"
    MANUAL_BY_PARTICIPANT = "MANUAL_BY_PARTICIPANT"
    MANUAL_BY_TOPIC = "MANUAL_BY_TOPIC"

class OwnershipKind(Enum):
    SHARED = "SHARED"
    EXCLUSIVE = "EXCLUSIVE"

@dataclass
class HistoryQosPolicy:
    kind: HistoryKind = HistoryKind.KEEP_LAST
    depth: int = 1

@dataclass
class ReliabilityQosPolicy:
    kind: ReliabilityKind = ReliabilityKind.BEST_EFFORT
    max_blocking_time: int = 0  # nanoseconds

@dataclass
class DurabilityQosPolicy:
    kind: DurabilityKind = DurabilityKind.VOLATILE

@dataclass
class DeadlineQosPolicy:
    period: int = 0  # nanoseconds

@dataclass
class LatencyBudgetQosPolicy:
    duration: int = 0  # nanoseconds

@dataclass
class LivelinessQosPolicy:
    kind: LivelinessKind = LivelinessKind.AUTOMATIC
    lease_duration: int = 0  # nanoseconds

@dataclass
class OwnershipQosPolicy:
    kind: OwnershipKind = OwnershipKind.SHARED

@dataclass
class TopicQos:
    reliability: ReliabilityQosPolicy = field(default_factory=ReliabilityQosPolicy)
    durability: DurabilityQosPolicy = field(default_factory=DurabilityQosPolicy)
    history: HistoryQosPolicy = field(default_factory=HistoryQosPolicy)
    deadline: DeadlineQosPolicy = field(default_factory=DeadlineQosPolicy)
    latency_budget: LatencyBudgetQosPolicy = field(default_factory=LatencyBudgetQosPolicy)
    liveliness: LivelinessQosPolicy = field(default_factory=LivelinessQosPolicy)
    ownership: OwnershipQosPolicy = field(default_factory=OwnershipQosPolicy)

@dataclass
class DDSTopic:
    name: str
    data_type: str
    qos: TopicQos = field(default_factory=TopicQos)
    keyed: bool = False
    description: str = ""

@dataclass
class DDSParticipant:
    name: str
    domain_id: int = 0
    qos: Optional[Any] = None

@dataclass
class DDSDataWriter:
    name: str
    topic_name: str
    qos: TopicQos = field(default_factory=TopicQos)

@dataclass
class DDSDataReader:
    name: str
    topic_name: str
    qos: TopicQos = field(default_factory=TopicQos)

@dataclass
class DDSPublisher:
    name: str
    data_writers: List[DDSDataWriter] = field(default_factory=list)
    qos: Optional[Any] = None

@dataclass
class DDSSubscriber:
    name: str
    data_readers: List[DDSDataReader] = field(default_factory=list)
    qos: Optional[Any] = None

@dataclass
class DDSDomain:
    domain_id: int
    name: str
    description: str = ""
    participants: List[DDSParticipant] = field(default_factory=list)
    topics: List[DDSTopic] = field(default_factory=list)
    publishers: List[DDSPublisher] = field(default_factory=list)
    subscribers: List[DDSSubscriber] = field(default_factory=list)

@dataclass
class DDSConfiguration:
    name: str
    version: str = "1.0.0"
    domains: List[DDSDomain] = field(default_factory=list)
    idl_types: List[str] = field(default_factory=list)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert configuration to dictionary"""
        return {
            "name": self.name,
            "version": self.version,
            "domains": [
                {
                    "domain_id": d.domain_id,
                    "name": d.name,
                    "description": d.description,
                    "topics": [
                        {
                            "name": t.name,
                            "data_type": t.data_type,
                            "keyed": t.keyed,
                            "qos": {
                                "reliability": t.qos.reliability.kind.value,
                                "durability": t.qos.durability.kind.value,
                                "history": t.qos.history.kind.value
                            }
                        }
                        for t in d.topics
                    ],
                    "publishers": [p.name for p in d.publishers],
                    "subscribers": [s.name for s in d.subscribers]
                }
                for d in self.domains
            ],
            "idl_types": self.idl_types
        }
