#!/usr/bin/env python3
"""
Data Flow Analyzer for DDS Systems
Analyzes data flows, detects cycles, and validates end-to-end paths
"""

from typing import List, Dict, Optional, Set, Tuple, Any
from dataclasses import dataclass, field
from collections import defaultdict, deque
import json


@dataclass
class DataFlowNode:
    """Node in data flow graph"""
    id: str
    name: str
    type: str  # 'publisher', 'subscriber', 'topic', 'component', 'gateway'
    domain_id: int = 0
    qos_profile: Optional[str] = None
    metadata: Dict[str, Any] = field(default_factory=dict)


@dataclass
class DataFlowEdge:
    """Edge in data flow graph"""
    source: str
    target: str
    topic: Optional[str] = None
    data_type: Optional[str] = None
    qos_profile: Optional[str] = None
    bandwidth: Optional[float] = None  # bytes/second
    latency: Optional[float] = None  # milliseconds


@dataclass
class DataFlowPath:
    """End-to-end data flow path"""
    source: str
    target: str
    nodes: List[str]
    edges: List[DataFlowEdge]
    total_latency: float = 0.0
    total_bandwidth: float = 0.0
    hops: int = 0


@dataclass
class DataFlowCycle:
    """Detected cycle in data flow"""
    nodes: List[str]
    edges: List[DataFlowEdge]
    length: int
    is_hazardous: bool = False


class DataFlowAnalyzer:
    """
    Analyzes data flows in DDS configurations
    
    Capabilities:
    - Build data flow graph from configuration
    - Detect cycles and feedback loops
    - Find all paths between nodes
    - Calculate end-to-end latency
    - Analyze bandwidth requirements
    - Validate data type consistency
    """
    
    def __init__(self):
        self.nodes: Dict[str, DataFlowNode] = {}
        self.edges: List[DataFlowEdge] = []
        self.adjacency: Dict[str, List[str]] = defaultdict(list)
        self.edge_map: Dict[Tuple[str, str], DataFlowEdge] = {}
        
    def build_from_config(self, config: Dict[str, Any]):
        """Build data flow graph from DDS configuration"""
        self.nodes.clear()
        self.edges.clear()
        self.adjacency.clear()
        self.edge_map.clear()
        
        # Add topics as nodes
        for topic in config.get('topics', []):
            node = DataFlowNode(
                id=f"topic:{topic['name']}",
                name=topic['name'],
                type='topic',
                domain_id=topic.get('domain_id', 0),
                qos_profile=topic.get('qos_profile')
            )
            self.add_node(node)
        
        # Add participants and their endpoints
        for participant in config.get('participants', []):
            p_id = f"participant:{participant['name']}"
            p_node = DataFlowNode(
                id=p_id,
                name=participant['name'],
                type='participant',
                domain_id=participant.get('domain_id', 0)
            )
            self.add_node(p_node)
            
            # Add publishers
            for pub in participant.get('publishers', []):
                topic_name = pub.get('topic')
                if topic_name:
                    edge = DataFlowEdge(
                        source=p_id,
                        target=f"topic:{topic_name}",
                        topic=topic_name,
                        data_type=pub.get('type'),
                        qos_profile=pub.get('qos_profile')
                    )
                    self.add_edge(edge)
            
            # Add subscribers
            for sub in participant.get('subscribers', []):
                topic_name = sub.get('topic')
                if topic_name:
                    edge = DataFlowEdge(
                        source=f"topic:{topic_name}",
                        target=p_id,
                        topic=topic_name,
                        data_type=sub.get('type'),
                        qos_profile=sub.get('qos_profile')
                    )
                    self.add_edge(edge)
        
        # Add components (from AUTOSAR integration)
        for component in config.get('components', []):
            c_id = f"component:{component['name']}"
            c_node = DataFlowNode(
                id=c_id,
                name=component['name'],
                type='component'
            )
            self.add_node(c_node)
            
            # Connect components to topics based on ports
            for port in component.get('ports', []):
                topic_name = port.get('topic')
                if topic_name:
                    if port.get('is_publisher'):
                        edge = DataFlowEdge(
                            source=c_id,
                            target=f"topic:{topic_name}",
                            topic=topic_name
                        )
                    else:
                        edge = DataFlowEdge(
                            source=f"topic:{topic_name}",
                            target=c_id,
                            topic=topic_name
                        )
                    self.add_edge(edge)
        
        # Add gateways
        for gateway in config.get('gateways', []):
            g_id = f"gateway:{gateway['name']}"
            g_node = DataFlowNode(
                id=g_id,
                name=gateway['name'],
                type='gateway'
            )
            self.add_node(g_node)
            
            # Connect gateways between domains
            for connection in gateway.get('connections', []):
                from_topic = connection.get('from_topic')
                to_topic = connection.get('to_topic')
                if from_topic and to_topic:
                    # Inbound to gateway
                    self.add_edge(DataFlowEdge(
                        source=f"topic:{from_topic}",
                        target=g_id,
                        topic=from_topic
                    ))
                    # Outbound from gateway
                    self.add_edge(DataFlowEdge(
                        source=g_id,
                        target=f"topic:{to_topic}",
                        topic=to_topic
                    ))
    
    def add_node(self, node: DataFlowNode):
        """Add a node to the graph"""
        self.nodes[node.id] = node
        
    def add_edge(self, edge: DataFlowEdge):
        """Add an edge to the graph"""
        self.edges.append(edge)
        self.adjacency[edge.source].append(edge.target)
        self.edge_map[(edge.source, edge.target)] = edge
    
    def find_cycles(self) -> List[DataFlowCycle]:
        """Find all cycles in the data flow graph"""
        cycles = []
        visited = set()
        rec_stack = []
        rec_stack_set = set()
        
        def dfs(node_id: str, path: List[str]):
            visited.add(node_id)
            rec_stack.append(node_id)
            rec_stack_set.add(node_id)
            
            for neighbor in self.adjacency[node_id]:
                if neighbor not in visited:
                    dfs(neighbor, path + [neighbor])
                elif neighbor in rec_stack_set:
                    # Found a cycle
                    cycle_start = rec_stack.index(neighbor)
                    cycle_nodes = rec_stack[cycle_start:]
                    
                    # Build edge list
                    cycle_edges = []
                    for i in range(len(cycle_nodes)):
                        src = cycle_nodes[i]
                        tgt = cycle_nodes[(i + 1) % len(cycle_nodes)]
                        if (src, tgt) in self.edge_map:
                            cycle_edges.append(self.edge_map[(src, tgt)])
                    
                    # Determine if hazardous (e.g., control loops without delay)
                    is_hazardous = self._is_hazardous_cycle(cycle_nodes, cycle_edges)
                    
                    cycle = DataFlowCycle(
                        nodes=cycle_nodes,
                        edges=cycle_edges,
                        length=len(cycle_nodes),
                        is_hazardous=is_hazardous
                    )
                    cycles.append(cycle)
            
            rec_stack.pop()
            rec_stack_set.remove(node_id)
        
        for node_id in self.nodes:
            if node_id not in visited:
                dfs(node_id, [node_id])
        
        return cycles
    
    def _is_hazardous_cycle(self, nodes: List[str], edges: List[DataFlowEdge]) -> bool:
        """Determine if a cycle is potentially hazardous"""
        # Check for control-related naming
        control_keywords = ['control', 'command', 'feedback', 'pid']
        for node_id in nodes:
            node = self.nodes.get(node_id)
            if node and any(kw in node.name.lower() for kw in control_keywords):
                # Check if there's a delay/buffer in the cycle
                has_delay = False
                for edge in edges:
                    if edge.qos_profile and 'keep_all' in edge.qos_profile.lower():
                        has_delay = True
                        break
                if not has_delay:
                    return True
        return False
    
    def find_all_paths(self, source: str, target: str, 
                       max_paths: int = 10) -> List[DataFlowPath]:
        """Find all paths between source and target nodes"""
        paths = []
        
        def dfs(current: str, target: str, visited: Set[str], 
                path_nodes: List[str], path_edges: List[DataFlowEdge]):
            if len(paths) >= max_paths:
                return
            
            if current == target:
                # Calculate totals
                total_latency = sum(e.latency or 0 for e in path_edges)
                max_bandwidth = max((e.bandwidth or 0) for e in path_edges) if path_edges else 0
                
                path = DataFlowPath(
                    source=source,
                    target=target,
                    nodes=path_nodes.copy(),
                    edges=path_edges.copy(),
                    total_latency=total_latency,
                    total_bandwidth=max_bandwidth,
                    hops=len(path_edges)
                )
                paths.append(path)
                return
            
            visited.add(current)
            
            for neighbor in self.adjacency[current]:
                if neighbor not in visited:
                    edge = self.edge_map.get((current, neighbor))
                    if edge:
                        path_edges.append(edge)
                        path_nodes.append(neighbor)
                        
                        dfs(neighbor, target, visited, path_nodes, path_edges)
                        
                        path_edges.pop()
                        path_nodes.pop()
            
            visited.remove(current)
        
        if source in self.nodes and target in self.nodes:
            dfs(source, target, {source}, [source], [])
        
        return paths
    
    def get_upstream_nodes(self, node_id: str, depth: int = -1) -> List[str]:
        """Get all upstream nodes (data sources) for a given node"""
        upstream = []
        visited = set()
        queue = deque([(node_id, 0)])
        
        # Build reverse adjacency
        reverse_adj = defaultdict(list)
        for edge in self.edges:
            reverse_adj[edge.target].append(edge.source)
        
        while queue:
            current, current_depth = queue.popleft()
            
            if current in visited:
                continue
            visited.add(current)
            
            if current != node_id:
                upstream.append(current)
            
            if depth < 0 or current_depth < depth:
                for neighbor in reverse_adj[current]:
                    if neighbor not in visited:
                        queue.append((neighbor, current_depth + 1))
        
        return upstream
    
    def get_downstream_nodes(self, node_id: str, depth: int = -1) -> List[str]:
        """Get all downstream nodes (data sinks) for a given node"""
        downstream = []
        visited = set()
        queue = deque([(node_id, 0)])
        
        while queue:
            current, current_depth = queue.popleft()
            
            if current in visited:
                continue
            visited.add(current)
            
            if current != node_id:
                downstream.append(current)
            
            if depth < 0 or current_depth < depth:
                for neighbor in self.adjacency[current]:
                    if neighbor not in visited:
                        queue.append((neighbor, current_depth + 1))
        
        return downstream
    
    def analyze_qos_consistency(self) -> List[Dict[str, Any]]:
        """Analyze QoS consistency across data flows"""
        issues = []
        
        # Group edges by topic
        topic_edges = defaultdict(list)
        for edge in self.edges:
            if edge.topic:
                topic_edges[edge.topic].append(edge)
        
        # Check QoS consistency for each topic
        for topic, edges in topic_edges.items():
            qos_profiles = set(e.qos_profile for e in edges if e.qos_profile)
            
            if len(qos_profiles) > 1:
                issues.append({
                    'type': 'qos_inconsistency',
                    'topic': topic,
                    'message': f"Topic '{topic}' has multiple QoS profiles: {qos_profiles}",
                    'edges': [(e.source, e.target, e.qos_profile) for e in edges]
                })
        
        return issues
    
    def analyze_type_consistency(self) -> List[Dict[str, Any]]:
        """Analyze data type consistency"""
        issues = []
        
        # Group edges by topic
        topic_edges = defaultdict(list)
        for edge in self.edges:
            if edge.topic:
                topic_edges[edge.topic].append(edge)
        
        # Check type consistency
        for topic, edges in topic_edges.items():
            data_types = set(e.data_type for e in edges if e.data_type)
            
            if len(data_types) > 1:
                issues.append({
                    'type': 'type_mismatch',
                    'topic': topic,
                    'message': f"Topic '{topic}' has inconsistent data types: {data_types}",
                    'edges': [(e.source, e.target, e.data_type) for e in edges]
                })
        
        return issues
    
    def get_domain_isolation(self) -> Dict[int, Set[str]]:
        """Get nodes isolated by domain"""
        domain_nodes = defaultdict(set)
        
        for node_id, node in self.nodes.items():
            domain_nodes[node.domain_id].add(node_id)
        
        return dict(domain_nodes)
    
    def find_orphaned_topics(self) -> List[str]:
        """Find topics with no publishers or subscribers"""
        orphaned = []
        
        for node_id, node in self.nodes.items():
            if node.type == 'topic':
                has_publisher = any(
                    e.target == node_id and self.nodes[e.source].type == 'participant'
                    for e in self.edges
                )
                has_subscriber = any(
                    e.source == node_id and self.nodes[e.target].type == 'participant'
                    for e in self.edges
                )
                
                if not has_publisher or not has_subscriber:
                    orphaned.append({
                        'topic': node.name,
                        'has_publisher': has_publisher,
                        'has_subscriber': has_subscriber
                    })
        
        return orphaned
    
    def generate_report(self) -> Dict[str, Any]:
        """Generate comprehensive data flow analysis report"""
        cycles = self.find_cycles()
        qos_issues = self.analyze_qos_consistency()
        type_issues = self.analyze_type_consistency()
        orphaned = self.find_orphaned_topics()
        domain_isolation = self.get_domain_isolation()
        
        return {
            'summary': {
                'nodes': len(self.nodes),
                'edges': len(self.edges),
                'cycles': len(cycles),
                'hazardous_cycles': sum(1 for c in cycles if c.is_hazardous),
                'qos_issues': len(qos_issues),
                'type_issues': len(type_issues),
                'orphaned_topics': len(orphaned),
                'domains': len(domain_isolation)
            },
            'cycles': [
                {
                    'nodes': c.nodes,
                    'length': c.length,
                    'is_hazardous': c.is_hazardous
                }
                for c in cycles
            ],
            'qos_issues': qos_issues,
            'type_issues': type_issues,
            'orphaned_topics': orphaned,
            'domain_isolation': {
                str(k): list(v) for k, v in domain_isolation.items()
            }
        }
    
    def to_dict(self) -> Dict[str, Any]:
        """Export graph to dictionary"""
        return {
            'nodes': [
                {
                    'id': n.id,
                    'name': n.name,
                    'type': n.type,
                    'domain_id': n.domain_id,
                    'qos_profile': n.qos_profile
                }
                for n in self.nodes.values()
            ],
            'edges': [
                {
                    'source': e.source,
                    'target': e.target,
                    'topic': e.topic,
                    'data_type': e.data_type,
                    'qos_profile': e.qos_profile
                }
                for e in self.edges
            ]
        }
    
    def export_to_dot(self) -> str:
        """Export graph to Graphviz DOT format"""
        lines = ['digraph DataFlow {', '  rankdir=LR;']
        
        # Node definitions
        for node_id, node in self.nodes.items():
            safe_id = node_id.replace(':', '_').replace('-', '_')
            shape = 'box' if node.type == 'topic' else 'ellipse'
            color = {
                'topic': 'lightblue',
                'participant': 'lightgreen',
                'component': 'lightyellow',
                'gateway': 'lightcoral'
            }.get(node.type, 'white')
            
            lines.append(f'  {safe_id} [label="{node.name}", shape={shape}, style=filled, fillcolor={color}];')
        
        # Edge definitions
        for edge in self.edges:
            src = edge.source.replace(':', '_').replace('-', '_')
            tgt = edge.target.replace(':', '_').replace('-', '_')
            label = edge.topic or ''
            lines.append(f'  {src} -> {tgt} [label="{label}"];')
        
        lines.append('}')
        return '\n'.join(lines)


# Example usage
if __name__ == '__main__':
    analyzer = DataFlowAnalyzer()
    
    test_config = {
        'topics': [
            {'name': 'Speed', 'domain_id': 0},
            {'name': 'Control', 'domain_id': 0}
        ],
        'participants': [
            {
                'name': 'Sensor',
                'domain_id': 0,
                'publishers': [{'topic': 'Speed', 'type': 'float'}]
            },
            {
                'name': 'Controller',
                'domain_id': 0,
                'subscribers': [{'topic': 'Speed', 'type': 'float'}],
                'publishers': [{'topic': 'Control', 'type': 'float'}]
            },
            {
                'name': 'Actuator',
                'domain_id': 0,
                'subscribers': [{'topic': 'Control', 'type': 'float'}]
            }
        ]
    }
    
    analyzer.build_from_config(test_config)
    
    print("Data Flow Analysis Report")
    print("=" * 50)
    
    report = analyzer.generate_report()
    print(f"Nodes: {report['summary']['nodes']}")
    print(f"Edges: {report['summary']['edges']}")
    print(f"Cycles: {report['summary']['cycles']}")
    
    # Print DOT format
    print("\nGraphviz DOT:")
    print(analyzer.export_to_dot())