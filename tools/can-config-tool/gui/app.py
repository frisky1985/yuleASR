#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CAN配置工具 - Web GUI

Flask后端API服务
"""

import sys
import json
import tempfile
import os
from pathlib import Path
from datetime import datetime
from flask import Flask, render_template, request, jsonify, send_file
from flask_cors import CORS
from werkzeug.utils import secure_filename

sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from dbc_parser import DbcParser
from can_matrix_parser import CanMatrixParser
from com_config_generator import ComConfigGenerator

app = Flask(__name__)
CORS(app)

# 配置
UPLOAD_FOLDER = tempfile.gettempdir()
ALLOWED_EXTENSIONS = {'dbc', 'csv', 'xlsx', 'xls'}
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB限制

# 全局状态
current_config = {
    'ecu_name': 'ECU0',
    'ipdus': [],
    'signals': [],
    'signal_groups': [],
    'source_file': None,
    'source_type': None
}


def allowed_file(filename):
    """检查文件是否允许"""
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS


@app.route('/')
def index():
    """主页面"""
    return render_template('index.html')


@app.route('/api/upload', methods=['POST'])
def upload_file():
    """上传并解析文件"""
    global current_config
    
    if 'file' not in request.files:
        return jsonify({'error': '没有文件'}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': '文件名为空'}), 400
    
    if not allowed_file(file.filename):
        return jsonify({'error': '不支持的文件格式，请上传DBC/CSV/Excel文件'}), 400
    
    try:
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        
        # 解析文件
        ext = filename.rsplit('.', 1)[1].lower()
        
        if ext == 'dbc':
            parser = DbcParser()
            network = parser.parse_file(filepath)
            config = parser.to_com_config()
            source_type = 'DBC'
        elif ext == 'csv':
            parser = CanMatrixParser()
            matrix = parser.parse_csv(filepath)
            config = parser.to_com_config()
            source_type = 'CSV'
        elif ext in ('xlsx', 'xls'):
            parser = CanMatrixParser()
            matrix = parser.parse_excel(filepath)
            config = parser.to_com_config()
            source_type = 'Excel'
        else:
            return jsonify({'error': '不支持的文件格式'}), 400
        
        # 更新全局配置
        current_config.update(config)
        current_config['source_file'] = filename
        current_config['source_type'] = source_type
        
        # 清理临时文件
        os.remove(filepath)
        
        return jsonify({
            'success': True,
            'message': f'成功解析{source_type}文件',
            'data': {
                'source_type': source_type,
                'source_file': filename,
                'ecu_name': current_config['ecu_name'],
                'ipdu_count': len(current_config['ipdus']),
                'signal_count': len(current_config['signals'])
            }
        })
        
    except Exception as e:
        return jsonify({'error': f'解析失败: {str(e)}'}), 500


@app.route('/api/config', methods=['GET'])
def get_config():
    """获取当前配置"""
    return jsonify({
        'ecu_name': current_config['ecu_name'],
        'ipdus': current_config['ipdus'],
        'signals': current_config['signals'],
        'signal_groups': current_config['signal_groups']
    })


@app.route('/api/config/ecu', methods=['PUT'])
def update_ecu():
    """更新ECU名称"""
    global current_config
    
    data = request.get_json()
    if 'ecu_name' in data:
        current_config['ecu_name'] = data['ecu_name']
    
    return jsonify({'success': True, 'ecu_name': current_config['ecu_name']})


@app.route('/api/config/ipdu/<int:index>', methods=['PUT'])
def update_ipdu(index):
    """更新IPDU配置"""
    global current_config
    
    if index < 0 or index >= len(current_config['ipdus']):
        return jsonify({'error': 'IPDU索引超出范围'}), 400
    
    data = request.get_json()
    ipdu = current_config['ipdus'][index]
    
    # 更新字段
    if 'direction' in data:
        ipdu['direction'] = data['direction']
    if 'cycle_time' in data:
        ipdu['cycle_time'] = int(data['cycle_time'])
    
    return jsonify({'success': True, 'ipdu': ipdu})


@app.route('/api/config/signal/<int:index>', methods=['PUT'])
def update_signal(index):
    """更新信号配置"""
    global current_config
    
    if index < 0 or index >= len(current_config['signals']):
        return jsonify({'error': '信号索引超出范围'}), 400
    
    data = request.get_json()
    signal = current_config['signals'][index]
    
    # 更新字段
    if 'start_bit' in data:
        signal['start_bit'] = int(data['start_bit'])
    if 'bit_length' in data:
        signal['bit_length'] = int(data['bit_length'])
    if 'factor' in data:
        signal['factor'] = float(data['factor'])
    if 'offset' in data:
        signal['offset'] = float(data['offset'])
    if 'init_value' in data:
        signal['init_value'] = int(data['init_value'])
    
    return jsonify({'success': True, 'signal': signal})


@app.route('/api/generate', methods=['POST'])
def generate_config():
    """生成Com配置文件"""
    global current_config
    
    try:
        # 创建生成器
        generator = ComConfigGenerator(current_config)
        
        # 生成配置文件
        output_dir = tempfile.mkdtemp()
        cfg_h_path, cfg_c_path = generator.generate(output_dir)
        
        # 读取文件内容
        with open(cfg_h_path, 'r') as f:
            cfg_h_content = f.read()
        with open(cfg_c_path, 'r') as f:
            cfg_c_content = f.read()
        
        # 清理临时文件
        os.remove(cfg_h_path)
        os.remove(cfg_c_path)
        os.rmdir(output_dir)
        
        return jsonify({
            'success': True,
            'files': {
                'Com_Cfg.h': cfg_h_content,
                'Com_Cfg.c': cfg_c_content
            }
        })
        
    except Exception as e:
        return jsonify({'error': f'生成失败: {str(e)}'}), 500


@app.route('/api/download/<filename>', methods=['POST'])
def download_file(filename):
    """下载配置文件"""
    if filename not in ['Com_Cfg.h', 'Com_Cfg.c']:
        return jsonify({'error': '无效的文件名'}), 400
    
    data = request.get_json()
    if 'content' not in data:
        return jsonify({'error': '缺少文件内容'}), 400
    
    content = data['content']
    
    # 创建临时文件
    temp_path = os.path.join(tempfile.gettempdir(), filename)
    with open(temp_path, 'w') as f:
        f.write(content)
    
    return send_file(temp_path, as_attachment=True, download_name=filename)


@app.route('/api/summary', methods=['GET'])
def get_summary():
    """获取配置摘要"""
    global current_config
    
    ipdus = current_config['ipdus']
    signals = current_config['signals']
    
    summary = {
        'ecu_name': current_config['ecu_name'],
        'source_type': current_config.get('source_type', 'None'),
        'source_file': current_config.get('source_file', 'None'),
        'ipdu_count': len(ipdus),
        'signal_count': len(signals),
        'ipdus': [
            {
                'name': ipdu['name'],
                'message_id': f"0x{ipdu['message_id']:04X}",
                'dlc': ipdu['dlc'],
                'direction': ipdu['direction'],
                'signal_count': len(ipdu.get('signals', []))
            }
            for ipdu in ipdus
        ],
        'signals': [
            {
                'name': sig['name'],
                'data_type': sig['data_type'],
                'start_bit': sig['start_bit'],
                'bit_length': sig['bit_length']
            }
            for sig in signals[:20]  # 只显示前20个
        ]
    }
    
    if len(signals) > 20:
        summary['signal_overflow'] = len(signals) - 20
    
    return jsonify(summary)


if __name__ == '__main__':
    print("=" * 60)
    print("🚀 CAN配置工具 - Web GUI")
    print("=" * 60)
    print("访问 http://localhost:5000 使用工具")
    print("按 Ctrl+C 停止服务")
    print("=" * 60)
    app.run(host='0.0.0.0', port=5000, debug=True)
