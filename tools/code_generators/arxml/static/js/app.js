/**
 * yuleASR ARXML Generator - Frontend Application
 * 
 * 可视化配置工具的前端逻辑
 */

// 全局状态
let currentModule = null;
let currentSchema = null;
let moduleConfig = {};
let generatedArxml = '';

// 初始化应用
document.addEventListener('DOMContentLoaded', () => {
    loadModules();
});

/**
 * 加载模块列表
 */
async function loadModules() {
    try {
        const response = await fetch('/api/modules');
        const modules = await response.json();
        
        // 渲染MCAL模块
        const mcalContainer = document.getElementById('mcal-modules');
        mcalContainer.innerHTML = modules.mcal.map(mod => `
            <div class="sidebar-item flex items-center px-3 py-2 rounded cursor-pointer" 
                 onclick="selectModule('${mod.id}', '${mod.name}')">
                <i class="fas fa-${mod.icon} w-6 text-gray-400"></i>
                <div class="ml-2">
                    <div class="text-sm font-medium">${mod.name}</div>
                    <div class="text-xs text-gray-500">${mod.description}</div>
                </div>
            </div>
        `).join('');
        
        // 渲染BSW模块
        const bswContainer = document.getElementById('bsw-modules');
        bswContainer.innerHTML = modules.bsw.map(mod => `
            <div class="sidebar-item flex items-center px-3 py-2 rounded cursor-pointer"
                 onclick="selectModule('${mod.id}', '${mod.name}')">
                <i class="fas fa-${mod.icon} w-6 text-gray-400"></i>
                <div class="ml-2">
                    <div class="text-sm font-medium">${mod.name}</div>
                    <div class="text-xs text-gray-500">${mod.description}</div>
                </div>
            </div>
        `).join('');
        
    } catch (error) {
        console.error('加载模块失败:', error);
        showToast('加载模块列表失败', 'error');
    }
}

/**
 * 选择模块
 */
async function selectModule(moduleId, moduleName) {
    currentModule = moduleId;
    
    // 更新UI选中状态
    document.querySelectorAll('.sidebar-item').forEach(el => {
        el.classList.remove('active');
    });
    event.currentTarget.classList.add('active');
    
    // 更新顶部显示
    document.getElementById('current-module').textContent = moduleName;
    
    // 加载模块配置索引
    try {
        const response = await fetch(`/api/schema/${moduleId}`);
        currentSchema = await response.json();
        
        // 初始化配置数据
        moduleConfig = {};
        currentSchema.containers.forEach(container => {
            if (container.is_list) {
                moduleConfig[container.name] = [];
            } else {
                moduleConfig[container.name] = {};
                container.parameters.forEach(param => {
                    moduleConfig[container.name][param.name] = param.default;
                });
            }
        });
        
        // 渲染配置表单
        renderConfigForm();
        
    } catch (error) {
        console.error('加载模块索引失败:', error);
        showToast('加载模块配置失败', 'error');
    }
}

/**
 * 渲染配置表单
 */
function renderConfigForm() {
    const container = document.getElementById('config-area');
    
    if (!currentSchema) {
        container.innerHTML = `
            <div class="text-center text-gray-400 mt-20">
                <i class="fas fa-arrow-left text-4xl mb-4"></i>
                <p class="text-lg">请从左侧选择一个模块进行配置</p>
            </div>
        `;
        return;
    }
    
    container.innerHTML = currentSchema.containers.map((container, index) => `
        <div class="config-card bg-white rounded-lg shadow border border-gray-200 mb-6">
            <div class="bg-gray-50 px-4 py-3 border-b border-gray-200 rounded-t-lg flex justify-between items-center">
                <h3 class="font-semibold text-gray-800">
                    <i class="fas fa-cube mr-2 text-blue-500"></i>${container.name}
                </h3>
                <span class="text-sm text-gray-500">${container.description}</span>
                ${container.is_list ? `
                    <button onclick="addListItem('${container.name}')" 
                            class="ml-4 px-3 py-1 bg-green-500 text-white text-xs rounded hover:bg-green-600">
                        <i class="fas fa-plus mr-1"></i>添加
                    </button>
                ` : ''}
            </div>
            <div class="p-4">
                ${container.is_list 
                    ? renderListContainer(container)
                    : renderParameterTable(container)
                }
            </div>
        </div>
    `).join('');
}

/**
 * 渲染参数表格
 */
function renderParameterTable(container) {
    return `
        <table class="w-full">
            <tbody>
                ${container.parameters.map(param => `
                    <tr class="parameter-row border-b border-gray-100 last:border-0">
                        <td class="py-3 px-4 w-1/3">
                            <label class="text-sm font-medium text-gray-700">${param.label || param.name}</label>
                        </td>
                        <td class="py-3 px-4">
                            ${renderParameterInput(param, container.name)}
                        </td>
                    </tr>
                `).join('')}
            </tbody>
        </table>
    `;
}

/**
 * 渲染列表型容器
 */
function renderListContainer(container) {
    const items = moduleConfig[container.name] || [];
    
    if (items.length === 0) {
        return `<p class="text-gray-400 text-center py-4">暂无配置项，请点击上方"添加"按钮</p>`;
    }
    
    return items.map((item, index) => `
        <div class="bg-gray-50 rounded p-4 mb-3 border border-gray-200">
            <div class="flex justify-between items-center mb-3">
                <span class="font-medium text-gray-700">#${index + 1}</span>
                <button onclick="removeListItem('${container.name}', ${index})" 
                        class="text-red-500 hover:text-red-700 text-sm">
                    <i class="fas fa-trash mr-1"></i>删除
                </button>
            </div>
            <table class="w-full">
                <tbody>
                    ${container.parameters.map(param => `
                        <tr class="border-b border-gray-200 last:border-0">
                            <td class="py-2 px-2 w-1/3 text-sm text-gray-600">${param.label || param.name}</td>
                            <td class="py-2 px-2">
                                ${renderListParameterInput(param, container.name, index)}
                            </td>
                        </tr>
                    `).join('')}
                </tbody>
            </table>
        </div>
    `).join('');
}

/**
 * 渲染参数输入控件
 */
function renderParameterInput(param, containerName) {
    const value = moduleConfig[containerName]?.[param.name] ?? param.default;
    const onchange = `updateConfig('${containerName}', '${param.name}', this.value, '${param.type}')`;
    
    switch (param.type) {
        case 'boolean':
            return `
                <label class="inline-flex items-center cursor-pointer">
                    <input type="checkbox" class="sr-only peer" 
                           ${value ? 'checked' : ''} 
                           onchange="updateConfig('${containerName}', '${param.name}', this.checked, 'boolean')">
                    <div class="relative w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-blue-600"></div>
                    <span class="ml-3 text-sm font-medium text-gray-900">${value ? '是' : '否'}</span>
                </label>
            `;
        
        case 'integer':
        case 'float':
            return `
                <input type="number" value="${value}" 
                       min="${param.min || ''}" max="${param.max || ''}"
                       step="${param.type === 'float' ? '0.1' : '1'}"
                       class="w-full px-3 py-2 border border-gray-300 rounded text-sm focus:outline-none focus:ring-2 focus:ring-blue-500"
                       onchange="${onchange}">
            `;
        
        case 'enum':
            return `
                <select class="w-full px-3 py-2 border border-gray-300 rounded text-sm focus:outline-none focus:ring-2 focus:ring-blue-500"
                        onchange="${onchange}">
                    ${param.options.map(opt => `
                        <option value="${opt}" ${opt === value ? 'selected' : ''}>${opt}</option>
                    `).join('')}
                </select>
            `;
        
        case 'string':
            return `
                <input type="text" value="${value}" 
                       class="w-full px-3 py-2 border border-gray-300 rounded text-sm focus:outline-none focus:ring-2 focus:ring-blue-500"
                       onchange="${onchange}">
            `;
        
        default:
            return `<input type="text" value="${value}" class="w-full px-3 py-2 border border-gray-300 rounded text-sm">`;
    }
}

/**
 * 渲染列表参数输入控件
 */
function renderListParameterInput(param, containerName, index) {
    const value = moduleConfig[containerName]?.[index]?.[param.name] ?? param.default;
    const onchange = `updateListConfig('${containerName}', ${index}, '${param.name}', this.value, '${param.type}')`;
    
    switch (param.type) {
        case 'boolean':
            return `
                <input type="checkbox" ${value ? 'checked' : ''} 
                       onchange="updateListConfig('${containerName}', ${index}, '${param.name}', this.checked, 'boolean')"
                       class="rounded text-blue-600 focus:ring-blue-500">
            `;
        
        case 'integer':
        case 'float':
            return `
                <input type="number" value="${value}" 
                       min="${param.min || ''}" max="${param.max || ''}"
                       step="${param.type === 'float' ? '0.1' : '1'}"
                       class="w-full px-2 py-1 border border-gray-300 rounded text-sm"
                       onchange="${onchange}">
            `;
        
        case 'enum':
            return `
                <select class="w-full px-2 py-1 border border-gray-300 rounded text-sm"
                        onchange="${onchange}">
                    ${param.options.map(opt => `
                        <option value="${opt}" ${opt === value ? 'selected' : ''}>${opt}</option>
                    `).join('')}
                </select>
            `;
        
        case 'string':
            return `
                <input type="text" value="${value}" 
                       class="w-full px-2 py-1 border border-gray-300 rounded text-sm"
                       onchange="${onchange}">
            `;
        
        default:
            return `<input type="text" value="${value}" class="w-full px-2 py-1 border border-gray-300 rounded text-sm">`;
    }
}

/**
 * 更新配置值
 */
function updateConfig(containerName, paramName, value, type) {
    if (!moduleConfig[containerName]) {
        moduleConfig[containerName] = {};
    }
    
    // 类型转换
    if (type === 'integer') {
        value = parseInt(value);
    } else if (type === 'float') {
        value = parseFloat(value);
    } else if (type === 'boolean') {
        value = value === true || value === 'true';
    }
    
    moduleConfig[containerName][paramName] = value;
    console.log('配置更新:', containerName, paramName, value);
}

/**
 * 更新列表配置值
 */
function updateListConfig(containerName, index, paramName, value, type) {
    if (!moduleConfig[containerName]) {
        moduleConfig[containerName] = [];
    }
    if (!moduleConfig[containerName][index]) {
        moduleConfig[containerName][index] = {};
    }
    
    // 类型转换
    if (type === 'integer') {
        value = parseInt(value);
    } else if (type === 'float') {
        value = parseFloat(value);
    } else if (type === 'boolean') {
        value = value === true || value === 'true';
    }
    
    moduleConfig[containerName][index][paramName] = value;
}

/**
 * 添加列表项
 */
function addListItem(containerName) {
    if (!moduleConfig[containerName]) {
        moduleConfig[containerName] = [];
    }
    
    const container = currentSchema.containers.find(c => c.name === containerName);
    const newItem = {};
    
    container.parameters.forEach(param => {
        newItem[param.name] = param.default;
    });
    
    moduleConfig[containerName].push(newItem);
    renderConfigForm();
}

/**
 * 移除列表项
 */
function removeListItem(containerName, index) {
    moduleConfig[containerName].splice(index, 1);
    renderConfigForm();
}

/**
 * 生成ARXML
 */
async function generateArxml() {
    if (!currentModule) {
        showToast('请先选择一个模块', 'warning');
        return;
    }
    
    const ecuName = document.getElementById('ecu-name').value || 'ECU0';
    
    try {
        const response = await fetch('/api/generate', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                module: currentModule,
                ecu: ecuName,
                config: moduleConfig
            })
        });
        
        const result = await response.json();
        
        if (result.success) {
            generatedArxml = result.arxml;
            // 高亮显示XML
            const highlighted = syntaxHighlight(generatedArxml);
            document.getElementById('arxml-preview').innerHTML = highlighted;
            showToast('ARXML生成成功！', 'success');
        } else {
            showToast('生成失败: ' + result.error, 'error');
        }
        
    } catch (error) {
        console.error('生成ARXML失败:', error);
        showToast('生成ARXML时出错', 'error');
    }
}

/**
 * XML语法高亮
 */
function syntaxHighlight(xml) {
    return xml
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/(&lt;\/?)([\w:-]+)/g, '<span style="color: #60a5fa;">$1$2</span>')
        .replace(/([\w:-]+)=/g, '<span style="color: #fbbf24;">$1</span>=')
        .replace(/"([^"]*)"/g, '<span style="color: #a3e635;">"$1"</span>')
        .replace(/(&gt;)([^&]*)/g, '$1<span style="color: #e5e7eb;">$2</span>');
}

/**
 * 复制到剪贴板
 */
function copyToClipboard() {
    if (!generatedArxml) {
        showToast('请先生成ARXML', 'warning');
        return;
    }
    
    navigator.clipboard.writeText(generatedArxml).then(() => {
        showToast('已复制到剪贴板', 'success');
    }).catch(() => {
        showToast('复制失败', 'error');
    });
}

/**
 * 下载ARXML文件
 */
async function downloadArxml() {
    if (!generatedArxml) {
        showToast('请先生成ARXML', 'warning');
        return;
    }
    
    const moduleName = currentModule || 'config';
    const filename = `${moduleName}.arxml`;
    
    try {
        const response = await fetch('/api/download', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                arxml: generatedArxml,
                filename: filename
            })
        });
        
        if (response.ok) {
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
            document.body.removeChild(a);
            showToast('下载成功！', 'success');
        } else {
            showToast('下载失败', 'error');
        }
        
    } catch (error) {
        console.error('下载失败:', error);
        showToast('下载时出错', 'error');
    }
}

/**
 * 重置配置
 */
function resetConfig() {
    if (currentSchema) {
        moduleConfig = {};
        currentSchema.containers.forEach(container => {
            if (container.is_list) {
                moduleConfig[container.name] = [];
            } else {
                moduleConfig[container.name] = {};
                container.parameters.forEach(param => {
                    moduleConfig[container.name][param.name] = param.default;
                });
            }
        });
        renderConfigForm();
        showToast('配置已重置', 'success');
    }
}

/**
 * 显示帮助
 */
function showHelp() {
    document.getElementById('help-modal').classList.remove('hidden');
}

/**
 * 隐藏帮助
 */
function hideHelp() {
    document.getElementById('help-modal').classList.add('hidden');
}

/**
 * 显示提示信息
 */
function showToast(message, type = 'info') {
    const toast = document.getElementById('toast');
    const toastMessage = document.getElementById('toast-message');
    
    const colors = {
        success: 'bg-green-600',
        error: 'bg-red-600',
        warning: 'bg-yellow-600',
        info: 'bg-blue-600'
    };
    
    toast.className = `fixed bottom-4 right-4 text-white px-6 py-3 rounded shadow-lg transform transition-transform duration-300 z-50 ${colors[type] || colors.info}`;
    toastMessage.textContent = message;
    
    // 显示
    toast.classList.remove('translate-y-20');
    
    // 3秒后隐藏
    setTimeout(() => {
        toast.classList.add('translate-y-20');
    }, 3000);
}
