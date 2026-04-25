/**
 * DDS QoS Configuration Wizard
 * Interactive wizard for configuring QoS policies based on use case
 */

class QoSWizard {
    constructor(containerId) {
        this.container = document.getElementById(containerId);
        this.currentStep = 0;
        this.answers = {};
        this.onComplete = null;
        
        this.steps = [
            {
                id: 'use-case',
                title: 'Select Use Case',
                description: 'What is the primary purpose of this configuration?',
                options: [
                    { id: 'sensor', label: 'Sensor Data', icon: 'fa-thermometer-half', 
                      desc: 'Periodic sensor readings, tolerant of occasional loss' },
                    { id: 'control', label: 'Real-time Control', icon: 'fa-cogs',
                      desc: 'Critical control commands requiring reliability' },
                    { id: 'video', label: 'Video Streaming', icon: 'fa-video',
                      desc: 'High bandwidth continuous data stream' },
                    { id: 'event', label: 'Event Notifications', icon: 'fa-bell',
                      desc: 'Important alerts that must be delivered' },
                    { id: 'config', label: 'Configuration', icon: 'fa-sliders-h',
                      desc: 'Setup data, must persist across restarts' },
                    { id: 'custom', label: 'Custom', icon: 'fa-wrench',
                      desc: 'Manually configure all options' }
                ]
            },
            {
                id: 'latency',
                title: 'Latency Requirements',
                description: 'What are your latency constraints?',
                condition: (answers) => answers.useCase !== 'config',
                options: [
                    { id: 'realtime', label: 'Hard Real-time', 
                      desc: '< 1ms latency required' },
                    { id: 'low', label: 'Low Latency',
                      desc: '< 10ms acceptable' },
                    { id: 'medium', label: 'Medium Latency',
                      desc: '< 100ms acceptable' },
                    { id: 'high', label: 'High Latency OK',
                      desc: '> 100ms acceptable' }
                ]
            },
            {
                id: 'reliability',
                title: 'Data Reliability',
                description: 'How important is it that every sample is delivered?',
                condition: (answers) => !['sensor', 'video'].includes(answers.useCase),
                options: [
                    { id: 'critical', label: 'Mission Critical',
                      desc: 'No data loss is acceptable' },
                    { id: 'important', label: 'Important',
                      desc: 'Occasional loss is tolerable' },
                    { id: 'best-effort', label: 'Best Effort',
                      desc: 'Latest data is most important' }
                ]
            },
            {
                id: 'bandwidth',
                title: 'Bandwidth Constraints',
                description: 'What are your network constraints?',
                options: [
                    { id: 'limited', label: 'Limited Bandwidth',
                      desc: 'Need to minimize network usage' },
                    { id: 'moderate', label: 'Moderate',
                      desc: 'Normal bandwidth available' },
                    { id: 'abundant', label: 'Abundant',
                      desc: 'High bandwidth available' }
                ]
            },
            {
                id: 'durability',
                title: 'Data Persistence',
                description: 'Should data be available to late-joining subscribers?',
                condition: (answers) => ['config', 'event'].includes(answers.useCase),
                options: [
                    { id: 'persistent', label: 'Persistent',
                      desc: 'Data survives system restarts' },
                    { id: 'transient', label: 'Transient',
                      desc: 'Data available while system runs' },
                    { id: 'volatile', label: 'Volatile',
                      desc: 'Only available at publish time' }
                ]
            }
        ];
    }

    init() {
        this.render();
        this.showStep(0);
    }

    render() {
        this.container.innerHTML = `
            <div class="qos-wizard">
                <div class="wizard-header">
                    <h3>QoS Configuration Wizard</h3>
                    <div class="progress-bar">
                        <div class="progress-fill" style="width: 0%"></div>
                    </div>
                    <div class="step-indicator">
                        Step <span class="current-step">1</span> of <span class="total-steps">${this.getVisibleStepCount()}</span>
                    </div>
                </div>
                
                <div class="wizard-content">
                    <div class="step-container"></div>
                </div>
                
                <div class="wizard-footer">
                    <button class="btn btn-secondary" id="wizard-prev" disabled>
                        <i class="fas fa-arrow-left"></i> Previous
                    </button>
                    <button class="btn btn-primary" id="wizard-next">
                        Next <i class="fas fa-arrow-right"></i>
                    </button>
                </div>
            </div>
        `;

        this.attachEventListeners();
    }

    getVisibleStepCount() {
        return this.steps.filter(s => !s.condition || s.condition(this.answers)).length;
    }

    getVisibleSteps() {
        return this.steps.filter(s => !s.condition || s.condition(this.answers));
    }

    showStep(index) {
        const visibleSteps = this.getVisibleSteps();
        if (index < 0 || index >= visibleSteps.length) return;

        this.currentStep = index;
        const step = visibleSteps[index];

        // Update progress
        const progress = ((index + 1) / visibleSteps.length) * 100;
        this.container.querySelector('.progress-fill').style.width = `${progress}%`;
        this.container.querySelector('.current-step').textContent = index + 1;
        this.container.querySelector('.total-steps').textContent = visibleSteps.length;

        // Update buttons
        const prevBtn = document.getElementById('wizard-prev');
        const nextBtn = document.getElementById('wizard-next');

        prevBtn.disabled = index === 0;
        nextBtn.innerHTML = index === visibleSteps.length - 1 ? 
            'Finish <i class="fas fa-check"></i>' : 
            'Next <i class="fas fa-arrow-right"></i>';

        // Render step content
        const container = this.container.querySelector('.step-container');
        container.innerHTML = `
            <div class="step" data-step="${step.id}">
                <h4>${step.title}</h4>
                <p class="step-description">${step.description}</p>
                <div class="options-grid">
                    ${step.options.map(opt => `
                        <div class="option-card ${this.answers[step.id] === opt.id ? 'selected' : ''}" 
                             data-value="${opt.id}">
                            ${opt.icon ? `<i class="fas ${opt.icon}"></i>` : ''}
                            <h5>${opt.label}</h5>
                            <p>${opt.desc}</p>
                        </div>
                    `).join('')}
                </div>
            </div>
        `;

        // Animate in
        container.querySelector('.step').classList.add('fade-in');
    }

    attachEventListeners() {
        // Navigation buttons
        document.getElementById('wizard-prev')?.addEventListener('click', () => {
            this.showStep(this.currentStep - 1);
        });

        document.getElementById('wizard-next')?.addEventListener('click', () => {
            const visibleSteps = this.getVisibleSteps();
            if (this.currentStep < visibleSteps.length - 1) {
                this.showStep(this.currentStep + 1);
            } else {
                this.finish();
            }
        });

        // Option selection
        this.container.addEventListener('click', (e) => {
            const card = e.target.closest('.option-card');
            if (card) {
                const stepId = this.container.querySelector('.step').dataset.step;
                const value = card.dataset.value;

                // Remove selection from other cards
                this.container.querySelectorAll('.option-card').forEach(c => {
                    c.classList.remove('selected');
                });
                card.classList.add('selected');

                // Store answer
                this.answers[stepId] = value;

                // Auto-advance after short delay
                setTimeout(() => {
                    const visibleSteps = this.getVisibleSteps();
                    if (this.currentStep < visibleSteps.length - 1) {
                        this.showStep(this.currentStep + 1);
                    }
                }, 300);
            }
        });
    }

    generateQoSProfile() {
        const qos = {
            name: this.generateProfileName(),
            description: this.generateDescription()
        };

        // Map answers to QoS policies
        switch (this.answers.useCase) {
            case 'sensor':
                qos.reliability = { kind: 'BEST_EFFORT' };
                qos.durability = { kind: 'VOLATILE' };
                qos.history = { kind: 'KEEP_LAST', depth: 1 };
                qos.deadline = { period: '100ms' };
                break;

            case 'control':
                qos.reliability = { kind: 'RELIABLE', max_blocking_time: '100ms' };
                qos.durability = { kind: 'VOLATILE' };
                qos.history = { kind: 'KEEP_LAST', depth: 1 };
                qos.deadline = { period: '10ms' };
                qos.latency_budget = { duration: '1ms' };
                break;

            case 'video':
                qos.reliability = { kind: 'BEST_EFFORT' };
                qos.durability = { kind: 'VOLATILE' };
                qos.history = { kind: 'KEEP_LAST', depth: 5 };
                qos.resource_limits = {
                    max_samples: 10,
                    max_instances: 1,
                    max_samples_per_instance: 10
                };
                break;

            case 'event':
                qos.reliability = { kind: 'RELIABLE', max_blocking_time: '1s' };
                qos.history = { kind: 'KEEP_ALL' };
                if (this.answers.durability === 'persistent') {
                    qos.durability = { kind: 'PERSISTENT' };
                } else if (this.answers.durability === 'transient') {
                    qos.durability = { kind: 'TRANSIENT' };
                } else {
                    qos.durability = { kind: 'TRANSIENT_LOCAL' };
                }
                break;

            case 'config':
                qos.reliability = { kind: 'RELIABLE' };
                qos.durability = { kind: 'TRANSIENT_LOCAL' };
                qos.history = { kind: 'KEEP_LAST', depth: 1 };
                qos.lifespan = { duration: '1h' };
                break;
        }

        // Adjust based on latency requirements
        switch (this.answers.latency) {
            case 'realtime':
                qos.latency_budget = { duration: '100us' };
                qos.transport_priority = { value: 255 };
                break;
            case 'low':
                qos.latency_budget = { duration: '1ms' };
                qos.transport_priority = { value: 200 };
                break;
            case 'medium':
                qos.latency_budget = { duration: '10ms' };
                break;
        }

        // Adjust based on bandwidth
        switch (this.answers.bandwidth) {
            case 'limited':
                qos.time_based_filter = { minimum_separation: '100ms' };
                qos.ownership = { kind: 'EXCLUSIVE' };
                break;
            case 'abundant':
                qos.destination_order = { kind: 'BY_SOURCE_TIMESTAMP' };
                break;
        }

        return qos;
    }

    generateProfileName() {
        const parts = [this.answers.useCase];
        if (this.answers.latency) parts.push(this.answers.latency);
        if (this.answers.reliability) parts.push(this.answers.reliability);
        return parts.join('_');
    }

    generateDescription() {
        const parts = [];
        parts.push(`Use case: ${this.answers.useCase}`);
        if (this.answers.latency) parts.push(`Latency: ${this.answers.latency}`);
        if (this.answers.reliability) parts.push(`Reliability: ${this.answers.reliability}`);
        if (this.answers.bandwidth) parts.push(`Bandwidth: ${this.answers.bandwidth}`);
        return parts.join(', ');
    }

    finish() {
        const qosProfile = this.generateQoSProfile();
        
        // Show summary
        const container = this.container.querySelector('.step-container');
        container.innerHTML = `
            <div class="step summary-step">
                <h4>Configuration Complete</h4>
                <div class="qos-summary">
                    <div class="summary-item">
                        <label>Profile Name:</label>
                        <span>${qosProfile.name}</span>
                    </div>
                    <div class="summary-item">
                        <label>Description:</label>
                        <span>${qosProfile.description}</span>
                    </div>
                    <div class="qos-policies">
                        <h5>QoS Policies:</h5>
                        <pre>${JSON.stringify(qosProfile, null, 2)}</pre>
                    </div>
                </div>
                <div class="summary-actions">
                    <button class="btn btn-primary" id="wizard-use-profile">
                        <i class="fas fa-check"></i> Use This Profile
                    </button>
                    <button class="btn btn-secondary" id="wizard-edit">
                        <i class="fas fa-edit"></i> Edit Manually
                    </button>
                    <button class="btn btn-secondary" id="wizard-restart">
                        <i class="fas fa-redo"></i> Start Over
                    </button>
                </div>
            </div>
        `;

        // Hide navigation
        this.container.querySelector('.wizard-footer').style.display = 'none';

        // Attach summary actions
        document.getElementById('wizard-use-profile')?.addEventListener('click', () => {
            if (this.onComplete) {
                this.onComplete(qosProfile);
            }
        });

        document.getElementById('wizard-edit')?.addEventListener('click', () => {
            // Switch to manual QoS editor
            if (this.onComplete) {
                this.onComplete(qosProfile, true);
            }
        });

        document.getElementById('wizard-restart')?.addEventListener('click', () => {
            this.answers = {};
            this.currentStep = 0;
            this.container.querySelector('.wizard-footer').style.display = 'flex';
            this.showStep(0);
        });
    }

    onComplete(callback) {
        this.onComplete = callback;
    }
}

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
    module.exports = QoSWizard;
}