#!/usr/bin/env python3
"""
DDS Web GUI Server - Flask Backend with WebSocket Support
Real-time DDS monitoring and management web interface
"""

import os
import sys
import json
import time
import hashlib
import secrets
import threading
from datetime import datetime, timedelta
from functools import wraps

from flask import Flask, request, jsonify, render_template, send_from_directory
from flask_cors import CORS
from flask_socketio import SocketIO, emit, join_room, leave_room
import jwt

# Add project path for DDS integration
sys.path.insert(0, '/home/admin/eth-dds-integration')
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Get correct paths
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEMPLATE_DIR = os.path.join(BASE_DIR, 'web', 'templates')
STATIC_DIR = os.path.join(BASE_DIR, 'web')

app = Flask(__name__, 
    template_folder=TEMPLATE_DIR,
    static_folder=STATIC_DIR)
app.config['SECRET_KEY'] = secrets.token_hex(32)
app.config['JWT_SECRET_KEY'] = secrets.token_hex(32)
app.config['JWT_ACCESS_TOKEN_EXPIRES'] = timedelta(hours=8)

CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')

# Import DDS client
try:
    from dds_client import DDSRuntimeClient, MockDDSRuntime, create_dds_client
    DDS_CLIENT_AVAILABLE = True
except ImportError:
    DDS_CLIENT_AVAILABLE = False
    print("Warning: DDS client not available, using mock data")

# Global DDS client instance
dds_client = None

def get_dds_client():
    """Get or create DDS client instance"""
    global dds_client
    if dds_client is None and DDS_CLIENT_AVAILABLE:
        try:
            dds_client = create_dds_client()
        except Exception as e:
            print(f"Failed to create DDS client: {e}")
            dds_client = MockDDSRuntime()
    elif dds_client is None:
        dds_client = MockDDSRuntime()
    return dds_client

# ==================== Authentication ====================

# Simple user store (in production, use database)
USERS = {
    'admin': {
        'password_hash': hashlib.sha256('admin123'.encode()).hexdigest(),
        'role': 'admin',
        'permissions': ['read', 'write', 'admin']
    },
    'operator': {
        'password_hash': hashlib.sha256('operator123'.encode()).hexdigest(),
        'role': 'operator',
        'permissions': ['read', 'write']
    },
    'viewer': {
        'password_hash': hashlib.sha256('viewer123'.encode()).hexdigest(),
        'role': 'viewer',
        'permissions': ['read']
    }
}

def token_required(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        token = None
        if 'Authorization' in request.headers:
            auth_header = request.headers['Authorization']
            try:
                token = auth_header.split(" ")[1]
            except IndexError:
                return jsonify({'message': 'Invalid token format'}), 401
        
        if not token:
            return jsonify({'message': 'Token is missing'}), 401
        
        try:
            data = jwt.decode(token, app.config['JWT_SECRET_KEY'], algorithms=["HS256"])
            current_user = data['user']
        except jwt.ExpiredSignatureError:
            return jsonify({'message': 'Token has expired'}), 401
        except jwt.InvalidTokenError:
            return jsonify({'message': 'Token is invalid'}), 401
        
        return f(current_user, *args, **kwargs)
    
    return decorated

def require_permission(permission):
    def decorator(f):
        @wraps(f)
        def decorated(current_user, *args, **kwargs):
            user = USERS.get(current_user)
            if not user or permission not in user.get('permissions', []):
                return jsonify({'message': 'Insufficient permissions'}), 403
            return f(current_user, *args, **kwargs)
        return decorated
    return decorator

# ==================== DDS State Management ====================

class DDSStateManager:
    def __init__(self):
        self.participants = {}
        self.topics = {}
        self.subscriptions = {}
        self.publications = {}
        self.qos_stats = {
            'latency': [],
            'throughput': [],
            'packet_loss': [],
            'timestamps': []
        }
        self.system_status = {
            'online': True,
            'mode': 'normal',  # normal, eol_test, ota_update
            'asil_level': 'D',
            'last_update': time.time()
        }
        self.alerts = []
        self.config_history = []
        self.topology = {'nodes': [], 'edges': []}
        self.running = True
        
    def start_monitoring(self):
        """Start background monitoring thread"""
        self.monitor_thread = threading.Thread(target=self._monitor_loop)
        self.monitor_thread.daemon = True
        self.monitor_thread.start()
        
    def _monitor_loop(self):
        """Background monitoring loop"""
        while self.running:
            try:
                self._collect_metrics()
                self._check_alerts()
                socketio.emit('metrics_update', self.get_current_metrics())
                time.sleep(1)
            except Exception as e:
                print(f"Monitor error: {e}")
                time.sleep(5)
    
    def _collect_metrics(self):
        """Collect DDS metrics (simulated for demo)"""
        import random
        now = time.time()
        
        # Simulate metrics
        latency = random.uniform(0.5, 5.0)  # ms
        throughput = random.uniform(100, 1000)  # KB/s
        packet_loss = random.uniform(0, 0.1)  # %
        
        self.qos_stats['latency'].append(latency)
        self.qos_stats['throughput'].append(throughput)
        self.qos_stats['packet_loss'].append(packet_loss)
        self.qos_stats['timestamps'].append(now)
        
        # Keep only last 100 points
        max_points = 100
        for key in self.qos_stats:
            if len(self.qos_stats[key]) > max_points:
                self.qos_stats[key] = self.qos_stats[key][-max_points:]
        
        self.system_status['last_update'] = now
        
    def _check_alerts(self):
        """Check for alert conditions"""
        if self.qos_stats['latency']:
            recent_latency = self.qos_stats['latency'][-1]
            if recent_latency > 4.0:
                self.add_alert('warning', f'High latency detected: {recent_latency:.2f}ms')
    
    def add_alert(self, level, message):
        """Add new alert"""
        alert = {
            'id': len(self.alerts),
            'level': level,
            'message': message,
            'timestamp': time.time(),
            'acknowledged': False
        }
        self.alerts.insert(0, alert)
        socketio.emit('new_alert', alert)
        
    def get_current_metrics(self):
        """Get current metrics for WebSocket broadcast"""
        return {
            'latency': self.qos_stats['latency'][-10:] if self.qos_stats['latency'] else [],
            'throughput': self.qos_stats['throughput'][-10:] if self.qos_stats['throughput'] else [],
            'packet_loss': self.qos_stats['packet_loss'][-10:] if self.qos_stats['packet_loss'] else [],
            'timestamps': self.qos_stats['timestamps'][-10:] if self.qos_stats['timestamps'] else [],
            'system_status': self.system_status,
            'participant_count': len(self.participants),
            'topic_count': len(self.topics)
        }
    
    def get_participants(self):
        """Get DDS participants from runtime"""
        try:
            client = get_dds_client()
            if hasattr(client, 'get_participants'):
                return client.get_participants()
        except Exception as e:
            print(f"Error getting participants: {e}")
        
        # Fallback to simulated data
        return [
            {
                'id': f'participant_{i}',
                'name': f'Node-{i}',
                'type': ['sensor', 'controller', 'actuator', 'gateway'][i % 4],
                'status': 'active',
                'domain_id': 0,
                'ip_address': f'192.168.1.{100+i}',
                'last_seen': time.time(),
                'topics_published': [f'topic_{j}' for j in range(i % 3)],
                'topics_subscribed': [f'topic_{j}' for j in range(3, 3 + i % 3)]
            }
            for i in range(8)
        ]
    
    def get_topics(self):
        """Get DDS topics from runtime"""
        try:
            client = get_dds_client()
            if hasattr(client, 'get_topics'):
                return client.get_topics()
        except Exception as e:
            print(f"Error getting topics: {e}")
        
        # Fallback to simulated data
        return [
            {
                'id': f'topic_{i}',
                'name': f'/vehicle/sensor_{i}',
                'type': ['temperature', 'speed', 'position', 'status'][i % 4],
                'qos_profile': 'reliable',
                'publishers': [f'participant_{i}'],
                'subscribers': [f'participant_{(i+1)%8}'],
                'message_count': 1000 + i * 100,
                'byte_rate': 1024 * (i + 1)
            }
            for i in range(6)
        ]
    
    def get_topology(self):
        """Get network topology from runtime"""
        try:
            client = get_dds_client()
            if hasattr(client, 'get_topology'):
                return client.get_topology()
        except Exception as e:
            print(f"Error getting topology: {e}")
        
        # Fallback to simulated data
        nodes = []
        edges = []
        
        # Add nodes
        for i in range(8):
            nodes.append({
                'id': f'node_{i}',
                'label': f'Node-{i}',
                'type': ['sensor', 'controller', 'actuator', 'gateway'][i % 4],
                'status': 'online'
            })
        
        # Add edges (connections)
        for i in range(6):
            edges.append({
                'from': f'node_{i}',
                'to': f'node_{(i+1)%8}',
                'label': f'topic_{i}',
                'type': 'pubsub'
            })
        
        return {'nodes': nodes, 'edges': edges}

# Global state manager
dds_state = DDSStateManager()

# ==================== REST API Routes ====================

@app.route('/')
def index():
    """Main dashboard page"""
    return render_template('index.html')

@app.route('/api/auth/login', methods=['POST'])
def login():
    """User login"""
    data = request.get_json()
    username = data.get('username')
    password = data.get('password')
    
    if not username or not password:
        return jsonify({'message': 'Username and password required'}), 400
    
    user = USERS.get(username)
    if not user:
        return jsonify({'message': 'User not found'}), 401
    
    password_hash = hashlib.sha256(password.encode()).hexdigest()
    if password_hash != user['password_hash']:
        return jsonify({'message': 'Invalid password'}), 401
    
    token = jwt.encode({
        'user': username,
        'role': user['role'],
        'exp': datetime.utcnow() + timedelta(hours=8)
    }, app.config['JWT_SECRET_KEY'], algorithm="HS256")
    
    return jsonify({
        'token': token,
        'user': username,
        'role': user['role'],
        'permissions': user['permissions']
    })

@app.route('/api/dashboard/summary')
@token_required
def get_dashboard_summary(current_user):
    """Get dashboard summary data"""
    return jsonify({
        'system_status': dds_state.system_status,
        'participants': {
            'total': len(dds_state.get_participants()),
            'active': len([p for p in dds_state.get_participants() if p['status'] == 'active'])
        },
        'topics': {
            'total': len(dds_state.get_topics()),
            'active': len(dds_state.get_topics())
        },
        'alerts': {
            'total': len(dds_state.alerts),
            'unacknowledged': len([a for a in dds_state.alerts if not a['acknowledged']])
        },
        'performance': {
            'avg_latency': sum(dds_state.qos_stats['latency'][-10:]) / 10 if dds_state.qos_stats['latency'] else 0,
            'avg_throughput': sum(dds_state.qos_stats['throughput'][-10:]) / 10 if dds_state.qos_stats['throughput'] else 0
        }
    })

@app.route('/api/nodes')
@token_required
def get_nodes(current_user):
    """Get all DDS nodes/participants"""
    return jsonify(dds_state.get_participants())

@app.route('/api/nodes/<node_id>')
@token_required
def get_node_detail(current_user, node_id):
    """Get node details"""
    participants = dds_state.get_participants()
    node = next((p for p in participants if p['id'] == node_id), None)
    if not node:
        return jsonify({'message': 'Node not found'}), 404
    return jsonify(node)

@app.route('/api/nodes/<node_id>/control', methods=['POST'])
@token_required
@require_permission('write')
def control_node(current_user, node_id):
    """Control node (start/stop/restart)"""
    data = request.get_json()
    action = data.get('action')
    
    if action not in ['start', 'stop', 'restart']:
        return jsonify({'message': 'Invalid action'}), 400
    
    # Simulate control action
    dds_state.add_alert('info', f'Node {node_id} {action} requested by {current_user}')
    
    return jsonify({
        'success': True,
        'message': f'Node {node_id} {action} initiated',
        'action': action,
        'timestamp': time.time()
    })

@app.route('/api/topics')
@token_required
def get_topics(current_user):
    """Get all DDS topics"""
    return jsonify(dds_state.get_topics())

@app.route('/api/topics/<topic_id>')
@token_required
def get_topic_detail(current_user, topic_id):
    """Get topic details"""
    topics = dds_state.get_topics()
    topic = next((t for t in topics if t['id'] == topic_id), None)
    if not topic:
        return jsonify({'message': 'Topic not found'}), 404
    return jsonify(topic)

@app.route('/api/qos/metrics')
@token_required
def get_qos_metrics(current_user):
    """Get QoS metrics history"""
    return jsonify({
        'latency': dds_state.qos_stats['latency'],
        'throughput': dds_state.qos_stats['throughput'],
        'packet_loss': dds_state.qos_stats['packet_loss'],
        'timestamps': dds_state.qos_stats['timestamps']
    })

@app.route('/api/qos/current')
@token_required
def get_qos_current(current_user):
    """Get current QoS stats"""
    return jsonify({
        'latency': dds_state.qos_stats['latency'][-1] if dds_state.qos_stats['latency'] else 0,
        'throughput': dds_state.qos_stats['throughput'][-1] if dds_state.qos_stats['throughput'] else 0,
        'packet_loss': dds_state.qos_stats['packet_loss'][-1] if dds_state.qos_stats['packet_loss'] else 0,
        'timestamp': time.time()
    })

@app.route('/api/topology')
@token_required
def get_topology(current_user):
    """Get network topology"""
    return jsonify(dds_state.get_topology())

@app.route('/api/alerts')
@token_required
def get_alerts(current_user):
    """Get all alerts"""
    return jsonify(dds_state.alerts)

@app.route('/api/alerts/<int:alert_id>/ack', methods=['POST'])
@token_required
@require_permission('write')
def acknowledge_alert(current_user, alert_id):
    """Acknowledge alert"""
    for alert in dds_state.alerts:
        if alert['id'] == alert_id:
            alert['acknowledged'] = True
            alert['acknowledged_by'] = current_user
            alert['acknowledged_at'] = time.time()
            return jsonify({'success': True})
    return jsonify({'message': 'Alert not found'}), 404

@app.route('/api/config')
@token_required
def get_config(current_user):
    """Get current configuration"""
    config_path = '/home/admin/eth-dds-integration/dds-config-tool/config.yaml'
    try:
        with open(config_path, 'r') as f:
            return jsonify({
                'content': f.read(),
                'format': 'yaml',
                'last_modified': os.path.getmtime(config_path)
            })
    except FileNotFoundError:
        return jsonify({'message': 'Config file not found'}), 404

@app.route('/api/config', methods=['POST'])
@token_required
@require_permission('write')
def update_config(current_user):
    """Update configuration"""
    data = request.get_json()
    content = data.get('content')
    
    if not content:
        return jsonify({'message': 'Content required'}), 400
    
    config_path = '/home/admin/eth-dds-integration/dds-config-tool/config.yaml'
    
    # Save old version
    backup_path = f"{config_path}.backup.{int(time.time())}"
    try:
        with open(config_path, 'r') as f:
            old_content = f.read()
        with open(backup_path, 'w') as f:
            f.write(old_content)
    except:
        pass
    
    # Write new config
    with open(config_path, 'w') as f:
        f.write(content)
    
    dds_state.config_history.append({
        'timestamp': time.time(),
        'user': current_user,
        'backup_path': backup_path
    })
    
    dds_state.add_alert('info', f'Configuration updated by {current_user}')
    
    return jsonify({'success': True, 'message': 'Configuration updated'})

@app.route('/api/config/validate', methods=['POST'])
@token_required
def validate_config(current_user):
    """Validate configuration"""
    data = request.get_json()
    content = data.get('content')
    
    # Simple YAML validation
    try:
        import yaml
        yaml.safe_load(content)
        return jsonify({'valid': True, 'errors': []})
    except Exception as e:
        return jsonify({'valid': False, 'errors': [str(e)]})

@app.route('/api/config/deploy', methods=['POST'])
@token_required
@require_permission('admin')
def deploy_config(current_user):
    """Deploy configuration to nodes"""
    data = request.get_json()
    nodes = data.get('nodes', [])
    
    # Simulate deployment
    results = []
    for node in nodes:
        results.append({
            'node': node,
            'status': 'success',
            'message': 'Configuration deployed'
        })
    
    dds_state.add_alert('info', f'Configuration deployed to {len(nodes)} nodes by {current_user}')
    
    return jsonify({
        'success': True,
        'deployed_at': time.time(),
        'results': results
    })

@app.route('/api/system/mode', methods=['POST'])
@token_required
@require_permission('admin')
def set_system_mode(current_user):
    """Set system mode (normal, eol_test, ota_update)"""
    data = request.get_json()
    mode = data.get('mode')
    
    if mode not in ['normal', 'eol_test', 'ota_update']:
        return jsonify({'message': 'Invalid mode'}), 400
    
    old_mode = dds_state.system_status['mode']
    dds_state.system_status['mode'] = mode
    
    dds_state.add_alert('warning', f'System mode changed from {old_mode} to {mode} by {current_user}')
    
    return jsonify({
        'success': True,
        'old_mode': old_mode,
        'new_mode': mode
    })

@app.route('/api/ota/rollback', methods=['POST'])
@token_required
@require_permission('admin')
def ota_rollback(current_user):
    """Initiate OTA rollback"""
    data = request.get_json()
    version = data.get('version')
    
    dds_state.system_status['mode'] = 'ota_update'
    
    dds_state.add_alert('warning', f'OTA rollback to version {version} initiated by {current_user}')
    
    return jsonify({
        'success': True,
        'version': version,
        'started_at': time.time(),
        'status': 'in_progress'
    })

@app.route('/api/eol/start', methods=['POST'])
@token_required
@require_permission('admin')
def start_eol_test(current_user):
    """Start End-of-Line test mode"""
    dds_state.system_status['mode'] = 'eol_test'
    
    dds_state.add_alert('info', f'EOL test mode started by {current_user}')
    
    return jsonify({
        'success': True,
        'mode': 'eol_test',
        'started_at': time.time()
    })

# ==================== WebSocket Events ====================

@socketio.on('connect')
def handle_connect():
    """Client connected"""
    print('Client connected')
    emit('connected', {'status': 'connected', 'timestamp': time.time()})

@socketio.on('disconnect')
def handle_disconnect():
    """Client disconnected"""
    print('Client disconnected')

@socketio.on('subscribe_metrics')
def handle_subscribe_metrics():
    """Subscribe to metrics updates"""
    join_room('metrics')
    emit('subscribed', {'channel': 'metrics'})

@socketio.on('unsubscribe_metrics')
def handle_unsubscribe_metrics():
    """Unsubscribe from metrics updates"""
    leave_room('metrics')
    emit('unsubscribed', {'channel': 'metrics'})

@socketio.on('subscribe_alerts')
def handle_subscribe_alerts():
    """Subscribe to alerts"""
    join_room('alerts')
    emit('subscribed', {'channel': 'alerts'})

@socketio.on('request_topology')
def handle_request_topology():
    """Request topology data"""
    emit('topology_update', dds_state.get_topology())

# ==================== Static Files ====================

@app.route('/static/<path:path>')
def send_static(path):
    """Serve static files"""
    return send_from_directory('../web', path)

# ==================== Main ====================

if __name__ == '__main__':
    print("Starting DDS Web GUI Server...")
    print("Dashboard: http://localhost:5000")
    print("API Docs: http://localhost:5000/api/")
    
    # Start monitoring
    dds_state.start_monitoring()
    
    # Run server
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)