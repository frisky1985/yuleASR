/**
 * DDS Domain Topology Visualization
 * Interactive D3.js-based domain topology diagram
 */

class DomainTopology {
    constructor(containerId, width = 800, height = 600) {
        this.containerId = containerId;
        this.width = width;
        this.height = height;
        this.svg = null;
        this.simulation = null;
        this.nodes = [];
        this.links = [];
        this.zoom = null;
        this.onNodeSelect = null;
        this.onNodeDblClick = null;
    }

    init() {
        this.createSVG();
        this.setupSimulation();
        this.setupZoom();
        this.setupLegend();
        return this;
    }

    createSVG() {
        const container = d3.select(`#${this.containerId}`);
        container.html('');

        this.svg = container.append('svg')
            .attr('width', this.width)
            .attr('height', this.height)
            .attr('class', 'topology-svg');

        // Define arrow markers for directed links
        this.svg.append('defs').selectAll('marker')
            .data(['end', 'end-active', 'end-error'])
            .enter().append('marker')
            .attr('id', d => `arrow-${d}`)
            .attr('viewBox', '0 -5 10 10')
            .attr('refX', 25)
            .attr('refY', 0)
            .attr('markerWidth', 6)
            .attr('markerHeight', 6)
            .attr('orient', 'auto')
            .append('path')
            .attr('d', 'M0,-5L10,0L0,5')
            .attr('fill', d => {
                if (d === 'end-active') return '#28a745';
                if (d === 'end-error') return '#dc3545';
                return '#999';
            });

        // Grid pattern
        const gridPattern = this.svg.append('defs')
            .append('pattern')
            .attr('id', 'grid')
            .attr('width', 20)
            .attr('height', 20)
            .attr('patternUnits', 'userSpaceOnUse');

        gridPattern.append('path')
            .attr('d', 'M 20 0 L 0 0 0 20')
            .attr('fill', 'none')
            .attr('stroke', '#e0e0e0')
            .attr('stroke-width', 0.5);

        // Background grid
        this.svg.append('rect')
            .attr('width', this.width)
            .attr('height', this.height)
            .attr('fill', 'url(#grid)');

        // Main group for zoom
        this.g = this.svg.append('g');

        // Groups for layers
        this.linkLayer = this.g.append('g').attr('class', 'links');
        this.nodeLayer = this.g.append('g').attr('class', 'nodes');
        this.labelLayer = this.g.append('g').attr('class', 'labels');
    }

    setupSimulation() {
        this.simulation = d3.forceSimulation()
            .force('link', d3.forceLink().id(d => d.id).distance(150))
            .force('charge', d3.forceManyBody().strength(-500))
            .force('center', d3.forceCenter(this.width / 2, this.height / 2))
            .force('collision', d3.forceCollide().radius(50));
    }

    setupZoom() {
        this.zoom = d3.zoom()
            .scaleExtent([0.1, 4])
            .on('zoom', (event) => {
                this.g.attr('transform', event.transform);
            });

        this.svg.call(this.zoom);

        // Zoom controls
        const controls = d3.select(`#${this.containerId}`)
            .append('div')
            .attr('class', 'topology-controls');

        controls.append('button')
            .attr('class', 'btn btn-sm btn-light')
            .html('<i class="fas fa-plus"></i>')
            .on('click', () => this.zoomBy(1.2));

        controls.append('button')
            .attr('class', 'btn btn-sm btn-light')
            .html('<i class="fas fa-minus"></i>')
            .on('click', () => this.zoomBy(0.8));

        controls.append('button')
            .attr('class', 'btn btn-sm btn-light')
            .html('<i class="fas fa-compress-arrows-alt"></i>')
            .on('click', () => this.fitToScreen());
    }

    setupLegend() {
        const legend = d3.select(`#${this.containerId}`)
            .append('div')
            .attr('class', 'topology-legend');

        const items = [
            { type: 'participant', label: 'Participant', color: '#3498db' },
            { type: 'publisher', label: 'Publisher', color: '#e74c3c' },
            { type: 'subscriber', label: 'Subscriber', color: '#2ecc71' },
            { type: 'topic', label: 'Topic', color: '#9b59b6' },
            { type: 'gateway', label: 'Gateway', color: '#f39c12' }
        ];

        items.forEach(item => {
            const row = legend.append('div').attr('class', 'legend-item');
            row.append('span')
                .attr('class', 'legend-color')
                .style('background-color', item.color);
            row.append('span')
                .attr('class', 'legend-label')
                .text(item.label);
        });
    }

    setData(nodes, links) {
        this.nodes = nodes.map(n => ({...n}));
        this.links = links.map(l => ({...l}));
        this.render();
    }

    render() {
        // Update links
        const link = this.linkLayer.selectAll('.link')
            .data(this.links, d => `${d.source.id || d.source}-${d.target.id || d.target}`);

        link.exit().remove();

        const linkEnter = link.enter().append('g')
            .attr('class', 'link');

        linkEnter.append('line')
            .attr('class', 'link-line')
            .attr('marker-end', d => {
                if (d.status === 'active') return 'url(#arrow-end-active)';
                if (d.status === 'error') return 'url(#arrow-end-error)';
                return 'url(#arrow-end)';
            });

        linkEnter.append('text')
            .attr('class', 'link-label')
            .attr('dy', -5)
            .attr('text-anchor', 'middle')
            .text(d => d.topic || '');

        const linkUpdate = linkEnter.merge(link);

        linkUpdate.select('.link-line')
            .attr('stroke', d => {
                if (d.status === 'active') return '#28a745';
                if (d.status === 'error') return '#dc3545';
                return '#999';
            })
            .attr('stroke-width', d => d.width || 2)
            .attr('stroke-dasharray', d => d.dashed ? '5,5' : null);

        // Update nodes
        const node = this.nodeLayer.selectAll('.node')
            .data(this.nodes, d => d.id);

        node.exit().transition().duration(300).attr('opacity', 0).remove();

        const nodeEnter = node.enter().append('g')
            .attr('class', 'node')
            .attr('cursor', 'pointer')
            .call(d3.drag()
                .on('start', (event, d) => this.dragstarted(event, d))
                .on('drag', (event, d) => this.dragged(event, d))
                .on('end', (event, d) => this.dragended(event, d)));

        // Node shapes based on type
        nodeEnter.each(function(d) {
            const el = d3.select(this);
            const color = d.color || DomainTopology.getNodeColor(d.type);

            if (d.type === 'topic') {
                el.append('rect')
                    .attr('width', 40)
                    .attr('height', 30)
                    .attr('x', -20)
                    .attr('y', -15)
                    .attr('rx', 5)
                    .attr('fill', color)
                    .attr('stroke', '#333')
                    .attr('stroke-width', 2);
            } else if (d.type === 'gateway') {
                el.append('polygon')
                    .attr('points', '0,-25 22,-12 22,12 0,25 -22,12 -22,-12')
                    .attr('fill', color)
                    .attr('stroke', '#333')
                    .attr('stroke-width', 2);
            } else {
                el.append('circle')
                    .attr('r', 25)
                    .attr('fill', color)
                    .attr('stroke', '#333')
                    .attr('stroke-width', 2);
            }

            // Add icon
            if (d.icon) {
                el.append('text')
                    .attr('class', 'node-icon')
                    .attr('text-anchor', 'middle')
                    .attr('dy', 5)
                    .attr('font-family', 'FontAwesome')
                    .attr('font-size', '16px')
                    .attr('fill', 'white')
                    .text(d.icon);
            }

            // Status indicator
            if (d.status) {
                el.append('circle')
                    .attr('class', 'status-indicator')
                    .attr('r', 6)
                    .attr('cx', 18)
                    .attr('cy', -18)
                    .attr('fill', d.status === 'online' ? '#2ecc71' : '#e74c3c')
                    .attr('stroke', 'white')
                    .attr('stroke-width', 2);
            }
        });

        const nodeUpdate = nodeEnter.merge(node);

        nodeUpdate.on('click', (event, d) => {
            event.stopPropagation();
            this.selectNode(d);
        });

        nodeUpdate.on('dblclick', (event, d) => {
            event.stopPropagation();
            if (this.onNodeDblClick) {
                this.onNodeDblClick(d);
            }
        });

        // Update labels
        const label = this.labelLayer.selectAll('.node-label')
            .data(this.nodes, d => d.id);

        label.exit().remove();

        const labelEnter = label.enter().append('text')
            .attr('class', 'node-label')
            .attr('text-anchor', 'middle')
            .attr('dy', 40)
            .attr('font-size', '12px')
            .attr('fill', '#333')
            .text(d => d.name);

        // Restart simulation
        this.simulation.nodes(this.nodes).on('tick', () => this.ticked());
        this.simulation.force('link').links(this.links);
        this.simulation.alpha(1).restart();
    }

    ticked() {
        this.linkLayer.selectAll('.link line')
            .attr('x1', d => d.source.x)
            .attr('y1', d => d.source.y)
            .attr('x2', d => d.target.x)
            .attr('y2', d => d.target.y);

        this.linkLayer.selectAll('.link text')
            .attr('x', d => (d.source.x + d.target.x) / 2)
            .attr('y', d => (d.source.y + d.target.y) / 2);

        this.nodeLayer.selectAll('.node')
            .attr('transform', d => `translate(${d.x},${d.y})`);

        this.labelLayer.selectAll('.node-label')
            .attr('x', d => d.x)
            .attr('y', d => d.y);
    }

    dragstarted(event, d) {
        if (!event.active) this.simulation.alphaTarget(0.3).restart();
        d.fx = d.x;
        d.fy = d.y;
    }

    dragged(event, d) {
        d.fx = event.x;
        d.fy = event.y;
    }

    dragended(event, d) {
        if (!event.active) this.simulation.alphaTarget(0);
        d.fx = null;
        d.fy = null;
    }

    selectNode(node) {
        this.nodeLayer.selectAll('.node').classed('selected', false);
        this.nodeLayer.selectAll('.node')
            .filter(d => d.id === node.id)
            .classed('selected', true);

        if (this.onNodeSelect) {
            this.onNodeSelect(node);
        }
    }

    zoomBy(scale) {
        this.svg.transition().duration(300).call(
            this.zoom.transform,
            d3.zoomTransform(this.svg.node()).scale(scale)
        );
    }

    fitToScreen() {
        if (this.nodes.length === 0) return;

        const bounds = this.g.node().getBBox();
        const parent = this.svg.node().parentElement;
        const fullWidth = parent.clientWidth;
        const fullHeight = parent.clientHeight;

        const width = bounds.width;
        const height = bounds.height;
        const midX = bounds.x + width / 2;
        const midY = bounds.y + height / 2;

        const scale = 0.8 / Math.max(width / fullWidth, height / fullHeight);
        const translate = [fullWidth / 2 - scale * midX, fullHeight / 2 - scale * midY];

        this.svg.transition().duration(750).call(
            this.zoom.transform,
            d3.zoomIdentity.translate(translate[0], translate[1]).scale(scale)
        );
    }

    highlightPath(nodeIds) {
        const nodeSet = new Set(nodeIds);

        this.nodeLayer.selectAll('.node')
            .transition().duration(200)
            .style('opacity', d => nodeSet.has(d.id) ? 1 : 0.3);

        this.linkLayer.selectAll('.link')
            .transition().duration(200)
            .style('opacity', d => 
                nodeSet.has(d.source.id) && nodeSet.has(d.target.id) ? 1 : 0.1
            );
    }

    clearHighlight() {
        this.nodeLayer.selectAll('.node')
            .transition().duration(200)
            .style('opacity', 1);

        this.linkLayer.selectAll('.link')
            .transition().duration(200)
            .style('opacity', 1);
    }

    static getNodeColor(type) {
        const colors = {
            participant: '#3498db',
            publisher: '#e74c3c',
            subscriber: '#2ecc71',
            topic: '#9b59b6',
            gateway: '#f39c12',
            service: '#1abc9c',
            default: '#95a5a6'
        };
        return colors[type] || colors.default;
    }

    destroy() {
        if (this.simulation) {
            this.simulation.stop();
        }
        if (this.svg) {
            this.svg.remove();
        }
    }
}

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
    module.exports = DomainTopology;
}