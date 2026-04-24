/**
 * DDS Visualizer - Chart Utilities
 * Advanced chart configurations and helper functions
 */

// Chart.js defaults
Chart.defaults.font.family = "'Segoe UI', system-ui, sans-serif";
Chart.defaults.color = '#64748b';
Chart.defaults.scale.grid.color = '#e2e8f0';

// Color palette
const colors = {
    primary: '#2563eb',
    primaryLight: 'rgba(37, 99, 235, 0.1)',
    success: '#10b981',
    successLight: 'rgba(16, 185, 129, 0.1)',
    warning: '#f59e0b',
    warningLight: 'rgba(245, 158, 11, 0.1)',
    danger: '#ef4444',
    dangerLight: 'rgba(239, 68, 68, 0.1)',
    info: '#06b6d4',
    infoLight: 'rgba(6, 182, 212, 0.1)'
};

/**
 * Create a real-time line chart
 */
function createRealtimeChart(ctx, options = {}) {
    const config = {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: options.label || 'Value',
                data: [],
                borderColor: options.color || colors.primary,
                backgroundColor: options.fillColor || colors.primaryLight,
                fill: true,
                tension: 0.4,
                pointRadius: 0,
                pointHoverRadius: 6
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: {
                duration: 0
            },
            interaction: {
                intersect: false,
                mode: 'index'
            },
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                },
                tooltip: {
                    mode: 'index',
                    intersect: false,
                    backgroundColor: 'rgba(30, 41, 59, 0.9)',
                    titleFont: { size: 13 },
                    bodyFont: { size: 12 },
                    padding: 10,
                    cornerRadius: 8,
                    displayColors: true
                }
            },
            scales: {
                x: {
                    grid: {
                        display: false
                    },
                    ticks: {
                        maxTicksLimit: 8,
                        maxRotation: 0
                    }
                },
                y: {
                    beginAtZero: true,
                    grid: {
                        borderDash: [2, 4]
                    }
                }
            },
            ...options.chartOptions
        }
    };
    
    return new Chart(ctx, config);
}

/**
 * Create a gauge chart for single metrics
 */
function createGaugeChart(ctx, options = {}) {
    const config = {
        type: 'doughnut',
        data: {
            labels: ['Value', 'Remaining'],
            datasets: [{
                data: [0, 100],
                backgroundColor: [
                    options.color || colors.primary,
                    '#e2e8f0'
                ],
                borderWidth: 0,
                cutout: '85%'
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            rotation: -90,
            circumference: 180,
            plugins: {
                legend: { display: false },
                tooltip: { enabled: false }
            }
        },
        plugins: [{
            id: 'gaugeText',
            beforeDraw: function(chart) {
                const width = chart.width,
                      height = chart.height,
                      ctx = chart.ctx;
                
                ctx.restore();
                const fontSize = (height / 114).toFixed(2);
                ctx.font = `bold ${fontSize}em sans-serif`;
                ctx.textBaseline = 'middle';
                ctx.fillStyle = '#1e293b';
                
                const text = options.value || '0',
                      textX = Math.round((width - ctx.measureText(text).width) / 2),
                      textY = height / 2 + 10;
                
                ctx.fillText(text, textX, textY);
                
                if (options.unit) {
                    ctx.font = `${(height / 200).toFixed(2)}em sans-serif`;
                    ctx.fillStyle = '#64748b';
                    const unitX = Math.round((width - ctx.measureText(options.unit).width) / 2),
                          unitY = height / 2 + 35;
                    ctx.fillText(options.unit, unitX, unitY);
                }
                
                ctx.save();
            }
        }]
    };
    
    return new Chart(ctx, config);
}

/**
 * Create a bar chart for distribution
 */
function createDistributionChart(ctx, options = {}) {
    const config = {
        type: 'bar',
        data: {
            labels: options.labels || [],
            datasets: [{
                label: options.label || 'Count',
                data: options.data || [],
                backgroundColor: options.colors || colors.primary,
                borderRadius: 4,
                borderSkipped: false
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: { display: false }
            },
            scales: {
                x: {
                    grid: { display: false }
                },
                y: {
                    beginAtZero: true,
                    grid: { borderDash: [2, 4] }
                }
            }
        }
    };
    
    return new Chart(ctx, config);
}

/**
 * Update a chart with new data point
 */
function updateChartRealtime(chart, label, value, maxPoints = 50) {
    chart.data.labels.push(label);
    chart.data.datasets[0].data.push(value);
    
    // Remove old data points
    if (chart.data.labels.length > maxPoints) {
        chart.data.labels.shift();
        chart.data.datasets[0].data.shift();
    }
    
    chart.update('none');
}

/**
 * Update gauge chart
 */
function updateGaugeChart(chart, value, max = 100) {
    const percentage = Math.min((value / max) * 100, 100);
    chart.data.datasets[0].data = [percentage, 100 - percentage];
    
    // Update color based on value
    if (percentage > 80) {
        chart.data.datasets[0].backgroundColor[0] = colors.danger;
    } else if (percentage > 60) {
        chart.data.datasets[0].backgroundColor[0] = colors.warning;
    } else {
        chart.data.datasets[0].backgroundColor[0] = colors.success;
    }
    
    chart.update();
}

/**
 * Create a radar chart for multi-metric comparison
 */
function createRadarChart(ctx, options = {}) {
    const config = {
        type: 'radar',
        data: {
            labels: options.labels || ['Metric 1', 'Metric 2', 'Metric 3', 'Metric 4', 'Metric 5'],
            datasets: options.datasets || [{
                label: 'Current',
                data: [0, 0, 0, 0, 0],
                backgroundColor: colors.primaryLight,
                borderColor: colors.primary,
                pointBackgroundColor: colors.primary,
                pointBorderColor: '#fff',
                pointHoverBackgroundColor: '#fff',
                pointHoverBorderColor: colors.primary
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                r: {
                    beginAtZero: true,
                    max: options.max || 100,
                    ticks: {
                        display: false
                    },
                    grid: {
                        color: '#e2e8f0'
                    },
                    angleLines: {
                        color: '#e2e8f0'
                    }
                }
            },
            plugins: {
                legend: {
                    position: 'bottom'
                }
            }
        }
    };
    
    return new Chart(ctx, config);
}

/**
 * Create a heatmap-style chart
 */
function createHeatmapChart(containerId, data, options = {}) {
    // Using a simple grid-based heatmap with divs
    const container = document.getElementById(containerId);
    if (!container) return;
    
    container.innerHTML = '';
    container.style.display = 'grid';
    container.style.gridTemplateColumns = `repeat(${options.cols || 10}, 1fr)`;
    container.style.gap = '2px';
    
    data.forEach((value, index) => {
        const cell = document.createElement('div');
        cell.style.aspectRatio = '1';
        cell.style.borderRadius = '2px';
        cell.style.backgroundColor = getHeatmapColor(value, options.min || 0, options.max || 100);
        cell.title = `${options.label || 'Value'}: ${value.toFixed(2)}`;
        container.appendChild(cell);
    });
}

/**
 * Get color for heatmap value
 */
function getHeatmapColor(value, min, max) {
    const ratio = (value - min) / (max - min);
    
    if (ratio < 0.25) return colors.success;
    if (ratio < 0.5) return colors.info;
    if (ratio < 0.75) return colors.warning;
    return colors.danger;
}

/**
 * Export chart as image
 */
function exportChart(chart, filename = 'chart.png') {
    const url = chart.toBase64Image();
    const link = document.createElement('a');
    link.download = filename;
    link.href = url;
    link.click();
}

// Export functions for use in other modules
window.ChartUtils = {
    colors,
    createRealtimeChart,
    createGaugeChart,
    createDistributionChart,
    createRadarChart,
    updateChartRealtime,
    updateGaugeChart,
    createHeatmapChart,
    exportChart
};
