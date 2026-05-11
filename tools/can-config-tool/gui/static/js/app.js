/**
 * CAN配置工具 - 前端JavaScript
 */

// 全局状态
let currentConfig = {
    ecu_name: 'ECU0',
    ipdus: [],
    signals: [],
    generatedFiles: null
};

let currentEditIndex = null;
let currentEditType = null;

// DOM元素
const uploadArea = document.getElementById('uploadArea');
const fileInput = document.getElementById('fileInput');
const uploadStatus = document.getElementById('uploadStatus');
const ecuNameInput = document.getElementById('ecuName');
const sourceFileInput = document.getElementById('sourceFile');
const sourceTypeInput = document.getElementById('sourceType');
const btnGenerate = document.getElementById('btnGenerate');
const btnReset = document.getElementById('btnReset');
const editModal = document.getElementById('editModal');

// 初始化
document.addEventListener('DOMContentLoaded', () => {
    initEventListeners();
    initTabs();
});

function initEventListeners() {
    // 文件上传
    uploadArea.addEventListener('click', () => fileInput.click());
    
    uploadArea.addEventListener('dragover', (e) => {
        e.preventDefault();
        uploadArea.classList.add('dragover');
    });
    
    uploadArea.addEventListener('dragleave', () => {
        uploadArea.classList.remove('dragover');
    });
    
    uploadArea.addEventListener('drop', (e) => {
        e.preventDefault();
        uploadArea.classList.remove('dragover');
        const files = e.dataTransfer.files;
        if (files.length > 0) {
            handleFileUpload(files[0]);
        }
    });
    
    fileInput.addEventListener('change', (e) => {
        if (e.target.files.length > 0) {
            handleFileUpload(e.target.files[0]);
        }
    });
    
    // ECU名称更新
    ecuNameInput.addEventListener('change', updateEcuName);
    
    // 按钮事件
    btnGenerate.addEventListener('click', generateConfig);
    btnReset.addEventListener('click', resetConfig);
    
    // 模态框事件
    document.getElementById('modalClose').addEventListener('click', closeModal);
    document.getElementById('modalCancel').addEventListener('click', closeModal);
    document.getElementById('modalSave').addEventListener('click', saveEdit);
    
    // 预览按钮
    document.getElementById('btnPreviewH').addEventListener('click', () => switchPreview('h'));
    document.getElementById('btnPreviewC').addEventListener('click', () => switchPreview('c'));
    document.getElementById('btnDownloadH').addEventListener('click', () => downloadFile('Com_Cfg.h'));
    document.getElementById('btnDownloadC').addEventListener('click', () => downloadFile('Com_Cfg.c'));
}

function initTabs() {
    const tabs = document.querySelectorAll('.tab');
    const contents = document.querySelectorAll('.tab-content');
    
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const targetId = 'tab-' + tab.dataset.tab;
            
            tabs.forEach(t => t.classList.remove('active'));
            contents.forEach(c => c.classList.remove('active'));
            
            tab.classList.add('active');
            document.getElementById(targetId).classList.add('active');
        });
    });
}

async function handleFileUpload(file) {
    showUploadStatus('上传中...', 'loading');
    
    const formData = new FormData();
    formData.append('file', file);
    
    try {
        const response = await fetch('/api/upload', {
            method: 'POST',
            body: formData
        });
        
        const data = await response.json();
        
        if (data.success) {
            showUploadStatus(`✓ ${data.message}`, 'success');
            sourceFileInput.value = data.data.source_file;
            sourceTypeInput.value = data.data.source_type;
            
            // 加载配置
            await loadConfig();
        } else {
            showUploadStatus(`❌ ${data.error || '上传失败'}`, 'error');
        }
    } catch (error) {
        showUploadStatus(`❌ 上传失败: ${error.message}`, 'error');
    }
}

function showUploadStatus(message, type) {
    const icon = type === 'loading' ? '<span class="loading"></span>' : 
                 type === 'success' ? '✓' : '❌';
    const color = type === 'error' ? '#e74c3c' : type === 'success' ? '#27ae60' : '#667eea';
    uploadStatus.innerHTML = `<span style="color: ${color}">${icon} ${message}</span>`;
}

async function loadConfig() {
    try {
        const response = await fetch('/api/config');
        const data = await response.json();
        
        currentConfig = { ...currentConfig, ...data };
        ecuNameInput.value = data.ecu_name;
        
        updateUI();
        btnGenerate.disabled = false;
    } catch (error) {
        console.error('加载配置失败:', error);
    }
}

async function updateEcuName() {
    const newName = ecuNameInput.value;
    
    try {
        await fetch('/api/config/ecu', {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ ecu_name: newName })
        });
        
        currentConfig.ecu_name = newName;
    } catch (error) {
        console.error('更新ECU名称失败:', error);
    }
}

function updateUI() {
    // 更新统计数字
    document.getElementById('statIpdu').textContent = currentConfig.ipdus.length;
    document.getElementById('statSignals').textContent = currentConfig.signals.length;
    
    const rxCount = currentConfig.ipdus.filter(i => i.direction === 'RECEIVE').length;
    const txCount = currentConfig.ipdus.filter(i => i.direction === 'SEND').length;
    document.getElementById('statRx').textContent = rxCount;
    document.getElementById('statTx').textContent = txCount;
    
    // 更新IPDU列表
    updateIpduTable();
    
    // 更新信号列表
    updateSignalsTable();
}

function updateIpduTable() {
    const emptyDiv = document.getElementById('ipduEmpty');
    const tableDiv = document.getElementById('ipduTable');
    const tbody = document.getElementById('ipduTableBody');
    
    if (currentConfig.ipdus.length === 0) {
        emptyDiv.style.display = 'block';
        tableDiv.style.display = 'none';
        return;
    }
    
    emptyDiv.style.display = 'none';
    tableDiv.style.display = 'block';
    
    tbody.innerHTML = currentConfig.ipdus.map((ipdu, index) => `
        <tr>
            <td><strong>${ipdu.name}</strong></td>
            <td><code>0x${ipdu.message_id.toString(16).toUpperCase().padStart(4, '0')}</code></td>
            <td>${ipdu.dlc}</td>
            <td><span class="badge badge-${ipdu.direction === 'SEND' ? 'primary' : 'secondary'}">${ipdu.direction === 'SEND' ? '发送' : '接收'}</span></td>
            <td>${ipdu.signals ? ipdu.signals.length : 0}</td>
            <td>
                <button class="btn btn-small" onclick="editIpdu(${index})">编辑</button>
            </td>
        </tr>
    `).join('');
}

function updateSignalsTable() {
    const emptyDiv = document.getElementById('signalsEmpty');
    const tableDiv = document.getElementById('signalsTable');
    const tbody = document.getElementById('signalsTableBody');
    
    if (currentConfig.signals.length === 0) {
        emptyDiv.style.display = 'block';
        tableDiv.style.display = 'none';
        return;
    }
    
    emptyDiv.style.display = 'none';
    tableDiv.style.display = 'block';
    
    tbody.innerHTML = currentConfig.signals.map((sig, index) => `
        <tr>
            <td><strong>${sig.name}</strong></td>
            <td><code>${sig.data_type}</code></td>
            <td>${sig.start_bit}</td>
            <td>${sig.bit_length}</td>
            <td>${sig.factor}</td>
            <td>${sig.offset}</td>
            <td>${sig.init_value}</td>
            <td>
                <button class="btn btn-small" onclick="editSignal(${index})">编辑</button>
            </td>
        </tr>
    `).join('');
}

function editIpdu(index) {
    currentEditIndex = index;
    currentEditType = 'ipdu';
    const ipdu = currentConfig.ipdus[index];
    
    document.getElementById('modalTitle').textContent = '编辑 IPDU';
    document.getElementById('modalBody').innerHTML = `
        <div class="form-group">
            <label class="form-label">名称</label>
            <input type="text" class="form-input" id="editName" value="${ipdu.name}" readonly>
        </div>
        <div class="form-group">
            <label class="form-label">方向</label>
            <select class="form-input" id="editDirection">
                <option value="SEND" ${ipdu.direction === 'SEND' ? 'selected' : ''}>发送 (SEND)</option>
                <option value="RECEIVE" ${ipdu.direction === 'RECEIVE' ? 'selected' : ''}>接收 (RECEIVE)</option>
            </select>
        </div>
        <div class="form-group">
            <label class="form-label">周期时间 (ms)</label>
            <input type="number" class="form-input" id="editCycleTime" value="${ipdu.cycle_time}">
        </div>
    `;
    
    editModal.classList.add('active');
}

function editSignal(index) {
    currentEditIndex = index;
    currentEditType = 'signal';
    const signal = currentConfig.signals[index];
    
    document.getElementById('modalTitle').textContent = '编辑 信号';
    document.getElementById('modalBody').innerHTML = `
        <div class="form-group">
            <label class="form-label">名称</label>
            <input type="text" class="form-input" id="editName" value="${signal.name}" readonly>
        </div>
        <div class="form-group">
            <label class="form-label">起始位</label>
            <input type="number" class="form-input" id="editStartBit" value="${signal.start_bit}">
        </div>
        <div class="form-group">
            <label class="form-label">长度</label>
            <input type="number" class="form-input" id="editBitLength" value="${signal.bit_length}">
        </div>
        <div class="form-group">
            <label class="form-label">因子 (Factor)</label>
            <input type="number" step="0.001" class="form-input" id="editFactor" value="${signal.factor}">
        </div>
        <div class="form-group">
            <label class="form-label">偏移 (Offset)</label>
            <input type="number" step="0.001" class="form-input" id="editOffset" value="${signal.offset}">
        </div>
        <div class="form-group">
            <label class="form-label">初始值</label>
            <input type="number" class="form-input" id="editInitValue" value="${signal.init_value}">
        </div>
    `;
    
    editModal.classList.add('active');
}

function closeModal() {
    editModal.classList.remove('active');
    currentEditIndex = null;
    currentEditType = null;
}

async function saveEdit() {
    if (currentEditIndex === null || !currentEditType) return;
    
    const data = {};
    
    if (currentEditType === 'ipdu') {
        data.direction = document.getElementById('editDirection').value;
        data.cycle_time = parseInt(document.getElementById('editCycleTime').value);
        
        await fetch(`/api/config/ipdu/${currentEditIndex}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
        
        currentConfig.ipdus[currentEditIndex].direction = data.direction;
        currentConfig.ipdus[currentEditIndex].cycle_time = data.cycle_time;
    } else if (currentEditType === 'signal') {
        data.start_bit = parseInt(document.getElementById('editStartBit').value);
        data.bit_length = parseInt(document.getElementById('editBitLength').value);
        data.factor = parseFloat(document.getElementById('editFactor').value);
        data.offset = parseFloat(document.getElementById('editOffset').value);
        data.init_value = parseInt(document.getElementById('editInitValue').value);
        
        await fetch(`/api/config/signal/${currentEditIndex}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });
        
        Object.assign(currentConfig.signals[currentEditIndex], data);
    }
    
    closeModal();
    updateUI();
}

async function generateConfig() {
    btnGenerate.disabled = true;
    btnGenerate.innerHTML = '<span class="loading"></span> 生成中...';
    
    try {
        const response = await fetch('/api/generate', { method: 'POST' });
        const data = await response.json();
        
        if (data.success) {
            currentConfig.generatedFiles = data.files;
            
            // 显示预览
            document.getElementById('previewEmpty').style.display = 'none';
            document.getElementById('previewContent').style.display = 'block';
            
            // 默认显示.h文件
            switchPreview('h');
            
            // 切换到预览选项卡
            document.querySelector('[data-tab="preview"]').click();
            
            showUploadStatus('✓ 配置文件生成成功', 'success');
        } else {
            showUploadStatus(`❌ ${data.error || '生成失败'}`, 'error');
        }
    } catch (error) {
        showUploadStatus(`❌ 生成失败: ${error.message}`, 'error');
    } finally {
        btnGenerate.disabled = false;
        btnGenerate.innerHTML = '<span>🚀</span> 生成配置文件';
    }
}

function switchPreview(type) {
    const btnH = document.getElementById('btnPreviewH');
    const btnC = document.getElementById('btnPreviewC');
    const previewArea = document.getElementById('previewCode');
    
    if (!currentConfig.generatedFiles) return;
    
    if (type === 'h') {
        btnH.classList.remove('btn-secondary');
        btnC.classList.add('btn-secondary');
        previewArea.innerHTML = syntaxHighlight(currentConfig.generatedFiles['Com_Cfg.h']);
    } else {
        btnH.classList.add('btn-secondary');
        btnC.classList.remove('btn-secondary');
        previewArea.innerHTML = syntaxHighlight(currentConfig.generatedFiles['Com_Cfg.c']);
    }
}

function syntaxHighlight(code) {
    return code
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/(\/\/.*$|\/\*[\s\S]*?\*\/)/gm, '<span class="comment">$1</span>')
        .replace(/(#include|#define|#ifndef|#endif)/g, '<span class="keyword">$1</span>')
        .replace(/\b(int|void|char|float|double|static|const|struct|typedef|extern)\b/g, '<span class="keyword">$1</span>')
        .replace(/\b(0x[0-9A-Fa-f]+|\d+)\b/g, '<span class="number">$1</span>')
        .replace(/(".*?")/g, '<span class="string">$1</span>');
}

async function downloadFile(filename) {
    if (!currentConfig.generatedFiles || !currentConfig.generatedFiles[filename]) {
        alert('请先生成配置文件');
        return;
    }
    
    const content = currentConfig.generatedFiles[filename];
    const blob = new Blob([content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

function resetConfig() {
    currentConfig = {
        ecu_name: 'ECU0',
        ipdus: [],
        signals: [],
        generatedFiles: null
    };
    
    ecuNameInput.value = 'ECU0';
    sourceFileInput.value = '';
    sourceTypeInput.value = '';
    uploadStatus.innerHTML = '';
    
    document.getElementById('statIpdu').textContent = '0';
    document.getElementById('statSignals').textContent = '0';
    document.getElementById('statRx').textContent = '0';
    document.getElementById('statTx').textContent = '0';
    
    document.getElementById('ipduEmpty').style.display = 'block';
    document.getElementById('ipduTable').style.display = 'none';
    document.getElementById('signalsEmpty').style.display = 'block';
    document.getElementById('signalsTable').style.display = 'none';
    document.getElementById('previewEmpty').style.display = 'block';
    document.getElementById('previewContent').style.display = 'none';
    
    btnGenerate.disabled = true;
}
