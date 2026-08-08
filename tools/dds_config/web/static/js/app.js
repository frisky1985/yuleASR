/**
 * DDS Configuration Tool - Frontend Application
 * 
 * Features:
 * - YAML editor with live validation
 * - Domain/Participant/Topic management
 * - QoS profile configuration
 * - Code generation preview
 * - Import/Export functionality
 */

class DDSConfigApp {
    constructor() {
        this.config = null;
        this.sessionId = this.generateSessionId();
        this.generatedFiles = {};
        this.currentPage = 'editor';
        this.modals = {};
        this.apiBaseUrl = '';
        
        this.init();
    }

    init() {
        this.initModals();
        this.initEventListeners();
        this.loadDefaultConfig();
        this.updateUI();
    }

    initModals() {
        this.modals.addDomain = new bootstrap.Modal(document.getElementById('addDomainModal'));
        this.modals.addTopic = new bootstrap.Modal(document.getElementById('addTopicModal'));
        this.modals.addQoS = new bootstrap.Modal(document.getElementById('addQoSModal'));
    }

    initEventListeners() {
        // Navigation
        document.querySelectorAll('[data-page]').forEach(link => {
            link.addEventListener('click', (e) => {
                e.preventDefault();
                const page = e.currentTarget.dataset.page;
                this.navigateTo(page);
            });
        });

        // Toolbar buttons
        document.getElementById('btn-new').addEventListener('click', () => this.newConfig());
        document.getElementById('btn-load').addEventListener('click', () => document.getElementById('file-input').click());
        document.getElementById('btn-save').addEventListener('click', () => this.downloadConfig());

        // File input
        document.getElementById('file-input').addEventListener('change', (e) => this.handleFileUpload(e));

        // Preview file tabs
        document.querySelectorAll('#preview-file-list button').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const filename = e.currentTarget.dataset.file;
                this.showPreviewFile(filename);
            });
        });

        // Auto-save on editor change (debounced)
        let timeout;
        document.getElementById('yaml-editor').addEventListener('input', () => {
            clearTimeout(timeout);
            timeout = setTimeout(() => this.parseEditorContent(), 1000);
        });
    }

    generateSessionId() {
        return 'session_' + Math.random().toString(36).substr(2, 9);
    }

    // Navigation
    navigateTo(page) {
        this.currentPage = page;
        
        // Update nav links
        document.querySelectorAll('[data-page]').forEach(link => {
            link.classList.toggle('active', link.dataset.page === page);
        });

        // Show/hide pages
        document.querySelectorAll('.page-content').forEach(content => {
            content.classList.toggle('d-none', !content.id.endsWith(page));
        });

        // Page-specific initialization
        if (page === 'domains') {
            this.renderDomains();
        } else if (page === 'topics') {
            this.renderTopics();
        } else if (page === 'qos') {
            this.renderQoS();
        } else if (page === 'preview') {
            this.generateCode();
        }
    }

    // Configuration Management
    loadDefaultConfig() {
        this.config = {
            system: {
                name: 'DDS_System',
                version: '1.0.0',
                description: 'DDS Configuration'
            },
            domains: [
                {
                    id: 0,
                    name: 'DefaultDomain',
                    participants: []
                }
            ],
            transport: {
                kind: 'UDP',
                interface: 'eth0',
                multicast_address: '239.255.0.1',
                port_base: 7400
            },
            qos_profiles: {
                default: {
                    reliability: 'BEST_EFFORT',
                    history: { kind: 'KEEP_LAST', depth: 1 }
                },
                reliable: {
                    reliability: 'RELIABLE',
                    durability: 'TRANSIENT_LOCAL',
                    history: { kind: 'KEEP_LAST', depth: 10 }
                }
            }
        };
        this.syncEditorWithConfig();
        this.updateUI();
    }

    newConfig() {
        if (confirm('Create new configuration? Unsaved changes will be lost.')) {
            this.loadDefaultConfig();
            this.showToast('New configuration created', 'success');
        }
    }

    loadTemplate(templateName) {
        const templates = {
            default: this.config,
            minimal: {
                system: { name: 'Minimal_DDS', version: '1.0.0' },
                domains: [{ id: 0, name: 'Domain0', participants: [] }]
            },
            automotive: {
                system: { name: 'Automotive_DDS', version: '1.0.0' },
                domains: [
                    {
                        id: 1,
                        name: 'PerceptionDomain',
                        participants: [
                            {
                                name: 'CameraECU',
                                qos_profile: 'sensor_high_reliability',
                                publishers: [
                                    {
                                        topic: 'CameraObjectDetection',
                                        type: 'ObjectDetectionData',
                                        qos: {
                                            reliability: 'RELIABLE',
                                            deadline: { sec: 0, nanosec: 33000000 }
                                        }
                                    }
                                ]
                            },
                            {
                                name: 'RadarECU',
                                publishers: [
                                    {
                                        topic: 'RadarDetection',
                                        type: 'RadarData',
                                        qos: { reliability: 'RELIABLE' }
                                    }
                                ]
                            }
                        ]
                    },
                    {
                        id: 2,
                        name: 'ControlDomain',
                        participants: [
                            {
                                name: 'PlanningECU',
                                qos_profile: 'control_realtime',
                                subscribers: [
                                    {
                                        topic: 'CameraObjectDetection',
                                        type: 'ObjectDetectionData'
                                    },
                                    {
                                        topic: 'RadarDetection',
                                        type: 'RadarData'
                                    }
                                ]
                            }
                        ]
                    }
                ],
                transport: { kind: 'UDP', interface: 'eth0' },
                tsn: { enabled: true }
            }
        };

        if (templates[templateName]) {
            this.config = JSON.parse(JSON.stringify(templates[templateName]));
            this.syncEditorWithConfig();
            this.updateUI();
            this.showToast(`Loaded ${templateName} template`, 'success');
        }
    }

    // File Operations
    async handleFileUpload(event) {
        const file = event.target.files[0];
        if (!file) return;

        const formData = new FormData();
        formData.append('file', file);

        try {
            const response = await fetch(`${this.apiBaseUrl}/api/config/upload`, {
                method: 'POST',
                body: formData
            });

            const result = await response.json();

            if (result.success) {
                this.config = result.config;
                this.sessionId = result.session_id;
                this.syncEditorWithConfig();
                this.updateUI();
                this.showToast(`Loaded ${file.name}`, 'success');
            } else {
                this.showToast(result.error || 'Failed to load file', 'danger');
            }
        } catch (error) {
            this.showToast('Error loading file: ' + error.message, 'danger');
        }

        // Reset file input
        event.target.value = '';
    }

    async downloadConfig() {
        const format = 'yaml';
        window.open(`${this.apiBaseUrl}/api/config/download?session_id=${this.sessionId}&format=${format}`, '_blank');
    }

    // Editor Management
    syncEditorWithConfig() {
        const yaml = this.toYAML(this.config);
        document.getElementById('yaml-editor').value = yaml;
    }

    parseEditorContent() {
        const content = document.getElementById('yaml-editor').value;
        try {
            this.config = this.fromYAML(content);
            this.updateUI();
            return true;
        } catch (e) {
            console.error('Parse error:', e);
            return false;
        }
    }

    formatYAML() {
        if (this.parseEditorContent()) {
            this.syncEditorWithConfig();
            this.showToast('YAML formatted', 'success');
        } else {
            this.showToast('Invalid YAML syntax', 'danger');
        }
    }

    // Validation
    async validateConfig() {
        if (!this.parseEditorContent()) {
            this.showValidationResult(false, ['Invalid YAML syntax'], []);
            return;
        }

        try {
            const response = await fetch(`${this.apiBaseUrl}/api/validate`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-Session-ID': this.sessionId
                },
                body: JSON.stringify({ config: this.config })
            });

            const result = await response.json();

            if (result.success) {
                this.showValidationResult(result.valid, result.errors, result.warnings);
                this.updateStatusIndicator(result.valid);
            } else {
                this.showToast(result.error || 'Validation failed', 'danger');
            }
        } catch (error) {
            // Fallback to client-side validation
            this.clientSideValidation();
        }
    }

    clientSideValidation() {
        const errors = [];
        const warnings = [];

        // Basic validation
        if (!this.config.system) {
            errors.push('Missing system configuration');
        } else {
            if (!this.config.system.name) errors.push('Missing system name');
            if (!this.config.system.version) warnings.push('Missing system version');
        }

        if (!this.config.domains || !Array.isArray(this.config.domains) || this.config.domains.length === 0) {
            errors.push('At least one domain is required');
        } else {
            const domainIds = new Set();
            this.config.domains.forEach((domain, idx) => {
                if (domain.id === undefined) errors.push(`Domain[${idx}]: Missing ID`);
                else if (domainIds.has(domain.id)) errors.push(`Domain[${idx}]: Duplicate ID ${domain.id}`);
                else domainIds.add(domain.id);

                if (!domain.name) errors.push(`Domain[${idx}]: Missing name`);
            });
        }

        this.showValidationResult(errors.length === 0, errors, warnings);
        this.updateStatusIndicator(errors.length === 0);
    }

    showValidationResult(valid, errors, warnings) {
        const container = document.getElementById('validation-results');
        let html = '';

        if (valid && errors.length === 0) {
            html = '<div class="alert alert-success"><i class="bi bi-check-circle"></i> Configuration is valid</div>';
        } else {
            if (errors.length > 0) {
                html += '<div class="alert alert-danger"><h6><i class="bi bi-x-circle"></i> Errors</h6><ul class="mb-0">';
                errors.forEach(err => html += `<li>${this.escapeHtml(err)}</li>`);
                html += '</ul></div>';
            }
            if (warnings.length > 0) {
                html += '<div class="alert alert-warning"><h6><i class="bi bi-exclamation-triangle"></i> Warnings</h6><ul class="mb-0">';
                warnings.forEach(warn => html += `<li>${this.escapeHtml(warn)}</li>`);
                html += '</ul></div>';
            }
        }

        container.innerHTML = html;
    }

    updateStatusIndicator(valid) {
        const indicator = document.getElementById('status-indicator');
        if (valid) {
            indicator.innerHTML = '<span class="badge bg-success me-2">●</span><small>Valid Configuration</small>';
        } else {
            indicator.innerHTML = '<span class="badge bg-danger me-2">●</span><small>Invalid Configuration</small>';
        }
    }

    // Domain Management
    renderDomains() {
        const container = document.getElementById('domains-list');
        const domains = this.config.domains || [];

        if (domains.length === 0) {
            container.innerHTML = '<div class="col"><div class="alert alert-info">No domains configured. Click "Add Domain" to create one.</div></div>';
            return;
        }

        container.innerHTML = domains.map(domain => {
            const participantCount = domain.participants?.length || 0;
            const topicCount = domain.participants?.reduce((acc, p) => {
                return acc + (p.publishers?.length || 0) + (p.subscribers?.length || 0);
            }, 0) || 0;

            return `
                <div class="col-md-6 col-lg-4 mb-4">
                    <div class="card h-100">
                        <div class="card-header d-flex justify-content-between align-items-center">
                            <span><i class="bi bi-globe"></i> ${this.escapeHtml(domain.name)}</span>
                            <span class="badge bg-primary">ID: ${domain.id}</span>
                        </div>
                        <div class="card-body">
                            <p class="text-muted mb-2">
                                <i class="bi bi-people"></i> ${participantCount} Participant${participantCount !== 1 ? 's' : ''}<br>
                                <i class="bi bi-chat-dots"></i> ${topicCount} Topic${topicCount !== 1 ? 's' : ''}
                            </p>
                            ${domain.participants?.length ? `
                                <h6 class="mt-3">Participants:</h6>
                                <ul class="list-unstyled small">
                                    ${domain.participants.map(p => `
                                        <li><i class="bi bi-person"></i> ${this.escapeHtml(p.name)}</li>
                                    `).join('')}
                                </ul>
                            ` : '<p class="text-muted small">No participants</p>'}
                        </div>
                        <div class="card-footer">
                            <button class="btn btn-sm btn-outline-primary" onclick="app.editDomain(${domain.id})">
                                <i class="bi bi-pencil"></i> Edit
                            </button>
                            <button class="btn btn-sm btn-outline-danger float-end" onclick="app.deleteDomain(${domain.id})">
                                <i class="bi bi-trash"></i>
                            </button>
                        </div>
                    </div>
                </div>
            `;
        }).join('');
    }

    showAddDomainModal() {
        document.getElementById('addDomainForm').reset();
        this.modals.addDomain.show();
    }

    addDomain() {
        const id = parseInt(document.getElementById('domain-id').value);
        const name = document.getElementById('domain-name').value.trim();

        if (isNaN(id) || id < 0 || id > 232) {
            this.showToast('Domain ID must be between 0 and 232', 'danger');
            return;
        }

        if (!name) {
            this.showToast('Domain name is required', 'danger');
            return;
        }

        // Check for duplicate ID
        if (this.config.domains.some(d => d.id === id)) {
            this.showToast(`Domain with ID ${id} already exists`, 'danger');
            return;
        }

        this.config.domains.push({ id, name, participants: [] });
        this.syncEditorWithConfig();
        this.modals.addDomain.hide();
        this.renderDomains();
        this.updateUI();
        this.showToast(`Domain "${name}" added`, 'success');
    }

    deleteDomain(id) {
        if (!confirm(`Delete domain ${id}? This action cannot be undone.`)) return;

        this.config.domains = this.config.domains.filter(d => d.id !== id);
        this.syncEditorWithConfig();
        this.renderDomains();
        this.updateUI();
        this.showToast('Domain deleted', 'success');
    }

    // Topic Management
    renderTopics() {
        const tbody = document.getElementById('topics-table');
        const topics = this.collectAllTopics();

        if (topics.length === 0) {
            tbody.innerHTML = '<tr><td colspan="7" class="text-center text-muted">No topics configured</td></tr>';
            return;
        }

        tbody.innerHTML = topics.map(topic => `
            <tr>
                <td><code>${this.escapeHtml(topic.name)}</code></td>
                <td><span class="badge bg-secondary">${this.escapeHtml(topic.type)}</span></td>
                <td>${topic.domain_id}</td>
                <td>${this.escapeHtml(topic.participant)}</td>
                <td>
                    <span class="badge ${topic.role === 'publisher' ? 'bg-info' : 'bg-warning'}">
                        ${topic.role}
                    </span>
                </td>
                <td>
                    ${topic.qos?.reliability ? `<span class="badge bg-light text-dark">${topic.qos.reliability}</span>` : '-'}
                </td>
                <td>
                    <button class="btn btn-sm btn-outline-danger" onclick="app.deleteTopic('${topic.name}', '${topic.participant}', '${topic.role}')">
                        <i class="bi bi-trash"></i>
                    </button>
                </td>
            </tr>
        `).join('');
    }

    collectAllTopics() {
        const topics = [];
        for (const domain of this.config.domains || []) {
            for (const participant of domain.participants || []) {
                for (const pub of participant.publishers || []) {
                    topics.push({
                        name: pub.topic,
                        type: pub.type || 'unknown',
                        domain_id: domain.id,
                        participant: participant.name,
                        role: 'publisher',
                        qos: pub.qos || {}
                    });
                }
                for (const sub of participant.subscribers || []) {
                    topics.push({
                        name: sub.topic,
                        type: sub.type || 'unknown',
                        domain_id: domain.id,
                        participant: participant.name,
                        role: 'subscriber',
                        qos: sub.qos || {}
                    });
                }
            }
        }
        return topics;
    }

    showAddTopicModal() {
        // Populate domain select
        const domainSelect = document.getElementById('topic-domain');
        domainSelect.innerHTML = (this.config.domains || []).map(d => 
            `<option value="${d.id}">${d.name} (ID: ${d.id})</option>`
        ).join('');

        // Populate participant select
        this.updateTopicParticipantSelect();

        // Listen for domain change
        domainSelect.onchange = () => this.updateTopicParticipantSelect();

        document.getElementById('addTopicForm').reset();
        this.modals.addTopic.show();
    }

    updateTopicParticipantSelect() {
        const domainId = parseInt(document.getElementById('topic-domain').value);
        const domain = this.config.domains?.find(d => d.id === domainId);
        
        const participantSelect = document.getElementById('topic-participant');
        participantSelect.innerHTML = (domain?.participants || []).map(p =>
            `<option value="${p.name}">${p.name}</option>`
        ).join('') || '<option value="">No participants</option>';
    }

    addTopic() {
        const name = document.getElementById('topic-name').value.trim();
        const type = document.getElementById('topic-type').value.trim();
        const domainId = parseInt(document.getElementById('topic-domain').value);
        const participantName = document.getElementById('topic-participant').value;
        const role = document.getElementById('topic-role').value;

        if (!name) {
            this.showToast('Topic name is required', 'danger');
            return;
        }

        if (!participantName) {
            this.showToast('Please select a participant', 'danger');
            return;
        }

        // Find domain and participant
        const domain = this.config.domains?.find(d => d.id === domainId);
        if (!domain) {
            this.showToast('Domain not found', 'danger');
            return;
        }

        let participant = domain.participants?.find(p => p.name === participantName);
        if (!participant) {
            this.showToast('Participant not found', 'danger');
            return;
        }

        // Create topic entry
        const topicEntry = {
            topic: name,
            type: type || 'void*',
            qos: {
                reliability: document.getElementById('topic-reliability').value,
                durability: document.getElementById('topic-durability').value,
                history: {
                    kind: document.getElementById('topic-history-kind').value,
                    depth: parseInt(document.getElementById('topic-history-depth').value) || 1
                }
            }
        };

        // Add to participant
        if (role === 'publisher') {
            if (!participant.publishers) participant.publishers = [];
            participant.publishers.push(topicEntry);
        } else {
            if (!participant.subscribers) participant.subscribers = [];
            participant.subscribers.push(topicEntry);
        }

        this.syncEditorWithConfig();
        this.modals.addTopic.hide();
        this.renderTopics();
        this.updateUI();
        this.showToast(`Topic "${name}" added as ${role}`, 'success');
    }

    deleteTopic(name, participantName, role) {
        if (!confirm(`Delete topic "${name}"?`)) return;

        for (const domain of this.config.domains || []) {
            for (const participant of domain.participants || []) {
                if (participant.name === participantName) {
                    if (role === 'publisher') {
                        participant.publishers = (participant.publishers || []).filter(p => p.topic !== name);
                    } else {
                        participant.subscribers = (participant.subscribers || []).filter(s => s.topic !== name);
                    }
                    this.syncEditorWithConfig();
                    this.renderTopics();
                    this.updateUI();
                    this.showToast('Topic deleted', 'success');
                    return;
                }
            }
        }
    }

    // QoS Management
    renderQoS() {
        const container = document.getElementById('qos-list');
        const profiles = this.config.qos_profiles || {};
        const profileNames = Object.keys(profiles);

        if (profileNames.length === 0) {
            container.innerHTML = '<div class="col"><div class="alert alert-info">No QoS profiles configured</div></div>';
            return;
        }

        container.innerHTML = profileNames.map(name => {
            const profile = profiles[name];
            return `
                <div class="col-md-6 col-lg-4 mb-4">
                    <div class="card h-100">
                        <div class="card-header d-flex justify-content-between">
                            <span><i class="bi bi-sliders"></i> ${this.escapeHtml(name)}</span>
                            <button class="btn btn-sm btn-outline-danger" onclick="app.deleteQoSProfile('${name}')">
                                <i class="bi bi-trash"></i>
                            </button>
                        </div>
                        <div class="card-body">
                            ${profile.reliability ? `<p class="mb-1"><small>Reliability: <span class="badge bg-light text-dark">${profile.reliability}</span></small></p>` : ''}
                            ${profile.durability ? `<p class="mb-1"><small>Durability: <span class="badge bg-light text-dark">${profile.durability}</span></small></p>` : ''}
                            ${profile.liveliness ? `<p class="mb-1"><small>Liveliness: <span class="badge bg-light text-dark">${profile.liveliness}</span></small></p>` : ''}
                            ${profile.history ? `<p class="mb-1"><small>History: <span class="badge bg-light text-dark">${typeof profile.history === 'object' ? profile.history.kind : profile.history}</span></small></p>` : ''}
                        </div>
                    </div>
                </div>
            `;
        }).join('');
    }

    showAddQoSModal() {
        document.getElementById('addQoSForm').reset();
        this.modals.addQoS.show();
    }

    addQoSProfile() {
        const name = document.getElementById('qos-name').value.trim();
        if (!name) {
            this.showToast('Profile name is required', 'danger');
            return;
        }

        if (!this.config.qos_profiles) this.config.qos_profiles = {};
        
        if (this.config.qos_profiles[name]) {
            this.showToast(`Profile "${name}" already exists`, 'danger');
            return;
        }

        const deadlineSec = parseFloat(document.getElementById('qos-deadline-sec').value) || 0;

        this.config.qos_profiles[name] = {
            reliability: document.getElementById('qos-reliability').value,
            durability: document.getElementById('qos-durability').value,
            liveliness: document.getElementById('qos-liveliness').value,
            history: {
                kind: document.getElementById('qos-history-kind').value,
                depth: parseInt(document.getElementById('qos-history-depth').value) || 1
            }
        };

        if (deadlineSec > 0) {
            this.config.qos_profiles[name].deadline = {
                sec: Math.floor(deadlineSec),
                nanosec: Math.floor((deadlineSec % 1) * 1000000000)
            };
        }

        this.syncEditorWithConfig();
        this.modals.addQoS.hide();
        this.renderQoS();
        this.updateUI();
        this.showToast(`QoS profile "${name}" created`, 'success');
    }

    deleteQoSProfile(name) {
        if (!confirm(`Delete QoS profile "${name}"?`)) return;

        delete this.config.qos_profiles[name];
        this.syncEditorWithConfig();
        this.renderQoS();
        this.updateUI();
        this.showToast('QoS profile deleted', 'success');
    }

    // Code Generation
    async generateCode() {
        if (!this.parseEditorContent()) {
            this.showToast('Cannot generate code: Invalid YAML', 'danger');
            return;
        }

        try {
            const response = await fetch(`${this.apiBaseUrl}/api/generate`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-Session-ID': this.sessionId
                },
                body: JSON.stringify({ config: this.config })
            });

            const result = await response.json();

            if (result.success) {
                this.generatedFiles = result.files;
                this.showPreviewFile('dds_config.h');
                this.showToast('Code generated successfully', 'success');
            } else {
                this.showToast(result.error || 'Generation failed', 'danger');
            }
        } catch (error) {
            this.showToast('Error generating code: ' + error.message, 'danger');
        }
    }

    showPreviewFile(filename) {
        document.querySelectorAll('#preview-file-list button').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.file === filename);
        });

        document.getElementById('preview-filename').textContent = filename;
        const content = this.generatedFiles[filename] || '// No content generated';
        document.getElementById('preview-content').querySelector('code').textContent = content;
    }

    async downloadGeneratedCode() {
        if (Object.keys(this.generatedFiles).length === 0) {
            await this.generateCode();
        }

        try {
            const response = await fetch(`${this.apiBaseUrl}/api/generate/download`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-Session-ID': this.sessionId
                },
                body: JSON.stringify({ config: this.config })
            });

            if (response.ok) {
                const blob = await response.blob();
                const url = window.URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = 'dds_generated.zip';
                a.click();
                window.URL.revokeObjectURL(url);
                this.showToast('Download started', 'success');
            } else {
                this.showToast('Download failed', 'danger');
            }
        } catch (error) {
            this.showToast('Error downloading: ' + error.message, 'danger');
        }
    }

    copyPreviewToClipboard() {
        const content = document.getElementById('preview-content').querySelector('code').textContent;
        navigator.clipboard.writeText(content).then(() => {
            this.showToast('Copied to clipboard', 'success');
        });
    }

    // Conversion
    async convertToARXML() {
        if (!this.parseEditorContent()) {
            this.showToast('Cannot convert: Invalid YAML', 'danger');
            return;
        }

        try {
            const response = await fetch(`${this.apiBaseUrl}/api/convert`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-Session-ID': this.sessionId
                },
                body: JSON.stringify({
                    config: this.config,
                    from: 'yaml',
                    to: 'arxml'
                })
            });

            const result = await response.json();

            if (result.success) {
                // Create and download ARXML file
                const blob = new Blob([result.content], { type: 'application/xml' });
                const url = window.URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = 'dds_config.arxml';
                a.click();
                window.URL.revokeObjectURL(url);
                this.showToast('ARXML file downloaded', 'success');
            } else {
                this.showToast(result.error || 'Conversion failed', 'danger');
            }
        } catch (error) {
            this.showToast('Error converting: ' + error.message, 'danger');
        }
    }

    // UI Helpers
    updateUI() {
        // Update counts
        const domainCount = this.config.domains?.length || 0;
        const topicCount = this.collectAllTopics().length;
        const qosCount = Object.keys(this.config.qos_profiles || {}).length;

        document.getElementById('domain-count').textContent = domainCount;
        document.getElementById('topic-count').textContent = topicCount;
        document.getElementById('qos-count').textContent = qosCount;
    }

    showToast(message, type = 'info') {
        const container = document.querySelector('.toast-container');
        const toast = document.createElement('div');
        toast.className = `toast align-items-center text-white bg-${type} border-0`;
        toast.setAttribute('role', 'alert');
        toast.innerHTML = `
            <div class="d-flex">
                <div class="toast-body">${this.escapeHtml(message)}</div>
                <button type="button" class="btn-close btn-close-white me-2 m-auto" data-bs-dismiss="toast"></button>
            </div>
        `;
        container.appendChild(toast);
        const bsToast = new bootstrap.Toast(toast, { delay: 3000 });
        bsToast.show();
        toast.addEventListener('hidden.bs.toast', () => toast.remove());
    }

    // YAML Utilities
    toYAML(obj, indent = 0) {
        if (obj === null || obj === undefined) return 'null';
        if (typeof obj === 'string') return obj.includes(':') || obj.includes('#') ? `"${obj}"` : obj;
        if (typeof obj === 'number' || typeof obj === 'boolean') return String(obj);
        
        const spaces = '  '.repeat(indent);
        let yaml = '';

        if (Array.isArray(obj)) {
            if (obj.length === 0) return '[]';
            for (const item of obj) {
                if (typeof item === 'object' && item !== null) {
                    const itemYaml = this.toYAML(item, indent + 1);
                    yaml += spaces + '- ' + itemYaml.trimStart().replace(/\n/g, '\n' + spaces + '  ') + '\n';
                } else {
                    yaml += spaces + '- ' + this.toYAML(item, 0) + '\n';
                }
            }
        } else {
            for (const [key, value] of Object.entries(obj)) {
                if (value === undefined) continue;
                if (typeof value === 'object' && value !== null) {
                    yaml += spaces + key + ':\n';
                    yaml += this.toYAML(value, indent + 1);
                } else {
                    yaml += spaces + key + ': ' + this.toYAML(value, 0) + '\n';
                }
            }
        }

        return yaml;
    }

    fromYAML(yaml) {
        // Simple YAML parser - for production use a proper library
        const lines = yaml.split('\n');
        const stack = [{}];
        const indentStack = [-1];

        for (let line of lines) {
            // Remove comments
            const commentIdx = line.indexOf('#');
            if (commentIdx >= 0) line = line.slice(0, commentIdx);

            if (!line.trim()) continue;

            const indent = line.search(/\S/);
            const content = line.trim();

            // Pop stack to find parent
            while (indentStack[indentStack.length - 1] >= indent) {
                stack.pop();
                indentStack.pop();
            }

            const parent = stack[stack.length - 1];

            if (content.startsWith('- ')) {
                // Array item
                const value = content.slice(2).trim();
                let arr = parent;
                if (!Array.isArray(parent)) {
                    // Find or create array in parent
                    const lastKey = Object.keys(parent).pop();
                    if (lastKey) {
                        if (!parent[lastKey]) parent[lastKey] = [];
                        arr = parent[lastKey];
                    }
                }
                if (value.includes(':')) {
                    const obj = {};
                    arr.push(obj);
                    stack.push(obj);
                    indentStack.push(indent);
                    // Parse key-value
                    const [k, ...v] = value.split(':');
                    obj[k.trim()] = v.join(':').trim() || {};
                } else {
                    arr.push(this.parseValue(value));
                }
            } else if (content.includes(':')) {
                // Key-value pair
                const colonIdx = content.indexOf(':');
                const key = content.slice(0, colonIdx).trim();
                const value = content.slice(colonIdx + 1).trim();

                if (value) {
                    parent[key] = this.parseValue(value);
                } else {
                    parent[key] = {};
                    stack.push(parent[key]);
                    indentStack.push(indent);
                }
            }
        }

        return stack[0];
    }

    parseValue(value) {
        value = value.trim();
        if (value === 'true') return true;
        if (value === 'false') return false;
        if (value === 'null' || value === '~') return null;
        if (/^-?\d+$/.test(value)) return parseInt(value);
        if (/^-?\d+\.\d+$/.test(value)) return parseFloat(value);
        if ((value.startsWith('"') && value.endsWith('"')) || 
            (value.startsWith("'") && value.endsWith("'"))) {
            return value.slice(1, -1);
        }
        return value;
    }

    escapeHtml(text) {
        if (!text) return '';
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    // Placeholder methods for sidebar actions
    showSystemConfig() {
        this.navigateTo('editor');
        this.showToast('Edit system settings in the YAML editor', 'info');
    }

    showTransportConfig() {
        this.navigateTo('editor');
        this.showToast('Edit transport settings in the YAML editor', 'info');
    }

    showTSNConfig() {
        this.navigateTo('editor');
        this.showToast('Edit TSN settings in the YAML editor', 'info');
    }

    editDomain(id) {
        this.navigateTo('editor');
        this.showToast(`Domain ${id} can be edited in the YAML editor`, 'info');
    }
}

// Initialize application
const app = new DDSConfigApp();
