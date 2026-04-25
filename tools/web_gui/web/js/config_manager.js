/**
 * DDS Configuration Manager
 * Handles import/export, validation, and version comparison of configurations
 */

class ConfigManager {
    constructor() {
        this.currentConfig = null;
        this.configHistory = [];
        this.onConfigChange = null;
        this.onValidationResult = null;
    }

    /**
     * Load configuration from file
     */
    loadFromFile(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = (e) => {
                try {
                    const content = e.target.result;
                    const config = this.parseConfig(content, file.name);
                    this.setCurrentConfig(config);
                    resolve(config);
                } catch (error) {
                    reject(error);
                }
            };
            reader.onerror = () => reject(new Error('Failed to read file'));
            reader.readAsText(file);
        });
    }

    /**
     * Parse configuration from string
     */
    parseConfig(content, filename) {
        let config;
        
        if (filename.endsWith('.json')) {
            config = JSON.parse(content);
        } else if (filename.endsWith('.yaml') || filename.endsWith('.yml')) {
            // Use js-yaml library
            config = jsyaml.load(content);
        } else {
            // Try YAML first, then JSON
            try {
                config = jsyaml.load(content);
            } catch {
                config = JSON.parse(content);
            }
        }

        // Add metadata
        config._metadata = {
            filename: filename,
            loadedAt: new Date().toISOString(),
            format: filename.endsWith('.json') ? 'json' : 'yaml'
        };

        return config;
    }

    /**
     * Export configuration to file
     */
    exportConfig(config, format = 'yaml', filename = null) {
        let content;
        let ext;

        if (format === 'json') {
            content = JSON.stringify(config, null, 2);
            ext = 'json';
        } else {
            content = jsyaml.dump(config, {
                indent: 2,
                lineWidth: -1,
                sortKeys: true
            });
            ext = 'yaml';
        }

        const name = filename || `dds_config_${new Date().toISOString().split('T')[0]}`;
        this.downloadFile(content, `${name}.${ext}`, 
            format === 'json' ? 'application/json' : 'application/yaml');

        return content;
    }

    /**
     * Export as ARXML (AUTOSAR format)
     */
    exportAsARXML(config, filename = null) {
        const arxml = this.convertToARXML(config);
        const name = filename || `dds_config_${new Date().toISOString().split('T')[0]}`;
        this.downloadFile(arxml, `${name}.arxml`, 'application/xml');
        return arxml;
    }

    /**
     * Convert configuration to ARXML format
     */
    convertToARXML(config) {
        const uuid = () => `urn:uuid:${this.generateUUID()}`;
        
        let arxml = `<?xml version="1.0" encoding="UTF-8"?>
<AUTOSAR xmlns="http://autosar.org/schema/r4.0">
  <AR-PACKAGES>
    <AR-PACKAGE UUID="${uuid()}">
      <SHORT-NAME>DDSConfiguration</SHORT-NAME>
      <ELEMENTS>
        <ECU-CONFIGURATION UUID="${uuid()}">
          <SHORT-NAME>DDS_ECU_Config</SHORT-NAME>
          <ECU-ID>${config.system?.name || 'DDS_ECU'}</ECU-ID>
          <CONFIGURATION>
            <DDS-DOMAINS>`;

        // Add domains
        if (config.domains) {
            config.domains.forEach(domain => {
                arxml += `
              <DDS-DOMAIN UUID="${uuid()}">
                <DOMAIN-ID>${domain.id}</DOMAIN-ID>
                <DISCOVERY-ENABLED>${domain.discovery?.enabled !== false}</DISCOVERY-ENABLED>
                <TRANSPORT-PROTOCOL>${domain.discovery?.protocol || 'UDPv4'}</TRANSPORT-PROTOCOL>
              </DDS-DOMAIN>`;
            });
        }

        arxml += `
            </DDS-DOMAINS>
            <DDS-TOPICS>`;

        // Add topics
        if (config.topics) {
            config.topics.forEach(topic => {
                arxml += `
              <DDS-TOPIC UUID="${uuid()}">
                <SHORT-NAME>${topic.name}</SHORT-NAME>
                <DATA-TYPE>${topic.type}</DATA-TYPE>
                <DOMAIN-ID>${topic.domain_id || 0}</DOMAIN-ID>
                <QOS-PROFILE>${topic.qos_profile || 'default'}</QOS-PROFILE>
              </DDS-TOPIC>`;
            });
        }

        arxml += `
            </DDS-TOPICS>
          </CONFIGURATION>
        </ECU-CONFIGURATION>
      </ELEMENTS>
    </AR-PACKAGE>
  </AR-PACKAGES>
</AUTOSAR>`;

        return arxml;
    }

    /**
     * Generate a UUID
     */
    generateUUID() {
        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
            const r = Math.random() * 16 | 0;
            const v = c === 'x' ? r : (r & 0x3 | 0x8);
            return v.toString(16);
        });
    }

    /**
     * Download file to browser
     */
    downloadFile(content, filename, mimeType) {
        const blob = new Blob([content], { type: mimeType });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    /**
     * Set current configuration
     */
    setCurrentConfig(config) {
        // Save to history
        if (this.currentConfig) {
            this.configHistory.push({
                config: JSON.parse(JSON.stringify(this.currentConfig)),
                timestamp: new Date().toISOString()
            });
            // Keep only last 20 versions
            if (this.configHistory.length > 20) {
                this.configHistory.shift();
            }
        }

        this.currentConfig = config;

        if (this.onConfigChange) {
            this.onConfigChange(config);
        }
    }

    /**
     * Get current configuration
     */
    getCurrentConfig() {
        return this.currentConfig;
    }

    /**
     * Validate current configuration
     */
    async validateConfig(config = null) {
        const cfg = config || this.currentConfig;
        if (!cfg) {
            throw new Error('No configuration loaded');
        }

        // Send to backend for validation
        try {
            const response = await fetch('/api/config/validate', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(cfg)
            });

            const result = await response.json();
            
            if (this.onValidationResult) {
                this.onValidationResult(result);
            }

            return result;
        } catch (error) {
            // Fallback to client-side validation
            return this.clientSideValidation(cfg);
        }
    }

    /**
     * Basic client-side validation
     */
    clientSideValidation(config) {
        const issues = [];

        // Check required fields
        if (!config.version) {
            issues.push({
                level: 'error',
                code: 'MISSING_VERSION',
                message: 'Configuration version is required',
                path: ''
            });
        }

        if (!config.system?.name) {
            issues.push({
                level: 'error',
                code: 'MISSING_SYSTEM_NAME',
                message: 'System name is required',
                path: 'system'
            });
        }

        // Check domain IDs
        if (config.domains) {
            const domainIds = new Set();
            config.domains.forEach((domain, i) => {
                if (domain.id === undefined) {
                    issues.push({
                        level: 'error',
                        code: 'MISSING_DOMAIN_ID',
                        message: `Domain at index ${i} is missing ID`,
                        path: `domains[${i}]`
                    });
                } else if (domain.id < 0 || domain.id > 230) {
                    issues.push({
                        level: 'error',
                        code: 'INVALID_DOMAIN_ID',
                        message: `Domain ID ${domain.id} is out of range (0-230)`,
                        path: `domains[${i}].id`
                    });
                } else if (domainIds.has(domain.id)) {
                    issues.push({
                        level: 'error',
                        code: 'DUPLICATE_DOMAIN_ID',
                        message: `Duplicate domain ID: ${domain.id}`,
                        path: `domains[${i}].id`
                    });
                }
                domainIds.add(domain.id);
            });
        }

        // Check topic references
        if (config.topics) {
            const topicNames = new Set();
            config.topics.forEach((topic, i) => {
                if (!topic.name) {
                    issues.push({
                        level: 'error',
                        code: 'MISSING_TOPIC_NAME',
                        message: `Topic at index ${i} is missing name`,
                        path: `topics[${i}]`
                    });
                } else if (topicNames.has(topic.name)) {
                    issues.push({
                        level: 'error',
                        code: 'DUPLICATE_TOPIC_NAME',
                        message: `Duplicate topic name: ${topic.name}`,
                        path: `topics[${i}].name`
                    });
                }
                topicNames.add(topic.name);

                if (!topic.type) {
                    issues.push({
                        level: 'warning',
                        code: 'MISSING_TOPIC_TYPE',
                        message: `Topic '${topic.name}' is missing type`,
                        path: `topics[${i}].type`
                    });
                }
            });
        }

        return {
            is_valid: !issues.some(i => i.level === 'error'),
            error_count: issues.filter(i => i.level === 'error').length,
            warning_count: issues.filter(i => i.level === 'warning').length,
            issues: issues
        };
    }

    /**
     * Compare two configurations
     */
    compareConfigs(oldConfig, newConfig) {
        const differences = [];

        this.compareObjects(oldConfig, newConfig, '', differences);

        return {
            added: differences.filter(d => d.type === 'added'),
            removed: differences.filter(d => d.type === 'removed'),
            modified: differences.filter(d => d.type === 'modified'),
            all: differences
        };
    }

    /**
     * Recursively compare objects
     */
    compareObjects(oldObj, newObj, path, differences) {
        const oldKeys = oldObj ? Object.keys(oldObj) : [];
        const newKeys = newObj ? Object.keys(newObj) : [];

        // Find removed keys
        oldKeys.forEach(key => {
            if (key.startsWith('_')) return; // Skip metadata
            
            const newPath = path ? `${path}.${key}` : key;
            
            if (!(key in newObj)) {
                differences.push({
                    type: 'removed',
                    path: newPath,
                    oldValue: oldObj[key]
                });
            } else if (typeof oldObj[key] === 'object' && oldObj[key] !== null &&
                       typeof newObj[key] === 'object' && newObj[key] !== null) {
                this.compareObjects(oldObj[key], newObj[key], newPath, differences);
            } else if (JSON.stringify(oldObj[key]) !== JSON.stringify(newObj[key])) {
                differences.push({
                    type: 'modified',
                    path: newPath,
                    oldValue: oldObj[key],
                    newValue: newObj[key]
                });
            }
        });

        // Find added keys
        newKeys.forEach(key => {
            if (key.startsWith('_')) return;
            
            const newPath = path ? `${path}.${key}` : key;
            
            if (!(key in oldObj)) {
                differences.push({
                    type: 'added',
                    path: newPath,
                    newValue: newObj[key]
                });
            }
        });
    }

    /**
     * Get configuration history
     */
    getHistory() {
        return this.configHistory;
    }

    /**
     * Restore configuration from history
     */
    restoreFromHistory(index) {
        if (index >= 0 && index < this.configHistory.length) {
            const historical = this.configHistory[index];
            this.setCurrentConfig(JSON.parse(JSON.stringify(historical.config)));
            return historical;
        }
        return null;
    }

    /**
     * Create configuration from template
     */
    createFromTemplate(templateName) {
        const templates = {
            'minimal': {
                version: '1.0.0',
                system: { name: 'MinimalDDS', version: '1.0.0' },
                domains: [{ id: 0, name: 'Default' }],
                topics: []
            },
            'automotive': {
                version: '1.0.0',
                system: { name: 'AutomotiveDDS', version: '1.0.0' },
                domains: [
                    { id: 0, name: 'Powertrain' },
                    { id: 1, name: 'Chassis' },
                    { id: 2, name: 'Body' }
                ],
                topics: [
                    { name: 'VehicleSpeed', type: 'SpeedData', domain_id: 0 },
                    { name: 'EngineTemp', type: 'TemperatureData', domain_id: 0 }
                ],
                qos_profiles: [
                    { name: 'sensor', reliability: { kind: 'BEST_EFFORT' } },
                    { name: 'control', reliability: { kind: 'RELIABLE' } }
                ]
            },
            'sensor-network': {
                version: '1.0.0',
                system: { name: 'SensorNetwork', version: '1.0.0' },
                domains: [{ id: 0, name: 'Sensors' }],
                topics: [
                    { name: 'Temperature', type: 'SensorData', qos_profile: 'sensor' },
                    { name: 'Pressure', type: 'SensorData', qos_profile: 'sensor' },
                    { name: 'Humidity', type: 'SensorData', qos_profile: 'sensor' }
                ]
            }
        };

        const template = templates[templateName];
        if (template) {
            this.setCurrentConfig(JSON.parse(JSON.stringify(template)));
        }
        return template;
    }

    /**
     * Register change callback
     */
    onChange(callback) {
        this.onConfigChange = callback;
    }

    /**
     * Register validation callback
     */
    onValidation(callback) {
        this.onValidationResult = callback;
    }
}

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
    module.exports = ConfigManager;
}