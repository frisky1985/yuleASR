"""
DDS Runtime Client
Provides interface to the DDS runtime for monitoring and control
"""

import os
import json
import socket
import struct
import threading
from datetime import datetime
from typing import Dict, List, Optional, Callable

class DDSRuntimeClient:
    """Client for DDS runtime communication"""
    
    def __init__(self, socket_path: str = '/tmp/dds_runtime.sock'):
        self.socket_path = socket_path
        self.connected = False
        self.callbacks = {}
        self.monitor_thread = None
        self.running = False
        
    def connect(self) -> bool:
        """Connect to DDS runtime"""
        try:
            if os.path.exists(self.socket_path):
                self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                self.socket.connect(self.socket_path)
                self.connected = True
                return True
        except Exception as e:
            print(f"Failed to connect to DDS runtime: {e}")
            self.connected = False
        return False
    
    def disconnect(self):
        """Disconnect from DDS runtime"""
        self.running = False
        self.connected = False
        if self.socket:
            self.socket.close()
    
    def send_command(self, command: str, params: Dict = None) -> Dict:
        """Send command to DDS runtime"""
        if not self.connected:
            return {'success': False, 'error': 'Not connected'}
        
        try:
            message = json.dumps({
                'command': command,
                'params': params or {},
                'timestamp': datetime.now().isoformat()
            })
            
            # Send length-prefixed message
            message_bytes = message.encode('utf-8')
            length = struct.pack('!I', len(message_bytes))
            self.socket.sendall(length + message_bytes)
            
            # Receive response
            response_length = struct.unpack('!I', self.socket.recv(4))[0]
            response_data = self.socket.recv(response_length)
            return json.loads(response_data.decode('utf-8'))
            
        except Exception as e:
            return {'success': False, 'error': str(e)}
    
    def get_participants(self) -> List[Dict]:
        """Get list of DDS participants"""
        response = self.send_command('list_participants')
        return response.get('participants', [])
    
    def get_topics(self) -> List[Dict]:
        """Get list of DDS topics"""
        response = self.send_command('list_topics')
        return response.get('topics', [])
    
    def get_qos_stats(self) -> Dict:
        """Get QoS statistics"""
        response = self.send_command('get_qos_stats')
        return response.get('stats', {})
    
    def get_topology(self) -> Dict:
        """Get network topology"""
        response = self.send_command('get_topology')
        return response.get('topology', {'nodes': [], 'edges': []})
    
    def control_participant(self, participant_id: str, action: str) -> Dict:
        """Control a participant (start/stop/restart)"""
        return self.send_command('control_participant', {
            'participant_id': participant_id,
            'action': action
        })
    
    def start_monitoring(self, callback: Callable):
        """Start background monitoring"""
        self.running = True
        self.callbacks['metrics'] = callback
        self.monitor_thread = threading.Thread(target=self._monitor_loop)
        self.monitor_thread.daemon = True
        self.monitor_thread.start()
    
    def _monitor_loop(self):
        """Monitor loop for real-time updates"""
        while self.running:
            try:
                stats = self.get_qos_stats()
                if 'metrics' in self.callbacks:
                    self.callbacks['metrics'](stats)
            except Exception as e:
                print(f"Monitor error: {e}")
            
            import time
            time.sleep(1)


class DDSStateAdapter:
    """Adapter between DDS runtime and Web GUI state"""
    
    def __init__(self, runtime_client: DDSRuntimeClient):
        self.runtime = runtime_client
        self._cache = {
            'participants': [],
            'topics': [],
            'stats': {},
            'last_update': None
        }
        self._lock = threading.Lock()
    
    def refresh(self):
        """Refresh all cached data"""
        with self._lock:
            self._cache['participants'] = self.runtime.get_participants()
            self._cache['topics'] = self.runtime.get_topics()
            self._cache['stats'] = self.runtime.get_qos_stats()
            self._cache['last_update'] = datetime.now()
    
    def get_participants(self) -> List[Dict]:
        """Get cached participants"""
        with self._lock:
            return self._cache['participants']
    
    def get_topics(self) -> List[Dict]:
        """Get cached topics"""
        with self._lock:
            return self._cache['topics']
    
    def get_stats(self) -> Dict:
        """Get cached statistics"""
        with self._lock:
            return self._cache['stats']
    
    def get_summary(self) -> Dict:
        """Get system summary"""
        with self._lock:
            participants = self._cache['participants']
            topics = self._cache['topics']
            stats = self._cache['stats']
            
            return {
                'participant_count': len(participants),
                'topic_count': len(topics),
                'active_participants': len([p for p in participants if p.get('active', False)]),
                'avg_latency': stats.get('avg_latency', 0),
                'avg_throughput': stats.get('avg_throughput', 0),
                'packet_loss_rate': stats.get('packet_loss_rate', 0),
                'last_update': self._cache['last_update'].isoformat() if self._cache['last_update'] else None
            }


class MockDDSRuntime:
    """Mock DDS runtime for testing without actual DDS"""
    
    def __init__(self):
        self.participants = [
            {
                'id': 'participant_0',
                'name': 'Sensor-ECU-Front',
                'type': 'sensor',
                'status': 'active',
                'domain_id': 0,
                'ip_address': '192.168.1.101',
                'last_seen': datetime.now().isoformat()
            },
            {
                'id': 'participant_1',
                'name': 'Controller-ECU-Main',
                'type': 'controller',
                'status': 'active',
                'domain_id': 0,
                'ip_address': '192.168.1.102',
                'last_seen': datetime.now().isoformat()
            },
            {
                'id': 'participant_2',
                'name': 'Actuator-ECU-Steering',
                'type': 'actuator',
                'status': 'active',
                'domain_id': 0,
                'ip_address': '192.168.1.103',
                'last_seen': datetime.now().isoformat()
            },
            {
                'id': 'participant_3',
                'name': 'Gateway-ECU',
                'type': 'gateway',
                'status': 'active',
                'domain_id': 0,
                'ip_address': '192.168.1.100',
                'last_seen': datetime.now().isoformat()
            }
        ]
        
        self.topics = [
            {
                'id': 'topic_0',
                'name': '/vehicle/speed',
                'type': 'speed',
                'qos_profile': 'reliable',
                'publishers': ['participant_0'],
                'subscribers': ['participant_1', 'participant_2']
            },
            {
                'id': 'topic_1',
                'name': '/vehicle/steering_angle',
                'type': 'steering',
                'qos_profile': 'reliable',
                'publishers': ['participant_1'],
                'subscribers': ['participant_2']
            },
            {
                'id': 'topic_2',
                'name': '/vehicle/temperature',
                'type': 'temperature',
                'qos_profile': 'best_effort',
                'publishers': ['participant_0'],
                'subscribers': ['participant_1', 'participant_3']
            }
        ]
    
    def get_participants(self):
        return self.participants
    
    def get_topics(self):
        return self.topics
    
    def get_qos_stats(self):
        import random
        return {
            'latency': random.uniform(0.5, 3.0),
            'throughput': random.uniform(500, 800),
            'packet_loss_rate': random.uniform(0, 0.1),
            'message_count': random.randint(10000, 50000)
        }
    
    def get_topology(self):
        nodes = []
        edges = []
        
        for p in self.participants:
            nodes.append({
                'id': p['id'],
                'label': p['name'],
                'type': p['type']
            })
        
        for t in self.topics:
            for pub in t['publishers']:
                for sub in t['subscribers']:
                    edges.append({
                        'from': pub,
                        'to': sub,
                        'label': t['name'].split('/')[-1]
                    })
        
        return {'nodes': nodes, 'edges': edges}


def create_dds_client():
    """Factory function to create appropriate DDS client"""
    client = DDSRuntimeClient()
    if client.connect():
        return client
    
    # Fall back to mock client for development
    print("Warning: Using mock DDS runtime (actual runtime not available)")
    return MockDDSRuntime()
