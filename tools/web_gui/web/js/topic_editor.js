/**
 * DDS Topic Editor
 * Visual topic configuration editor with QoS policy management
 */

class TopicEditor {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.topics = [];
        this.selectedTopic = null;
        this.onChange = null;
    }

    init() {
        this.render();
        this.attachEventListeners();
    }

    render() {
        this.container.innerHTML = `
            <div class="topic-editor">
                <div class="topic-list-panel">
                    <div class="panel-header">
                        <h3>Topics</h3>
                        <button id="btn-add-topic" class="btn btn-sm btn-primary">
                            <i class="fas fa-plus"></i> Add
                        </button>
                    </div>
                    <div id="topic-list" class="topic-list"></div>
                </div>
                <div class="topic-detail-panel">
                    <div id="topic-form" class="topic-form">
                        <div class="empty-state">
                            <i class="fas fa-arrow-left"></i>
                            <p>Select a topic to edit or create a new one</p>
                        </div>
                    </div>
                </div>
            </div>
        `;
        this.renderTopicList();
    }

    renderTopicList() {
        const listEl = document.getElementById('topic-list');
        if (this.topics.length === 0) {
            listEl.innerHTML = '<div class="empty-list">No topics configured</div>';
            return;
        }

        listEl.innerHTML = this.topics.map((topic, index) => `
            <div class="topic-item ${this.selectedTopic === index ? 'selected' : ''}" 
                 data-index="${index}">
                <div class="topic-name">${topic.name || 'Unnamed'}</div>
                <div class="topic-type">${topic.type || 'Unknown'}</div>
                <button class="btn-delete-topic" data-index="${index}">
                    <i class="fas fa-trash"></i>
                </button>
            </div>
        `).join('');
    }

    renderTopicForm(topic = null, index = null) {
        const formEl = document.getElementById('topic-form');
        const isNew = topic === null;
        const t = topic || { name: '', type: '', domain_id: 0, qos_profile: '', partitions: [] };

        formEl.innerHTML = `
            <div class="form-header">
                <h4>${isNew ? 'New Topic' : 'Edit Topic'}</h4>
                <div class="form-actions">
                    <button id="btn-save-topic" class="btn btn-primary">
                        <i class="fas fa-save"></i> Save
                    </button>
                    ${!isNew ? `
                        <button id="btn-clone-topic" class="btn btn-secondary">
                            <i class="fas fa-copy"></i> Clone
                        </button>
                    ` : ''}
                </div>
            </div>
            
            <div class="form-section">
                <h5>Basic Configuration</h5>
                <div class="form-group">
                    <label>Topic Name *</label>
                    <input type="text" id="topic-name" value="${t.name}" 
                           placeholder="e.g., VehicleSpeed" class="form-control">
                    <small>Use alphanumeric characters and underscores</small>
                </div>
                
                <div class="form-row">
                    <div class="form-group">
                        <label>Data Type *</label>
                        <input type="text" id="topic-type" value="${t.type}" 
                               placeholder="e.g., SpeedData" class="form-control"
                               list="type-suggestions">
                        <datalist id="type-suggestions">
                            <option value="VehicleData">
                            <option value="SensorData">
                            <option value="ControlCommand">
                            <option value="DiagnosticData">
                        </datalist>
                    </div>
                    
                    <div class="form-group">
                        <label>Domain ID</label>
                        <input type="number" id="topic-domain" value="${t.domain_id}" 
                               min="0" max="230" class="form-control">
                    </div>
                </div>
            </div>

            <div class="form-section">
                <h5>QoS Configuration</h5>
                <div class="form-group">
                    <label>QoS Profile</label>
                    <select id="topic-qos" class="form-control">
                        <option value="">-- Custom --</option>
                        <option value="default" ${t.qos_profile === 'default' ? 'selected' : ''}>Default</option>
                        <option value="reliable" ${t.qos_profile === 'reliable' ? 'selected' : ''}>Reliable</option>
                        <option value="best_effort" ${t.qos_profile === 'best_effort' ? 'selected' : ''}>Best Effort</option>
                        <option value="realtime" ${t.qos_profile === 'realtime' ? 'selected' : ''}>Real-time</option>
                        <option value="sensor" ${t.qos_profile === 'sensor' ? 'selected' : ''}>Sensor Data</option>
                    </select>
                </div>

                <div id="custom-qos-panel" class="custom-qos-panel ${t.qos_profile ? 'hidden' : ''}">
                    <div class="qos-tabs">
                        <button class="qos-tab active" data-tab="reliability">Reliability</button>
                        <button class="qos-tab" data-tab="durability">Durability</button>
                        <button class="qos-tab" data-tab="history">History</button>
                        <button class="qos-tab" data-tab="deadline">Deadline</button>
                        <button class="qos-tab" data-tab="advanced">Advanced</button>
                    </div>
                    
                    <div class="qos-tab-content" id="tab-reliability">
                        <div class="form-group">
                            <label>Reliability Kind</label>
                            <select id="qos-reliability-kind" class="form-control">
                                <option value="BEST_EFFORT">Best Effort</option>
                                <option value="RELIABLE">Reliable</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label>Max Blocking Time</label>
                            <input type="text" id="qos-max-blocking" value="100ms" class="form-control">
                        </div>
                    </div>
                    
                    <div class="qos-tab-content hidden" id="tab-durability">
                        <div class="form-group">
                            <label>Durability Kind</label>
                            <select id="qos-durability-kind" class="form-control">
                                <option value="VOLATILE">Volatile</option>
                                <option value="TRANSIENT_LOCAL">Transient Local</option>
                                <option value="TRANSIENT">Transient</option>
                                <option value="PERSISTENT">Persistent</option>
                            </select>
                        </div>
                    </div>
                    
                    <div class="qos-tab-content hidden" id="tab-history">
                        <div class="form-group">
                            <label>History Kind</label>
                            <select id="qos-history-kind" class="form-control">
                                <option value="KEEP_LAST">Keep Last</option>
                                <option value="KEEP_ALL">Keep All</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label>History Depth</label>
                            <input type="number" id="qos-history-depth" value="1" min="1" max="65535" class="form-control">
                        </div>
                    </div>
                    
                    <div class="qos-tab-content hidden" id="tab-deadline">
                        <div class="form-group">
                            <label>Deadline Period</label>
                            <input type="text" id="qos-deadline" value="INFINITE" class="form-control">
                            <small>Use format like 100ms, 1s, or INFINITE</small>
                        </div>
                        <div class="form-group">
                            <label>Latency Budget</label>
                            <input type="text" id="qos-latency" value="0s" class="form-control">
                        </div>
                    </div>
                    
                    <div class="qos-tab-content hidden" id="tab-advanced">
                        <div class="form-group">
                            <label>Liveliness</label>
                            <select id="qos-liveliness-kind" class="form-control">
                                <option value="AUTOMATIC">Automatic</option>
                                <option value="MANUAL_BY_PARTICIPANT">Manual by Participant</option>
                                <option value="MANUAL_BY_TOPIC">Manual by Topic</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label>Ownership</label>
                            <select id="qos-ownership-kind" class="form-control">
                                <option value="SHARED">Shared</option>
                                <option value="EXCLUSIVE">Exclusive</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label>Transport Priority</label>
                            <input type="number" id="qos-priority" value="0" min="0" class="form-control">
                        </div>
                    </div>
                </div>
            </div>

            <div class="form-section">
                <h5>Partitions & Filters</h5>
                <div class="form-group">
                    <label>Partitions</label>
                    <div id="partitions-list" class="tag-list">
                        ${(t.partitions || []).map(p => `
                            <span class="tag">${p} <i class="fas fa-times remove-tag"></i></span>
                        `).join('')}
                    </div>
                    <input type="text" id="new-partition" placeholder="Add partition..." class="form-control">
                </div>
                
                <div class="form-group">
                    <label>Content Filter (optional)</label>
                    <textarea id="topic-filter" class="form-control" rows="2" 
                              placeholder="e.g., speed > 0">${t.filter || ''}</textarea>
                </div>
            </div>
        `;

        this.attachFormListeners(index);
    }

    attachEventListeners() {
        // Add topic button
        document.getElementById('btn-add-topic')?.addEventListener('click', () => {
            this.selectedTopic = null;
            this.renderTopicList();
            this.renderTopicForm();
        });

        // Topic list selection
        document.getElementById('topic-list')?.addEventListener('click', (e) => {
            const item = e.target.closest('.topic-item');
            if (item) {
                const index = parseInt(item.dataset.index);
                this.selectedTopic = index;
                this.renderTopicList();
                this.renderTopicForm(this.topics[index], index);
            }

            const deleteBtn = e.target.closest('.btn-delete-topic');
            if (deleteBtn) {
                const index = parseInt(deleteBtn.dataset.index);
                this.deleteTopic(index);
            }
        });
    }

    attachFormListeners(index) {
        // QoS tab switching
        document.querySelectorAll('.qos-tab').forEach(tab => {
            tab.addEventListener('click', () => {
                document.querySelectorAll('.qos-tab').forEach(t => t.classList.remove('active'));
                document.querySelectorAll('.qos-tab-content').forEach(c => c.classList.add('hidden'));
                
                tab.classList.add('active');
                const tabId = `tab-${tab.dataset.tab}`;
                document.getElementById(tabId)?.classList.remove('hidden');
            });
        });

        // QoS profile selection
        document.getElementById('topic-qos')?.addEventListener('change', (e) => {
            const customPanel = document.getElementById('custom-qos-panel');
            if (e.target.value === '') {
                customPanel.classList.remove('hidden');
            } else {
                customPanel.classList.add('hidden');
            }
        });

        // Save button
        document.getElementById('btn-save-topic')?.addEventListener('click', () => {
            this.saveTopic(index);
        });

        // Clone button
        document.getElementById('btn-clone-topic')?.addEventListener('click', () => {
            this.cloneTopic(index);
        });

        // Partitions
        const partitionInput = document.getElementById('new-partition');
        partitionInput?.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                e.preventDefault();
                this.addPartition(partitionInput.value);
                partitionInput.value = '';
            }
        });

        document.getElementById('partitions-list')?.addEventListener('click', (e) => {
            if (e.target.classList.contains('remove-tag') || e.target.closest('.remove-tag')) {
                e.target.closest('.tag').remove();
            }
        });
    }

    saveTopic(index) {
        const topic = {
            name: document.getElementById('topic-name').value,
            type: document.getElementById('topic-type').value,
            domain_id: parseInt(document.getElementById('topic-domain').value) || 0,
            qos_profile: document.getElementById('topic-qos').value,
            partitions: Array.from(document.querySelectorAll('#partitions-list .tag')).map(
                tag => tag.textContent.trim()
            ),
            filter: document.getElementById('topic-filter').value
        };

        // Collect custom QoS if no profile selected
        if (!topic.qos_profile) {
            topic.qos = {
                reliability: {
                    kind: document.getElementById('qos-reliability-kind').value,
                    max_blocking_time: document.getElementById('qos-max-blocking').value
                },
                durability: {
                    kind: document.getElementById('qos-durability-kind').value
                },
                history: {
                    kind: document.getElementById('qos-history-kind').value,
                    depth: parseInt(document.getElementById('qos-history-depth').value) || 1
                },
                deadline: {
                    period: document.getElementById('qos-deadline').value
                },
                latency_budget: {
                    duration: document.getElementById('qos-latency').value
                },
                liveliness: {
                    kind: document.getElementById('qos-liveliness-kind').value
                },
                ownership: {
                    kind: document.getElementById('qos-ownership-kind').value
                },
                transport_priority: {
                    value: parseInt(document.getElementById('qos-priority').value) || 0
                }
            };
        }

        if (index !== null) {
            this.topics[index] = topic;
        } else {
            this.topics.push(topic);
            this.selectedTopic = this.topics.length - 1;
        }

        this.renderTopicList();
        this.notifyChange();
    }

    deleteTopic(index) {
        if (confirm('Are you sure you want to delete this topic?')) {
            this.topics.splice(index, 1);
            if (this.selectedTopic === index) {
                this.selectedTopic = null;
                this.renderTopicForm();
            } else if (this.selectedTopic > index) {
                this.selectedTopic--;
            }
            this.renderTopicList();
            this.notifyChange();
        }
    }

    cloneTopic(index) {
        const topic = JSON.parse(JSON.stringify(this.topics[index]));
        topic.name = topic.name + '_copy';
        this.topics.push(topic);
        this.selectedTopic = this.topics.length - 1;
        this.renderTopicList();
        this.renderTopicForm(topic, this.selectedTopic);
        this.notifyChange();
    }

    addPartition(name) {
        if (name.trim()) {
            const list = document.getElementById('partitions-list');
            const tag = document.createElement('span');
            tag.className = 'tag';
            tag.innerHTML = `${name.trim()} <i class="fas fa-times remove-tag"></i>`;
            list.appendChild(tag);
        }
    }

    loadTopics(topics) {
        this.topics = topics || [];
        this.selectedTopic = null;
        this.renderTopicList();
        this.renderTopicForm();
    }

    getTopics() {
        return this.topics;
    }

    onChange(callback) {
        this.onChange = callback;
    }

    notifyChange() {
        if (this.onChange) {
            this.onChange(this.topics);
        }
    }
}

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
    module.exports = TopicEditor;
}