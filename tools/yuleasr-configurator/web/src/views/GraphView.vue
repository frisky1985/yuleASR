<template>
  <div class="graph-view">
    <el-card>
      <template #header>
        <div class="card-header">
          <span>Module Dependency Graph</span>
          <el-space>
            <el-button @click="resetZoom">Reset Zoom</el-button>
            <el-button type="primary" @click="loadGraph">Refresh</el-button>
          </el-space>
        </div>
      </template>

      <div ref="graphContainer" class="graph-container" />

      <el-alert
        v-if="!hasData"
        title="Loading dependency data..."
        type="info"
        style="margin-bottom: 20px"
      />
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { getInitOrder } from '@/api/client'
import * as d3 from 'd3'
import type { InitOrderResult } from '@/types'

const graphContainer = ref<HTMLDivElement>()
const hasData = ref(false)
let svg: d3.Selection<SVGSVGElement, unknown, null, undefined> | null = null

const layerColors: Record<string, string> = {
  MCAL: '#409eff',
  ECUAL: '#67c23a',
  Services: '#e6a23c',
  OS: '#f56c6c',
  'BSW Coordination': '#909399',
}

function guessLayer(name: string): string {
  const mcal = ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Adc', 'Pwm', 'Gpt', 'Wdg', 'Icu', 'Lin', 'Eep', 'Fls']
  const ecual = ['CanIf', 'CanTp', 'IoHwAb', 'EthIf', 'MemIf', 'Fee', 'Ea', 'FrIf', 'LinIf']
  if (mcal.includes(name)) return 'MCAL'
  if (ecual.includes(name)) return 'ECUAL'
  if (['Com', 'PduR', 'NvM', 'Dcm', 'Dem'].includes(name)) return 'Services'
  if (name === 'Os') return 'OS'
  return 'BSW Coordination'
}

async function loadGraph() {
  if (!graphContainer.value) return

  try {
    const result: InitOrderResult = await getInitOrder()
    hasData.value = true
    renderGraph(result)
  } catch {
    hasData.value = false
  }
}

function renderGraph(result: InitOrderResult) {
  if (!graphContainer.value) return

  const container = graphContainer.value
  container.innerHTML = ''

  const width = container.clientWidth
  const height = 600

  svg = d3.select(container)
    .append('svg')
    .attr('width', width)
    .attr('height', height)

  const g = svg.append('g')

  const zoom = d3.zoom<SVGSVGElement, unknown>()
    .scaleExtent([0.1, 4])
    .on('zoom', (event) => g.attr('transform', event.transform))

  svg.call(zoom)

  const nodes = result.order.map((name) => ({
    id: name,
    layer: guessLayer(name),
  }))

  const links: Array<{ source: string; target: string }> = []
  for (const [node, deps] of Object.entries(result.dependencies)) {
    for (const dep of deps) {
      if (result.order.includes(dep)) {
        links.push({ source: dep, target: node })
      }
    }
  }

  const simulation = d3.forceSimulation(nodes as any)
    .force('link', d3.forceLink(links).id((d: any) => d.id).distance(80))
    .force('charge', d3.forceManyBody().strength(-200))
    .force('center', d3.forceCenter(width / 2, height / 2))

  const link = g.append('g')
    .selectAll('line')
    .data(links)
    .enter()
    .append('line')
    .attr('stroke', '#999')
    .attr('stroke-opacity', 0.6)
    .attr('stroke-width', 1)

  const node = g.append('g')
    .selectAll('g')
    .data(nodes)
    .enter()
    .append('g')
    .call(d3.drag<SVGGElement, any>()
      .on('start', (event, d: any) => {
        if (!event.active) simulation.alphaTarget(0.3).restart()
        d.fx = d.x
        d.fy = d.y
      })
      .on('drag', (event, d: any) => {
        d.fx = event.x
        d.fy = event.y
      })
      .on('end', (event, d: any) => {
        if (!event.active) simulation.alphaTarget(0)
        d.fx = null
        d.fy = null
      })
    )

  node.append('circle')
    .attr('r', 12)
    .attr('fill', (d: any) => layerColors[d.layer] || '#999')

  node.append('text')
    .text((d: any) => d.id)
    .attr('dx', 16)
    .attr('dy', 4)
    .attr('font-size', '10px')
    .attr('fill', '#333')

  simulation.on('tick', () => {
    link
      .attr('x1', (d: any) => d.source.x)
      .attr('y1', (d: any) => d.source.y)
      .attr('x2', (d: any) => d.target.x)
      .attr('y2', (d: any) => d.target.y)
    node.attr('transform', (d: any) => `translate(${d.x},${d.y})`)
  })
}

function resetZoom() {
  if (svg) {
    svg.transition().duration(750).call(
      d3.zoom<SVGSVGElement, unknown>().transform as any,
      d3.zoomIdentity
    )
  }
}

onMounted(() => {
  loadGraph()
})
</script>

<style scoped>
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
.graph-container {
  height: 600px;
  border: 1px solid #e4e7ed;
  border-radius: 4px;
  background: #fafafa;
}
</style>
