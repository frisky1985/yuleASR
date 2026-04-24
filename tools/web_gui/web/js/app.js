/**
 * DDS Visualizer - Main Application JavaScript
 * Handles UI, WebSocket communication, charts, and real-time updates
 */

// Global state
const state = {
    socket: null,
    token: localStorage.getItem('dds_token'),
    currentPage: 'dashboard',
    charts: {},
    topologyNetwork: null,
    refreshInterval: null
};

// API base URL
const API_BASE = window.location.origin;

// ==================== Initialization ====================

document.addEventListener('DOMContentLoaded', function() {
    initializeApp();
});

function initializeApp() {
    // Initialize Socket.IO connection
    initSocketConnection();
    
    // Setup navigation
    setupNavigation();
    
    // Initialize charts
    initCharts();
    
    // Load initial data
    loadDashboardData();
    
    // Setup periodic refresh
    setupRefreshInterval();
}

// ==================== Socket.IO ====================

function initSocketConnection() {
    state.socket = io(API_BASE, {
        transports: ['websocket', 'polling'],
        reconnection: true,
        reconnectionDelay: 1000,
        reconnectionDelayMax: 5000
    });
    
    state.socket.on('connect', function() {
        console.log('Socket connected');
        updateConnectionStatus(true);
        
        // Subscribe to metrics
        state.socket.emit('subscribe_metrics');
        state.socket.emit('subscribe_alerts');
    });
    
    state.socket.on('disconnect', function() {
        console.log('Socket disconnected');
        updateConnectionStatus(false);
    });
    
    state.socket.on('metrics_update', function(data) {
        handleMetricsUpdate(data);
    });
    
    state.socket.on('new_alert', function(alert) {
        handleNewAlert(alert);
    });
    
    state.socket.on('topology_update', function(topology) {
        updateTopology(topology);
    });
}

function updateConnectionStatus(connected) {
    const dot = document.getElementById('connection-dot');
    const text = document.getElementById('connection-text');
    
    if (connected) {
        dot.classList.remove('connection-disconnected');
        dot.classList.add('connection-connected');
        text.textContent = 'Connected';
    } else {
        dot.classList.remove('connection-connected');
        dot.classList.add('connection-disconnected');
        text.textContent = 'Disconnected';
    }
}

// ==================== Navigation ====================

function setupNavigation() {
    document.querySelectorAll('.nav-link').forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            const page = this.dataset.page;
            navigateToPage(page);
            
            // Update active nav
            document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
            this.classList.add('active');
        });
    });
}

function navigateToPage(page) {
    state.currentPage = page;
    
    // Hide all pages
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    
    // Show target page
    document.getElementById(`page-${page}`).classList.add('active');
    
    // Update page title
    const titles = {
        'dashboard': 'Dashboard',
        'nodes': 'Node Management',
        'topics': 'Topic Management',
        'qos': 'QoS Monitor',
        'topology': 'Network Topology',
        'config': 'Configuration',
        'eol': 'End-of-Line Test',
        'ota': 'OTA Update'
    };
    document.getElementById('page-title').textContent = titles[page] || page;
    
    // Page-specific initialization
    switch(page) {
        case 'nodes':
            loadNodes();
            break;
        case 'topics':
            loadTopics();
            break;
        case 'qos':
            loadQoSData();
            break;
        case 'topology':
            initTopology();
            break;
        case 'config':
            loadConfig();
            break;
    }
}

// ==================== Charts ====================

function initCharts() {
    // Main chart (Latency & Throughput)
    const mainCtx = document.getElementById('mainChart').getContext('2d');
    state.charts.main = new Chart(mainCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Latency (ms)',
                    data: [],
                    borderColor: '#2563eb',
                    backgroundColor: 'rgba(37, 99, 235, 0.1)',
                    fill: true,
                    tension: 0.4,
                    yAxisID: 'y'
                },
                {
                    label: 'Throughput (KB/s)',
                    data: [],
                    borderColor: '#10b981',
                    backgroundColor: 'rgba(16, 185, 129, 0.1)',
                    fill: true,
                    tension: 0.4,
                    yAxisID: 'y1'
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            interaction: {
                intersect: false,
                mode: 'index'
            },
            scales: {
                y: {
                    type: 'linear',
                    display: true,
                    position: 'left',
                    title: {
                        display: true,
                        text: 'Latency (ms)'
                    }
                },
                y1: {
                    type: 'linear',
                    display: true,
                    position: 'right',
                    title: {
                        display: true,
                        text: 'Throughput (KB/s)'
                    },
                    grid: {
                        drawOnChartArea: false
                    }
                }
            }
        }
    });
    
    // Packet Loss Chart
    const packetCtx = document.getElementById('packetLossChart').getContext('2d');
    state.charts.packetLoss = new Chart(packetCtx, {
        type: 'doughnut',
        data: {
            labels: ['Success', 'Loss'],
            datasets: [{
                data: [99.9, 0.1],
                backgroundColor: ['#10b981', '#ef4444'],
                borderWidth: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            cutout: '70%',
            plugins: {
                legend: {
                    position: 'bottom'
                }
            }
        }
    });
}

function initQoSCharts() {
    // Latency Chart
    const latCtx = document.getElementById('latencyChart').getContext('2d');
    state.charts.latency = new Chart(latCtx, {
        type: 'bar',
        data: {
            labels: ['0-1ms', '1-2ms', '2-3ms', '3-4ms', '4-5ms', '>5ms'],
            datasets: [{
                label: 'Message Count',
                data: [45, 30, 15, 7, 2, 1],
                backgroundColor: '#2563eb'
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: true
                }
            }
        }
    });
    
    // Throughput Chart
    const tpCtx = document.getElementById('throughputChart').getContext('2d');
    state.charts.throughput = new Chart(tpCtx, {
        type: 'line',
        data: {
            labels: generateTimeLabels(20),
            datasets: [{
                label: 'Throughput (KB/s)',
                data: generateRandomData(20, 500, 800),
                borderColor: '#10b981',
                backgroundColor: 'rgba(16, 185, 129, 0.1)',
                fill: true,
                tension: 0.4
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false
        }
    });
    
    // Packet Loss History Chart
    const plhCtx = document.getElementById('packetLossHistoryChart').getContext('2d');
    state.charts.packetLossHistory = new Chart(plhCtx, {
        type: 'line',
        data: {
            labels: generateTimeLabels(20),
            datasets: [{
                label: 'Packet Loss (%)',
                data: generateRandomData(20, 0, 0.5),
                borderColor: '#ef4444',
                backgroundColor: 'rgba(239, 68, 68, 0.1)',
                fill: true,
                tension: 0.4
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: true,
                    max: 1
                }
            }
        }
    });
}

function generateTimeLabels(count) {
    const labels = [];
    const now = new Date();
    for (let i = count - 1; i >= 0; i--) {
        const t = new Date(now - i * 1000);
        labels.push(t.toLocaleTimeString());
    }
    return labels;
}

function generateRandomData(count, min, max) {
    return Array.from({length: count}, () => Math.random() * (max - min) + min);
}

// ==================== Data Loading ====================

async function loadDashboardData() {
    try {
        const response = await fetch(`${API_BASE}/api/dashboard/summary`, {
            headers: {
                'Authorization': `Bearer ${state.token}`
            }
        });
        
        if (!response.ok) throw new Error('Failed to load dashboard data');
        
        const data = await response.json();
        updateDashboardStats(data);
    } catch (error) {
        console.error('Error loading dashboard:', error);
    }
}

function updateDashboardStats(data) {
    document.getElementById('stat-participants').textContent = data.participants.active;
    document.getElementById('stat-topics').textContent = data.topics.active;
    document.getElementById('stat-latency').innerHTML = 
        `${data.performance.avg_latency.toFixed(1)}<span class="fs-6">ms</span>`;
    document.getElementById('stat-alerts').textContent = data.alerts.unacknowledged;
    
    // Update mode badge
    const modeBadge = document.getElementById('system-mode');
    const modes = {
        'normal': { text: 'Normal Mode', class: 'mode-normal' },
        'eol_test': { text: 'EOL Test Mode', class: 'mode-eol' },
        'ota_update': { text: 'OTA Update Mode', class: 'mode-ota' }
    };
    const mode = modes[data.system_status.mode] || modes.normal;
    modeBadge.textContent = mode.text;
    modeBadge.className = `mode-badge ${mode.class}`;
}

async function loadNodes() {
    try {
        const response = await fetch(`${API_BASE}/api/nodes`, {
            headers: {
                'Authorization': `Bearer ${state.token}`
            }
        });
        
        if (!response.ok) throw new Error('Failed to load nodes');
        
        const nodes = await response.json();
        renderNodesTable(nodes);
    } catch (error) {
        console.error('Error loading nodes:', error);
    }
}

function renderNodesTable(nodes) {
    const tbody = document.getElementById('nodes-table-body');
    tbody.innerHTML = nodes.map(node => `
        <tr>
            <td><code>${node.id}</code></td>
            <td><strong>${node.name}</strong></td>
            <td><span class="badge bg-secondary">${node.type}</span></td>
            <td>
                <span class="badge ${node.status === 'active' ? 'bg-success' : 'bg-danger'}">
                    ${node.status}
                </span>
            </td>
            <td><code>${node.ip_address}</code></td>
            <td>
                <small>Pub: ${node.topics_published.length}, Sub: ${node.topics_subscribed.length}</small>
            </td>
            <td>
                <button class="btn btn-sm btn-outline-primary btn-action" onclick="viewNode('${node.id}')">
                    <i class="bi bi-eye"></i>
                </button>
                <button class="btn btn-sm btn-outline-success btn-action" onclick="controlNode('${node.id}', 'restart')">
                    <i class="bi bi-arrow-clockwise"></i>
                </button>
            </td>
        </tr>
    `).join('');
}

async function loadTopics() {
    try {
        const response = await fetch(`${API_BASE}/api/topics`, {
            headers: {
                'Authorization': `Bearer ${state.token}`
            }
        });
        
        if (!response.ok) throw new Error('Failed to load topics');
        
        const topics = await response.json();
        renderTopicsTable(topics);
    } catch (error) {
        console.error('Error loading topics:', error);
    }
}

function renderTopicsTable(topics) {
    const tbody = document.getElementById('topics-table-body');
    tbody.innerHTML = topics.map(topic => `
        <tr>
            <td><code>${topic.name}</code></td>
            <td><span class="badge bg-info">${topic.type}</span></td>
            <td><span class="badge bg-primary">${topic.qos_profile}</span></td>
            <td>${topic.publishers.length}</td>
            <td>${topic.subscribers.length}</td>
            <td>${(topic.byte_rate / 1024).toFixed(1)} KB/s</td>
            <td>
                <button class="btn btn-sm btn-outline-primary btn-action" onclick="viewTopic('${topic.id}')">
                    <i class="bi bi-eye"></i>
                </button>
            </td>
        </tr>
    `).join('');
}

async function loadQoSData() {
    try {
        const response = await fetch(`${API_BASE}/api/qos/metrics`, {
            headers: {
                'Authorization': `Bearer ${state.token}`
            }
        });
        
        if (!response.ok) throw new Error('Failed to load QoS data');
        
        const data = await response.json();
        updateQoSCharts(data);
    } catch (error) {
        console.error('Error loading QoS data:', error);
    }
    
    // Initialize charts if not already done
    if (!state.charts.latency) {
        initQoSCharts();
    }
}

function updateQoSCharts(data) {
    // Update stats
    if (data.latency.length > 0) {
        document.getElementById('qos-min-latency').textContent = 
            `${Math.min(...data.latency).toFixed(1)} ms`;
        document.getElementById('qos-max-latency').textContent = 
            `${Math.max(...data.latency).toFixed(1)} ms`;
    }
    if (data.throughput.length > 0) {
        const avgTp = data.throughput.reduce((a, b) => a + b, 0) / data.throughput.length;
        document.getElementById('qos-avg-throughput').textContent = `${avgTp.toFixed(0)} KB/s`;
    }
}

async function loadConfig() {
    try {
        const response = await fetch(`${API_BASE}/api/config`, {
            headers: {
                'Authorization': `Bearer ${state.token}`
            }
        });
        
        if (!response.ok) throw new Error('Failed to load config');
        
        const data = await response.json();
        document.getElementById('config-editor').value = data.content;
    } catch (error) {
        console.error('Error loading config:', error);
        document.getElementById('config-editor').value = '# Failed to load configuration';
    }
}

// ==================== Topology ====================

function initTopology() {
    if (state.topologyNetwork) {
        state.topologyNetwork.destroy();
    }
    
    const container = document.getElementById('topology-container');
    
    fetch(`${API_BASE}/api/topology`, {
        headers: {
            'Authorization': `Bearer ${state.token}`
        }
    })
    .then(response => response.json())
    .then(topology => {
        const nodes = new vis.DataSet(
            topology.nodes.map(n => ({
                id: n.id,
                label: n.label,
                color: getNodeColor(n.type),
                shape: 'dot',
                size: 20
            }))
        );
        
        const edges = new vis.DataSet(
            topology.edges.map(e => ({
                from: e.from,
                to: e.to,
                label: e.label,
                arrows: 'to'
            }))
        );
        
        const options = {
            layout: {
                hierarchical: false
            },
            physics: {
                stabilization: false,
                barnesHut: {
                    gravitationalConstant: -2000,
                    springConstant: 0.04
                }
            },
            interaction: {
                hover: true
            }
        };
        
        state.topologyNetwork = new vis.Network(container, { nodes, edges }, options);
    })
    .catch(error => {
        console.error('Error loading topology:', error);
    });
}

function getNodeColor(type) {
    const colors = {
        'sensor': '#2563eb',
        'controller': '#10b981',
        'actuator': '#f59e0b',
        'gateway': '#06b6d4'
    };
    return colors[type] || '#64748b';
}

function updateTopology(topology) {
    if (!state.topologyNetwork) return;
    
    // Update with new data
    initTopology();
}

function resetTopologyView() {
    if (state.topologyNetwork) {
        state.topologyNetwork.fit();
    }
}

function refreshTopology() {
    initTopology();
}

// ==================== Real-time Updates ====================

function handleMetricsUpdate(data) {
    // Update charts
    if (state.charts.main && data.timestamps) {
        const labels = data.timestamps.map(t => new Date(t * 1000).toLocaleTimeString());
        
        state.charts.main.data.labels = labels;
        state.charts.main.data.datasets[0].data = data.latency;
        state.charts.main.data.datasets[1].data = data.throughput;
        state.charts.main.update('none');
    }
    
    // Update stats
    document.getElementById('stat-latency').innerHTML = 
        `${(data.latency[data.latency.length - 1] || 0).toFixed(1)}<span class="fs-6">ms</span>`;
}

function handleNewAlert(alert) {
    const container = document.getElementById('alerts-container');
    
    const alertClass = alert.level === 'warning' ? 'alert-warning' : 
                       alert.level === 'error' ? 'alert-danger' : 'alert-info';
    
    const alertHtml = `
        <div class="alert-item ${alertClass}">
            <i class="bi bi-${alert.level === 'error' ? 'x-circle' : alert.level === 'warning' ? 'exclamation-triangle' : 'info-circle'}"></i>
            <div>
                <div class="fw-semibold">${alert.level.toUpperCase()}</div>
                <div class="small">${alert.message}</div>
                <div class="text-muted small">${new Date(alert.timestamp * 1000).toLocaleString()}</div>
            </div>
        </div>
    `;
    
    container.insertAdjacentHTML('afterbegin', alertHtml);
    
    // Update alert count
    const currentCount = parseInt(document.getElementById('stat-alerts').textContent);
    document.getElementById('stat-alerts').textContent = currentCount + 1;
}

// ==================== Actions ====================

async function controlNode(nodeId, action) {
    try {
        const response = await fetch(`${API_BASE}/api/nodes/${nodeId}/control`, {
            method: 'POST',
            headers: {
                'Authorization': `Bearer ${state.token}`,
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ action })
        });
        
        const result = await response.json();
        showNotification(result.message, 'success');
        
        // Refresh nodes list
        loadNodes();
    } catch (error) {
        showNotification('Failed to control node: ' + error.message, 'error');
    }
}

function viewNode(nodeId) {
    showNotification(`Viewing node ${nodeId}`, 'info');
}

function viewTopic(topicId) {
    showNotification(`Viewing topic ${topicId}`, 'info');
}

async function validateConfig() {
    const content = document.getElementById('config-editor').value;
    
    try {
        const response = await fetch(`${API_BASE}/api/config/validate`, {
            method: 'POST',
            headers: {
                'Authorization': `Bearer ${state.token}`,
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ content })
        });
        
        const result = await response.json();
        if (result.valid) {
            showNotification('Configuration is valid', 'success');
        } else {
            showNotification('Configuration invalid: ' + result.errors.join(', '), 'error');
        }
    } catch (error) {
        showNotification('Validation failed: ' + error.message, 'error');
    }
}

async function saveConfig() {
    const content = document.getElementById('config-editor').value;
    
    try {
        const response = await fetch(`${API_BASE}/api/config`, {
            method: 'POST',
            headers: {
                'Authorization': `Bearer ${state.token}`,
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ content })
        });
        
        const result = await response.json();
        showNotification(result.message, 'success');
    } catch (error) {
        showNotification('Failed to save config: ' + error.message, 'error');
    }
}

async function deployConfig() {
    try {
        const response = await fetch(`${API_BASE}/api/config/deploy`, {
            method: 'POST',
            headers: {
                'Authorization': `Bearer ${state.token}`,
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ nodes: ['all'] })
        });
        
        const result = await response.json();
        showNotification('Configuration deployed successfully', 'success');
    } catch (error) {
        showNotification('Deployment failed: ' + error.message, 'error');
    }
}

function startEOLTest() {
    showNotification('Starting EOL test...', 'info');
}

function generateEOLReport() {
    showNotification('Generating EOL report...', 'info');
}

function startOTAUpdate() {
    const version = document.getElementById('ota-target-version').value;
    showNotification(`Starting OTA update to ${version}...`, 'info');
}

function rollbackOTA() {
    showNotification('Initiating OTA rollback...', 'warning');
}

function refreshNodes() {
    loadNodes();
    showNotification('Nodes refreshed', 'success');
}

function refreshTopics() {
    loadTopics();
    showNotification('Topics refreshed', 'success');
}

// ==================== Authentication ====================

function showLoginModal() {
    const modal = new bootstrap.Modal(document.getElementById('loginModal'));
    modal.show();
}

async function performLogin() {
    const username = document.getElementById('login-username').value;
    const password = document.getElementById('login-password').value;
    
    try {
        const response = await fetch(`${API_BASE}/api/auth/login`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ username, password })
        });
        
        const data = await response.json();
        
        if (response.ok) {
            state.token = data.token;
            localStorage.setItem('dds_token', data.token);
            
            bootstrap.Modal.getInstance(document.getElementById('loginModal')).hide();
            showNotification(`Welcome, ${data.user}!`, 'success');
            
            // Reload data with new token
            loadDashboardData();
        } else {
            document.getElementById('login-error').textContent = data.message;
            document.getElementById('login-error').classList.remove('d-none');
        }
    } catch (error) {
        document.getElementById('login-error').textContent = 'Login failed: ' + error.message;
        document.getElementById('login-error').classList.remove('d-none');
    }
}

// ==================== Utilities ====================

function setupRefreshInterval() {
    state.refreshInterval = setInterval(() => {
        if (state.currentPage === 'dashboard') {
            loadDashboardData();
        }
    }, 30000); // Refresh every 30 seconds
}

function showNotification(message, type = 'info') {
    // Create toast notification
    const toast = document.createElement('div');
    toast.className = `alert alert-${type === 'error' ? 'danger' : type} alert-dismissible fade show position-fixed`;
    toast.style.cssText = 'top: 20px; right: 20px; z-index: 9999; min-width: 300px;';
    toast.innerHTML = `
        ${message}
        <button type="button" class="btn-close" data-bs-dismiss="alert"></button>
    `;
    
    document.body.appendChild(toast);
    
    // Auto remove after 5 seconds
    setTimeout(() => {
        toast.remove();
    }, 5000);
}

// ==================== Export ====================

window.navigateToPage = navigateToPage;
window.controlNode = controlNode;
window.viewNode = viewNode;
window.viewTopic = viewTopic;
window.validateConfig = validateConfig;
window.saveConfig = saveConfig;
window.deployConfig = deployConfig;
window.startEOLTest = startEOLTest;
window.generateEOLReport = generateEOLReport;
window.startOTAUpdate = startOTAUpdate;
window.rollbackOTA = rollbackOTA;
window.refreshNodes = refreshNodes;
window.refreshTopics = refreshTopics;
window.resetTopologyView = resetTopologyView;
window.refreshTopology = refreshTopology;
window.showLoginModal = showLoginModal;
window.performLogin = performLogin;