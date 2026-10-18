"""
Network Management (NM) configuration parser and generator.

Supports:
- NM matrix file parsing (CSV/JSON format)
- AUTOSAR NM module configuration generation
- CanSM (CAN State Manager) configuration
- EcuM (ECU State Manager) configuration
"""

import json
import csv
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Any
from io import StringIO


@dataclass
class NmNode:
    """Network Management Node definition"""
    name: str
    node_id: int
    ecu_name: str = ""
    sleep_mode: bool = False
    comment: str = ""


@dataclass
class NmMessage:
    """NM Message definition"""
    id: int
    name: str
    length: int  # DLC
    cycle_time_ms: int
    sender: str
    receivers: List[str] = field(default_factory=list)
    payload: Dict[str, Any] = field(default_factory=dict)
    comment: str = ""


@dataclass
class NmCluster:
    """NM Cluster definition"""
    name: str
    protocol: str  # "CAN", "LIN", "Ethernet"
    nodes: List[NmNode] = field(default_factory=list)
    messages: List[NmMessage] = field(default_factory=list)
    config: Dict[str, Any] = field(default_factory=dict)


@dataclass
class NmConfig:
    """Complete NM configuration"""
    version: str = "1.0"
    clusters: List[NmCluster] = field(default_factory=list)
    global_config: Dict[str, Any] = field(default_factory=dict)


class NmMatrixParser:
    """Parse NM matrix from CSV or JSON format"""

    def parse_csv(self, content: str) -> NmConfig:
        """Parse NM matrix from CSV content"""
        config = NmConfig()
        cluster = NmCluster(name="DefaultCluster", protocol="CAN")

        reader = csv.DictReader(StringIO(content))

        for row in reader:
            # Parse NM message
            msg = NmMessage(
                id=int(row.get('MessageID', row.get('ID', 0))),
                name=row.get('MessageName', row.get('Name', '')),
                length=int(row.get('DLC', row.get('Length', 8))),
                cycle_time_ms=int(row.get('CycleTime', row.get('CycleTimeMs', 100))),
                sender=row.get('Sender', row.get('Transmitter', '')),
                receivers=[r.strip() for r in row.get('Receivers', '').split(',') if r.strip()],
                comment=row.get('Comment', ''),
            )

            # Parse NM node if not exists
            if msg.sender and not any(n.name == msg.sender for n in cluster.nodes):
                node = NmNode(
                    name=msg.sender,
                    node_id=len(cluster.nodes),
                    ecu_name=msg.sender,
                )
                cluster.nodes.append(node)

            for receiver in msg.receivers:
                if not any(n.name == receiver for n in cluster.nodes):
                    node = NmNode(
                        name=receiver,
                        node_id=len(cluster.nodes),
                        ecu_name=receiver,
                    )
                    cluster.nodes.append(node)

            cluster.messages.append(msg)

        config.clusters.append(cluster)
        return config

    def parse_json(self, content: str) -> NmConfig:
        """Parse NM matrix from JSON content"""
        data = json.loads(content)
        config = NmConfig()

        for cluster_data in data.get('clusters', []):
            cluster = NmCluster(
                name=cluster_data.get('name', 'DefaultCluster'),
                protocol=cluster_data.get('protocol', 'CAN'),
                config=cluster_data.get('config', {}),
            )

            # Parse nodes
            for node_data in cluster_data.get('nodes', []):
                node = NmNode(
                    name=node_data['name'],
                    node_id=node_data.get('id', len(cluster.nodes)),
                    ecu_name=node_data.get('ecu_name', node_data['name']),
                    sleep_mode=node_data.get('sleep_mode', False),
                    comment=node_data.get('comment', ''),
                )
                cluster.nodes.append(node)

            # Parse messages
            for msg_data in cluster_data.get('messages', []):
                msg = NmMessage(
                    id=msg_data['id'],
                    name=msg_data['name'],
                    length=msg_data.get('length', 8),
                    cycle_time_ms=msg_data.get('cycle_time_ms', 100),
                    sender=msg_data.get('sender', ''),
                    receivers=msg_data.get('receivers', []),
                    payload=msg_data.get('payload', {}),
                    comment=msg_data.get('comment', ''),
                )
                cluster.messages.append(msg)

            config.clusters.append(cluster)

        config.global_config = data.get('global_config', {})
        config.version = data.get('version', '1.0')

        return config


class AutosarNmGenerator:
    """Generate AUTOSAR NM configuration from parsed NM matrix"""

    def __init__(self, config: NmConfig):
        self.config = config

    def generate_nm_config(self) -> Dict[str, Any]:
        """Generate Nm module configuration"""
        nm_config = {
            'NmGlobalConfig': {
                'NmVersionInfoApi': False,
                'NmDevErrorDetect': True,
                'NmPassiveModeStartup': False,
                'NmMainFunctionPeriod': 0.01,  # 10ms
                'NmBusLoadReduction': False,
            },
            'NmClusters': [],
        }

        for cluster in self.config.clusters:
            nm_cluster = {
                'NmClusterName': cluster.name,
                'NmProtocol': cluster.protocol,
                'NmNodes': [],
                'NmMessages': [],
            }

            # Generate node configs
            for node in cluster.nodes:
                nm_node = {
                    'NmNodeName': node.name,
                    'NmNodeId': node.node_id,
                    'NmEcuName': node.ecu_name,
                    'NmSleepMode': node.sleep_mode,
                    'NmNetworkMode': 'NM_NETWORK_MODE',
                }
                nm_cluster['NmNodes'].append(nm_node)

            # Generate message configs
            for msg in cluster.messages:
                nm_msg = {
                    'NmMessageName': msg.name,
                    'NmMessageId': msg.id,
                    'NmMessageLength': msg.length,
                    'NmCycleTimeMs': msg.cycle_time_ms,
                    'NmSender': msg.sender,
                    'NmReceivers': msg.receivers,
                    'NmPduNid': msg.id,
                }
                nm_cluster['NmMessages'].append(nm_msg)

            nm_config['NmClusters'].append(nm_cluster)

        return nm_config

    def generate_cansm_config(self) -> Dict[str, Any]:
        """Generate CanSM (CAN State Manager) configuration"""
        cansm_config = {
            'CanSMGlobalConfig': {
                'CanSMVersionInfoApi': False,
                'CanSMDevErrorDetect': True,
                'CanSMMainFunctionPeriod': 0.01,
            },
            'CanSMNetworks': [],
        }

        for cluster in self.config.clusters:
            if cluster.protocol != 'CAN':
                continue

            cansm_network = {
                'CanSMNetworkName': cluster.name,
                'CanSMBusOffRecoveryTime': 0.1,  # 100ms
                'CanSMFullCommTimeout': 2.0,  # 2s
                'CanSMLightTimeout': 0.5,  # 500ms
                'CanSMModeRequestTimeout': 1.0,
                'CanSMNetworkMode': 'FULL_COMMUNICATION',
            }
            cansm_config['CanSMNetworks'].append(cansm_network)

        return cansm_config

    def generate_ecum_config(self) -> Dict[str, Any]:
        """Generate EcuM (ECU State Manager) configuration"""
        ecum_config = {
            'EcuMGlobalConfig': {
                'EcuMVersionInfoApi': False,
                'EcuMDevErrorDetect': True,
                'EcuMMainFunctionPeriod': 0.01,
                'EcuMSleepMode': True,
                'EcuMWakeupSupport': True,
            },
            'EcuMStateConfig': {
                'EcuMStartupState': 'ECUM_STATE_STARTUP_ONE',
                'EcuMShutdownState': 'ECUM_STATE_SHUTDOWN',
                'EcuMSleepState': 'ECUM_STATE_SLEEP',
                'EcuMWakeupState': 'ECUM_STATE_WAKEUP',
            },
            'EcuMNetworkConfig': [],
        }

        # Generate network configs from NM clusters
        for cluster in self.config.clusters:
            network = {
                'EcuMNetworkName': cluster.name,
                'EcuMNetworkProtocol': cluster.protocol,
                'EcuMWakeupSources': [],
            }

            # Add wakeup sources from nodes
            for node in cluster.nodes:
                if node.sleep_mode:
                    wakeup_source = {
                        'EcuMWakeupSourceName': f"Wakeup_{node.name}",
                        'EcuMWakeupSourceId': node.node_id,
                        'EcuMWakeupValidationTimeout': 1.0,
                    }
                    network['EcuMWakeupSources'].append(wakeup_source)

            ecum_config['EcuMNetworkConfig'].append(network)

        return ecum_config

    def generate_complete_config(self) -> Dict[str, Any]:
        """Generate complete NM-related configuration"""
        return {
            'Nm': self.generate_nm_config(),
            'CanSM': self.generate_cansm_config(),
            'EcuM': self.generate_ecum_config(),
            'metadata': {
                'version': self.config.version,
                'cluster_count': len(self.config.clusters),
                'node_count': sum(len(c.nodes) for c in self.config.clusters),
                'message_count': sum(len(c.messages) for c in self.config.clusters),
            }
        }


def parse_nm_matrix(content: str, format: str = 'json') -> NmConfig:
    """Parse NM matrix from content"""
    parser = NmMatrixParser()
    if format == 'csv':
        return parser.parse_csv(content)
    else:
        return parser.parse_json(content)


def generate_autosar_nm_config(content: str, format: str = 'json') -> Dict[str, Any]:
    """Parse NM matrix and generate complete AUTOSAR configuration"""
    config = parse_nm_matrix(content, format)
    generator = AutosarNmGenerator(config)
    return generator.generate_complete_config()


if __name__ == '__main__':
    import sys

    # Test with sample JSON
    sample_json = json.dumps({
        "version": "1.0",
        "global_config": {
            "NmMainFunctionPeriod": 0.01,
            "NmBusLoadReduction": False,
        },
        "clusters": [
            {
                "name": "CAN_Cluster",
                "protocol": "CAN",
                "nodes": [
                    {"name": "ECU1", "id": 0, "ecu_name": "EngineECU", "sleep_mode": True},
                    {"name": "ECU2", "id": 1, "ecu_name": "GatewayECU", "sleep_mode": True},
                    {"name": "ECU3", "id": 2, "ecu_name": "DisplayECU", "sleep_mode": False},
                ],
                "messages": [
                    {
                        "id": 0x500,
                        "name": "NM_ECU1",
                        "length": 8,
                        "cycle_time_ms": 100,
                        "sender": "ECU1",
                        "receivers": ["ECU2", "ECU3"],
                        "comment": "NM message from ECU1",
                    },
                    {
                        "id": 0x501,
                        "name": "NM_ECU2",
                        "length": 8,
                        "cycle_time_ms": 100,
                        "sender": "ECU2",
                        "receivers": ["ECU1", "ECU3"],
                        "comment": "NM message from ECU2",
                    },
                ],
            }
        ]
    })

    result = generate_autosar_nm_config(sample_json, 'json')
    print(json.dumps(result, indent=2))
