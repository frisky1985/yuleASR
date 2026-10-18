<template>
  <div class="dependency-graph">
    <div class="graph-toolbar">
      <el-select v-model="selectedLayer" size="small" placeholder="Filter by layer" clearable style="width: 150px">
        <el-option label="All Layers" value="" />
        <el-option label="MCAL" value="MCAL" />
        <el-option label="ECUAL" value="ECUAL" />
        <el-option label="Services" value="Services" />
        <el-option label="OS" value="OS" />
        <el-option label="RTE" value="RTE" />
        <el-option label="Integration" value="Integration" />
      </el-select>

      <el-checkbox v-model="showLabels" size="small">Show Labels</el-checkbox>
      <el-checkbox v-model="highlightCycles" size="small">Highlight Cycles</el-checkbox>

      <div class="toolbar-spacer" />

      <el-button size="small" @click="resetZoom">
        <el-icon><Refresh /></el-icon>
        Reset View
      </el-button>

      <el-button size="small" @click="exportGraph">
        <el-icon><Download /></el-icon>
        Export PNG
      </el-button>
    </div>

    <div class="graph-info" v-if="cycleCount > 0">
      <el-alert
        :title="`Detected ${cycleCount} circular dependencies`"
        type="warning"
        show-icon
        :closable="false"
      />
    </div>

    <div ref="graphContainer" class="graph-container">
      <svg ref="svgElement" width="100%" height="100%">
        <g ref="graphGroup"></g>
      </svg>
    </div>

    <div class="graph-legend">
      <div class="legend-item">
        <div class="legend-color" style="background: #409eff"></div>
        <span>MCAL</span>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background: #67c23a"></div>
        <span>ECUAL</span>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background: #e6a23c"></div>
        <span>Services</span>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background: #f56c6c"></div>
        <span>OS</span>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background: #909399"></div>
        <span>RTE</span>
      </div>
      <div class="legend-item">
        <div class="legend-color" style="background: #b37feb"></div>
        <span>Integration</span>
      </div>
      <div class="legend-item">
        <div class="legend-color legend-cycle" style="background: #f56c6c"></div>
        <span>Circular Dependency</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, watch } from 'vue'
import { Refresh, Download } from '@element-plus/icons-vue'
import * as d3 from 'd3'
import { useConfigStore } from '@/stores/config'

interface GraphNode {
  id: string
  layer: string
  enabled: boolean
  x?: number
  y?: number
  fx?: number | null
  fy?: number | null
}

interface GraphLink {
  source: string
  target: string
  isCycle?: boolean
}

const store = useConfigStore()

const graphContainer = ref<HTMLElement>()
const svgElement = ref<SVGSVGElement>()
const graphGroup = ref<SVGGElement>()
const selectedLayer = ref('')
const showLabels = ref(true)
const highlightCycles = ref(true)

const nodes = ref<GraphNode[]>([])
const links = ref<GraphLink[]>([])
const cycleCount = ref(0)

const LAYER_COLORS: Record<string, string> = {
  MCAL: '#409eff',
  ECUAL: '#67c23a',
  Services: '#e6a23c',
  OS: '#f56c6c',
  RTE: '#909399',
  Integration: '#b37feb',
}

const DEPENDENCIES: Record<string, string[]> = {
  Mcu: [],
  Port: ['Mcu'],
  Dio: ['Port'],
  Can: ['Mcu', 'Port'],
  Spi: ['Mcu', 'Port'],
  Adc: ['Mcu'],
  Pwm: ['Mcu'],
  Gpt: ['Mcu'],
  Wdg: ['Mcu'],
  Icu: ['Mcu'],
  Lin: ['Mcu', 'Port'],
  Eep: ['Spi'],
  Fls: ['Spi'],
  CanIf: ['Can'],
  CanTp: ['CanIf'],
  EthIf: ['Eth'],
  MemIf: [],
  Fee: ['MemIf'],
  Ea: ['MemIf'],
  LinIf: ['Lin'],
  PduR: ['CanIf', 'Com'],
  Com: ['PduR'],
  NvM: ['Fee', 'Ea'],
  Dcm: ['Com'],
  Dem: ['NvM'],
  Os: [],
  Rte: ['Os'],
  BswM: ['EcuM'],
  EcuM: ['Mcu'],
}

function guessLayer(name: string): string {
  const mcal = ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Adc', 'Pwm', 'Gpt', 'Wdg', 'Icu', 'Lin', 'Eep', 'Fls']
  const ecual = ['CanIf', 'CanTp', 'EthIf', 'MemIf', 'Fee', 'Ea', 'LinIf', 'PduR']
  if (mcal.includes(name)) return 'MCAL'
  if (ecual.includes(name)) return 'ECUAL'
  if (name === 'Os') return 'OS'
  if (name === 'Rte') return 'RTE'
  if (['BswM', 'EcuM'].includes(name)) return 'Integration'
  return 'Services'
}

function detectCycles(): GraphLink[] {
  const cycleLinks: GraphLink[] = []
  const visited = new Set<string>()
  const recursionStack = new Set<string>()

  function dfs(node: string, path: string[]): boolean {
    if (recursionStack.has(node)) {
      const cycleStart = path.indexOf(node)
      if (cycleStart >= 0) {
        for (let i = cycleStart; i < path.length - 1; i++) {
          cycleLinks.push({ source: path[i], target: path[i + 1], isCycle: true })
        }
      }
      return true
    }

    if (visited.has(node)) return false

    visited.add(node)
    recursionStack.add(node)
    path.push(node)

    const deps = DEPENDENCIES[node] || []
    for (const dep of deps) {
      if (dfs(dep, [...path])) {
        cycleLinks.push({ source: node, target: dep, isCycle: true })
      }
    }

    recursionStack.delete(node)
    return false
  }

  for (const node of Object.keys(DEPENDENCIES)) {
    dfs(node, [])
  }

  return cycleLinks
}

function buildGraph() {
  const modules = store.modules
  const nodeMap = new Map<string, GraphNode>()

  for (const [name, mod] of Object.entries(modules)) {
    const layer = guessLayer(name)
    if (!selectedLayer.value || layer === selectedLayer.value) {
      nodeMap.set(name, {
        id: name,
        layer,
        enabled: mod.enabled,
      })
    }
  }

  const graphLinks: GraphLink[] = []
  for (const [name, deps] of Object.entries(DEPENDENCIES)) {
    if (!nodeMap.has(name)) continue

    for (const dep of deps) {
      if (nodeMap.has(dep)) {
        graphLinks.push({ source: name, target: dep })
      }
    }
  }

  const cycles = highlightCycles.value ? detectCycles() : []
  cycleCount.value = new Set(cycles.map((c) => `${c.source}-${c.target}`)).size

  for (const cycle of cycles) {
    const existing = graphLinks.find(
      (l) => l.source === cycle.source && l.target === cycle.target,
    )
    if (existing) {
      existing.isCycle = true
    }
  }

  nodes.value = Array.from(nodeMap.values())
  links.value = graphLinks
}

function renderGraph() {
  if (!graphContainer.value || !svgElement.value || !graphGroup.value) return

  const width = graphContainer.value.clientWidth
  const height = graphContainer.value.clientHeight

  const svg = d3.select(svgElement.value)
  const g = d3.select(graphGroup.value)

  svg.attr('viewBox', `0 0 ${width} ${height}`)

  const simulation = d3
    .forceSimulation(nodes.value as any)
    .force(
      'link',
      d3.forceLink(links.value as any)
        .id((d: any) => d.id)
        .distance(100),
    )
    .force('charge', d3.forceManyBody().strength(-300))
    .force('center', d3.forceCenter(width / 2, height / 2))
    .force('collision', d3.forceCollide().radius(30))

  g.selectAll('*').remove()

  const link = g
    .append('g')
    .selectAll('line')
    .data(links.value)
    .join('line')
    .attr('stroke', (d) => (d.isCycle ? '#f56c6c' : '#999'))
    .attr('stroke-width', (d) => (d.isCycle ? 3 : 1.5))
    .attr('stroke-opacity', 0.6)
    .attr('stroke-dasharray', (d) => (d.isCycle ? '5,5' : 'none'))

  const node = g
    .append('g')
    .selectAll('g')
    .data(nodes.value)
    .join('g')
    .call(
      d3.drag<SVGGElement, GraphNode>()
        .on('start', (event, d) => {
          if (!event.active) simulation.alphaTarget(0.3).restart()
          d.fx = d.x
          d.fy = d.y
        })
        .on('drag', (event, d) => {
          d.fx = event.x
          d.fy = event.y
        })
        .on('end', (event, d) => {
          if (!event.active) simulation.alphaTarget(0)
          d.fx = null
          d.fy = null
        }) as any,
    )

  node
    .append('circle')
    .attr('r', 20)
    .attr('fill', (d) => LAYER_COLORS[d.layer] || '#999')
    .attr('stroke', (d) => (d.enabled ? '#fff' : '#ccc'))
    .attr('stroke-width', (d) => (d.enabled ? 3 : 1))
    .attr('opacity', (d) => (d.enabled ? 1 : 0.5))

  if (showLabels.value) {
    node
      .append('text')
      .text((d) => d.id)
      .attr('x', 0)
      .attr('y', 30)
      .attr('text-anchor', 'middle')
      .attr('font-size', '11px')
      .attr('fill', '#303133')
  }

  simulation.on('tick', () => {
    link
      .attr('x1', (d: any) => d.source.x)
      .attr('y1', (d: any) => d.source.y)
      .attr('x2', (d: any) => d.target.x)
      .attr('y2', (d: any) => d.target.y)

    node.attr('transform', (d) => `translate(${d.x},${d.y})`)
  })
}

function resetZoom() {
  if (!svgElement.value || !graphGroup.value) return

  const svg = d3.select(svgElement.value)
  const g = d3.select(graphGroup.value)

  svg.transition().duration(750).call(
    d3.zoom<SVGSVGElement, unknown>()
      .scaleExtent([0.1, 4])
      .on('zoom', (event) => {
        g.attr('transform', event.transform)
      }) as any,
    d3.zoomIdentity,
  )
}

function exportGraph() {
  if (!svgElement.value) return

  const svgData = new XMLSerializer().serializeToString(svgElement.value)
  const canvas = document.createElement('canvas')
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  const img = new Image()
  img.onload = () => {
    canvas.width = img.width
    canvas.height = img.height
    ctx.drawImage(img, 0, 0)

    const link = document.createElement('a')
    link.download = `dependency-graph-${Date.now()}.png`
    link.href = canvas.toDataURL('image/png')
    link.click()
  }

  img.src = 'data:image/svg+xml;base64,' + btoa(unescape(encodeURIComponent(svgData)))
}

watch([selectedLayer, showLabels, highlightCycles], () => {
  buildGraph()
  renderGraph()
})

onMounted(() => {
  buildGraph()
  renderGraph()
})
</script>

<style scoped>
.dependency-graph {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #fff;
}

.graph-toolbar {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px;
  border-bottom: 1px solid #e4e7ed;
  background: #fafafa;
}

.toolbar-spacer {
  flex: 1;
}

.graph-info {
  padding: 10px;
  border-bottom: 1px solid #e4e7ed;
}

.graph-container {
  flex: 1;
  position: relative;
  overflow: hidden;
}

.graph-container svg {
  width: 100%;
  height: 100%;
}

.graph-legend {
  display: flex;
  gap: 16px;
  padding: 10px;
  border-top: 1px solid #e4e7ed;
  background: #fafafa;
  flex-wrap: wrap;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
}

.legend-color {
  width: 16px;
  height: 16px;
  border-radius: 50%;
  border: 2px solid #fff;
  box-shadow: 0 0 0 1px #ddd;
}

.legend-cycle {
  border-style: dashed;
}
</style>
