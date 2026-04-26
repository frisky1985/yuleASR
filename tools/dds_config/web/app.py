#!/usr/bin/env python3
"""
DDS Configuration Web GUI - Flask Backend

Provides REST API for DDS configuration management:
    - /api/validate  - Validate configuration
    - /api/generate  - Generate code
    - /api/convert   - Convert between formats
    - /api/config    - Get/set configuration
    - /api/domains   - Manage domains
    - /api/topics    - Manage topics
    - /api/qos       - Manage QoS profiles

Usage:
    python app.py              # Run development server
    flask run --host=0.0.0.0   # Production deployment
"""

import os
import sys
import json
import logging
import tempfile
import uuid
from pathlib import Path
from datetime import datetime
from typing import Dict, Any, Optional, List

from flask import Flask, request, jsonify, render_template, send_file
from flask_cors import CORS
from werkzeug.utils import secure_filename

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))
from dds_config_cli import (
    DDSConfigValidator, DDSCodeGenerator, ARXMLConverter,
    DDSConfigError, load_config, save_config, HAS_YAML
)

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('dds-web')

# Flask application
app = Flask(__name__, 
    template_folder='templates',
    static_folder='static'
)
app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY', 'dev-secret-key')
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB max file size
app.config['UPLOAD_FOLDER'] = tempfile.gettempdir()

# Enable CORS
CORS(app, resources={
    r"/api/*": {
        "origins": "*",
        "methods": ["GET", "POST", "PUT", "DELETE", "OPTIONS"],
        "allow_headers": ["Content-Type", "Authorization"]
    }
})

# In-memory session storage for configurations
session_configs: Dict[str, Dict[str, Any]] = {}

# Default configuration template
DEFAULT_CONFIG = {
    "system": {
        "name": "DDS_System",
        "version": "1.0.0",
        "description": "DDS Configuration"
    },
    "domains": [
        {
            "id": 0,
            "name": "DefaultDomain",
            "participants": []
        }
    ],
    "transport": {
        "kind": "UDP",
        "interface": "eth0",
        "multicast_address": "239.255.0.1",
        "port_base": 7400
    },
    "qos_profiles": {
        "default": {
            "reliability": "BEST_EFFORT",
            "history": {"kind": "KEEP_LAST", "depth": 1}
        },
        "reliable": {
            "reliability": "RELIABLE",
            "durability": "TRANSIENT_LOCAL",
            "history": {"kind": "KEEP_LAST", "depth": 10}
        },
        "realtime": {
            "reliability": "RELIABLE",
            "deadline": {"sec": 0, "nanosec": 100000000},
            "latency_budget": {"sec": 0, "nanosec": 1000000}
        }
    }
}

# =============================================================================
# Routes - Web Interface
# =============================================================================

@app.route('/')
def index():
    """Main page"""
    return render_template('index.html')

@app.route('/editor')
def editor():
    """Configuration editor page"""
    return render_template('index.html', page='editor')

@app.route('/domains')
def domains_page():
    """Domain management page"""
    return render_template('index.html', page='domains')

@app.route('/topics')
def topics_page():
    """Topic management page"""
    return render_template('index.html', page='topics')

@app.route('/qos')
def qos_page():
    """QoS management page"""
    return render_template('index.html', page='qos')

# =============================================================================
# API - Configuration Management
# =============================================================================

@app.route('/api/config', methods=['GET', 'POST', 'PUT'])
def api_config():
    """
    GET: Get current configuration
    POST: Create new configuration
    PUT: Update configuration
    """
    session_id = request.headers.get('X-Session-ID', 'default')
    
    if request.method == 'GET':
        config = session_configs.get(session_id, DEFAULT_CONFIG.copy())
        return jsonify({
            'success': True,
            'config': config,
            'session_id': session_id
        })
    
    elif request.method == 'POST':
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        session_configs[session_id] = data
        return jsonify({
            'success': True,
            'message': 'Configuration created',
            'session_id': session_id
        })
    
    elif request.method == 'PUT':
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        # Merge with existing config
        existing = session_configs.get(session_id, {})
        existing.update(data)
        session_configs[session_id] = existing
        
        return jsonify({
            'success': True,
            'message': 'Configuration updated',
            'session_id': session_id
        })

@app.route('/api/config/upload', methods=['POST'])
def api_config_upload():
    """Upload configuration file"""
    if 'file' not in request.files:
        return jsonify({'success': False, 'error': 'No file provided'}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({'success': False, 'error': 'No file selected'}), 400
    
    try:
        content = file.read().decode('utf-8')
        
        # Parse based on extension
        if file.filename.endswith(('.yaml', '.yml')):
            if not HAS_YAML:
                return jsonify({'success': False, 'error': 'YAML support not available'}), 400
            import yaml
            config = yaml.safe_load(content)
        else:
            config = json.loads(content)
        
        session_id = str(uuid.uuid4())
        session_configs[session_id] = config
        
        return jsonify({
            'success': True,
            'config': config,
            'session_id': session_id,
            'filename': file.filename
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 400

@app.route('/api/config/download', methods=['GET'])
def api_config_download():
    """Download configuration as file"""
    session_id = request.args.get('session_id', 'default')
    format = request.args.get('format', 'yaml')
    
    config = session_configs.get(session_id, DEFAULT_CONFIG)
    
    if format == 'yaml':
        if not HAS_YAML:
            return jsonify({'success': False, 'error': 'YAML support not available'}), 400
        import yaml
        content = yaml.dump(config, default_flow_style=False, sort_keys=False)
        mime_type = 'application/x-yaml'
        ext = 'yaml'
    else:
        content = json.dumps(config, indent=2)
        mime_type = 'application/json'
        ext = 'json'
    
    # Create temp file
    temp_path = Path(tempfile.gettempdir()) / f"dds_config.{ext}"
    temp_path.write_text(content, encoding='utf-8')
    
    return send_file(
        temp_path,
        mimetype=mime_type,
        as_attachment=True,
        download_name=f"dds_config.{ext}"
    )

# =============================================================================
# API - Validation
# =============================================================================

@app.route('/api/validate', methods=['POST'])
def api_validate():
    """
    Validate DDS configuration
    
    Request body: JSON configuration or {config: {...}}
    Response: Validation results with errors and warnings
    """
    try:
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        # Extract config from request
        config = data.get('config', data)
        
        # Validate
        validator = DDSConfigValidator()
        is_valid, errors, warnings = validator.validate(config)
        
        return jsonify({
            'success': True,
            'valid': is_valid,
            'errors': errors,
            'warnings': warnings,
            'summary': {
                'total_errors': len(errors),
                'total_warnings': len(warnings)
            }
        })
    
    except Exception as e:
        logger.exception("Validation error")
        return jsonify({'success': False, 'error': str(e)}), 500

# =============================================================================
# API - Code Generation
# =============================================================================

@app.route('/api/generate', methods=['POST'])
def api_generate():
    """
    Generate C code from configuration
    
    Request body: JSON configuration or {config: {...}}
    Response: Generated files content
    """
    try:
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        config = data.get('config', data)
        
        # Validate first
        validator = DDSConfigValidator()
        is_valid, errors, warnings = validator.validate(config)
        
        if not is_valid:
            return jsonify({
                'success': False,
                'error': 'Configuration validation failed',
                'errors': errors
            }), 400
        
        # Generate code
        generator = DDSCodeGenerator(config)
        files = generator.generate()
        
        return jsonify({
            'success': True,
            'files': files,
            'warnings': warnings,
            'generated_at': datetime.now().isoformat()
        })
    
    except Exception as e:
        logger.exception("Generation error")
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/generate/download', methods=['POST'])
def api_generate_download():
    """Generate and download as zip file"""
    try:
        data = request.get_json()
        config = data.get('config', data)
        
        generator = DDSCodeGenerator(config)
        files = generator.generate()
        
        # Create zip file
        import zipfile
        zip_path = Path(tempfile.gettempdir()) / 'dds_generated.zip'
        
        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
            for filename, content in files.items():
                zf.writestr(filename, content)
        
        return send_file(
            zip_path,
            mimetype='application/zip',
            as_attachment=True,
            download_name='dds_generated.zip'
        )
    
    except Exception as e:
        logger.exception("Download error")
        return jsonify({'success': False, 'error': str(e)}), 500

# =============================================================================
# API - Format Conversion
# =============================================================================

@app.route('/api/convert', methods=['POST'])
def api_convert():
    """
    Convert between YAML and ARXML formats
    
    Request body: {
        "config": {...} or "content": "...",
        "from": "yaml" | "arxml",
        "to": "yaml" | "arxml"
    }
    """
    try:
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        from_format = data.get('from', 'yaml')
        to_format = data.get('to', 'arxml')
        
        converter = ARXMLConverter()
        
        if from_format == 'yaml' and to_format == 'arxml':
            config = data.get('config', {})
            result = converter.yaml_to_arxml(config)
            return jsonify({
                'success': True,
                'content': result,
                'format': 'arxml'
            })
        
        elif from_format == 'arxml' and to_format == 'yaml':
            content = data.get('content', '')
            result = converter.arxml_to_yaml(content)
            return jsonify({
                'success': True,
                'config': result,
                'format': 'yaml'
            })
        
        else:
            return jsonify({
                'success': False,
                'error': f'Unsupported conversion: {from_format} to {to_format}'
            }), 400
    
    except Exception as e:
        logger.exception("Conversion error")
        return jsonify({'success': False, 'error': str(e)}), 500

# =============================================================================
# API - Domain Management
# =============================================================================

@app.route('/api/domains', methods=['GET', 'POST'])
def api_domains():
    """List or create domains"""
    session_id = request.headers.get('X-Session-ID', 'default')
    config = session_configs.get(session_id, DEFAULT_CONFIG)
    
    if request.method == 'GET':
        return jsonify({
            'success': True,
            'domains': config.get('domains', [])
        })
    
    elif request.method == 'POST':
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        if 'domains' not in config:
            config['domains'] = []
        
        # Check for duplicate ID
        new_id = data.get('id')
        for domain in config['domains']:
            if domain.get('id') == new_id:
                return jsonify({
                    'success': False,
                    'error': f'Domain with ID {new_id} already exists'
                }), 400
        
        config['domains'].append(data)
        session_configs[session_id] = config
        
        return jsonify({
            'success': True,
            'domain': data,
            'message': 'Domain created'
        })

@app.route('/api/domains/<int:domain_id>', methods=['GET', 'PUT', 'DELETE'])
def api_domain(domain_id: int):
    """Get, update, or delete specific domain"""
    session_id = request.headers.get('X-Session-ID', 'default')
    config = session_configs.get(session_id, DEFAULT_CONFIG)
    
    # Find domain
    domains = config.get('domains', [])
    domain = None
    domain_index = -1
    for i, d in enumerate(domains):
        if d.get('id') == domain_id:
            domain = d
            domain_index = i
            break
    
    if domain is None:
        return jsonify({'success': False, 'error': 'Domain not found'}), 404
    
    if request.method == 'GET':
        return jsonify({'success': True, 'domain': domain})
    
    elif request.method == 'PUT':
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        # Update domain
        domains[domain_index].update(data)
        session_configs[session_id] = config
        
        return jsonify({
            'success': True,
            'domain': domains[domain_index],
            'message': 'Domain updated'
        })
    
    elif request.method == 'DELETE':
        domains.pop(domain_index)
        session_configs[session_id] = config
        
        return jsonify({
            'success': True,
            'message': f'Domain {domain_id} deleted'
        })

# =============================================================================
# API - Topic Management
# =============================================================================

@app.route('/api/topics', methods=['GET', 'POST'])
def api_topics():
    """List or create topics"""
    session_id = request.headers.get('X-Session-ID', 'default')
    config = session_configs.get(session_id, DEFAULT_CONFIG)
    
    # Collect all topics from all participants
    topics = []
    for domain in config.get('domains', []):
        for participant in domain.get('participants', []):
            participant_name = participant.get('name', 'Unknown')
            domain_id = domain.get('id', 0)
            
            for pub in participant.get('publishers', []):
                topics.append({
                    'name': pub.get('topic'),
                    'type': pub.get('type', 'unknown'),
                    'participant': participant_name,
                    'domain_id': domain_id,
                    'role': 'publisher',
                    'qos': pub.get('qos', {})
                })
            
            for sub in participant.get('subscribers', []):
                topics.append({
                    'name': sub.get('topic'),
                    'type': sub.get('type', 'unknown'),
                    'participant': participant_name,
                    'domain_id': domain_id,
                    'role': 'subscriber',
                    'qos': sub.get('qos', {})
                })
    
    if request.method == 'GET':
        # Filter by domain if specified
        domain_filter = request.args.get('domain', type=int)
        if domain_filter is not None:
            topics = [t for t in topics if t['domain_id'] == domain_filter]
        
        return jsonify({
            'success': True,
            'topics': topics,
            'total': len(topics)
        })
    
    elif request.method == 'POST':
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        # Add topic to specified participant
        domain_id = data.get('domain_id')
        participant_name = data.get('participant')
        topic_name = data.get('name')
        topic_type = data.get('type', 'void*')
        role = data.get('role', 'publisher')
        qos = data.get('qos', {})
        
        # Find domain and participant
        for domain in config.get('domains', []):
            if domain.get('id') == domain_id:
                for participant in domain.get('participants', []):
                    if participant.get('name') == participant_name:
                        # Add topic
                        endpoint = {
                            'topic': topic_name,
                            'type': topic_type,
                            'qos': qos
                        }
                        
                        if role == 'publisher':
                            if 'publishers' not in participant:
                                participant['publishers'] = []
                            participant['publishers'].append(endpoint)
                        else:
                            if 'subscribers' not in participant:
                                participant['subscribers'] = []
                            participant['subscribers'].append(endpoint)
                        
                        session_configs[session_id] = config
                        
                        return jsonify({
                            'success': True,
                            'topic': endpoint,
                            'message': f'Topic {topic_name} added as {role}'
                        })
        
        return jsonify({
            'success': False,
            'error': 'Domain or participant not found'
        }), 404

# =============================================================================
# API - QoS Management
# =============================================================================

@app.route('/api/qos', methods=['GET', 'POST'])
def api_qos():
    """List or create QoS profiles"""
    session_id = request.headers.get('X-Session-ID', 'default')
    config = session_configs.get(session_id, DEFAULT_CONFIG)
    
    if request.method == 'GET':
        profiles = config.get('qos_profiles', {})
        return jsonify({
            'success': True,
            'profiles': profiles
        })
    
    elif request.method == 'POST':
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        profile_name = data.get('name')
        if not profile_name:
            return jsonify({'success': False, 'error': 'Profile name required'}), 400
        
        if 'qos_profiles' not in config:
            config['qos_profiles'] = {}
        
        config['qos_profiles'][profile_name] = data.get('settings', {})
        session_configs[session_id] = config
        
        return jsonify({
            'success': True,
            'profile': {profile_name: config['qos_profiles'][profile_name]},
            'message': f'QoS profile {profile_name} created'
        })

@app.route('/api/qos/<profile_name>', methods=['GET', 'PUT', 'DELETE'])
def api_qos_profile(profile_name: str):
    """Get, update, or delete QoS profile"""
    session_id = request.headers.get('X-Session-ID', 'default')
    config = session_configs.get(session_id, DEFAULT_CONFIG)
    
    profiles = config.get('qos_profiles', {})
    
    if profile_name not in profiles:
        return jsonify({'success': False, 'error': 'Profile not found'}), 404
    
    if request.method == 'GET':
        return jsonify({
            'success': True,
            'profile': {profile_name: profiles[profile_name]}
        })
    
    elif request.method == 'PUT':
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'error': 'No data provided'}), 400
        
        profiles[profile_name].update(data.get('settings', {}))
        session_configs[session_id] = config
        
        return jsonify({
            'success': True,
            'profile': {profile_name: profiles[profile_name]},
            'message': f'QoS profile {profile_name} updated'
        })
    
    elif request.method == 'DELETE':
        del profiles[profile_name]
        session_configs[session_id] = config
        
        return jsonify({
            'success': True,
            'message': f'QoS profile {profile_name} deleted'
        })

# =============================================================================
# API - Template Management
# =============================================================================

@app.route('/api/templates', methods=['GET'])
def api_templates():
    """Get available configuration templates"""
    templates = {
        'default': DEFAULT_CONFIG,
        'minimal': {
            "system": {"name": "Minimal_DDS", "version": "1.0.0"},
            "domains": [{"id": 0, "name": "Domain0", "participants": []}]
        },
        'automotive': {
            "system": {"name": "Automotive_DDS", "version": "1.0.0"},
            "domains": [
                {
                    "id": 1,
                    "name": "PerceptionDomain",
                    "participants": [
                        {
                            "name": "CameraECU",
                            "qos_profile": "sensor_high_reliability",
                            "publishers": [
                                {
                                    "topic": "CameraObjectDetection",
                                    "type": "ObjectDetectionData",
                                    "qos": {
                                        "reliability": "RELIABLE",
                                        "deadline": {"sec": 0, "nanosec": 33000000}
                                    }
                                }
                            ]
                        }
                    ]
                }
            ],
            "transport": {"kind": "UDP", "interface": "eth0"},
            "tsn": {"enabled": True}
        }
    }
    
    return jsonify({
        'success': True,
        'templates': templates
    })

# =============================================================================
# Error Handlers
# =============================================================================

@app.errorhandler(404)
def not_found(error):
    return jsonify({'success': False, 'error': 'Not found'}), 404

@app.errorhandler(500)
def internal_error(error):
    logger.exception("Internal server error")
    return jsonify({'success': False, 'error': 'Internal server error'}), 500

# =============================================================================
# Main Entry Point
# =============================================================================

if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='DDS Configuration Web Server')
    parser.add_argument('--host', default='0.0.0.0', help='Host to bind to')
    parser.add_argument('--port', type=int, default=5000, help='Port to listen on')
    parser.add_argument('--debug', action='store_true', help='Enable debug mode')
    
    args = parser.parse_args()
    
    if args.debug:
        app.debug = True
        logger.setLevel(logging.DEBUG)
    
    logger.info(f"Starting DDS Configuration Web Server on {args.host}:{args.port}")
    app.run(host=args.host, port=args.port, debug=args.debug)
