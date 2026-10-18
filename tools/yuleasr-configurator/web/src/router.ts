import { createRouter, createWebHistory, createWebHashHistory } from 'vue-router'

const isElectron = typeof window !== 'undefined' && !!window.electronAPI?.isElectron

const router = createRouter({
  history: isElectron ? createWebHashHistory() : createWebHistory(),
  routes: [
    {
      path: '/',
      name: 'home',
      component: () => import('@/views/HomeView.vue'),
    },
    {
      path: '/config',
      name: 'config',
      component: () => import('@/views/ConfigEditorView.vue'),
    },
    {
      path: '/editor',
      name: 'editor',
      component: () => import('@/views/EditorView.vue'),
    },
    {
      path: '/graph',
      name: 'graph',
      component: () => import('@/views/DependencyGraphView.vue'),
    },
    {
      path: '/diff',
      name: 'diff',
      component: () => import('@/views/ConfigDiffView.vue'),
    },
    {
      path: '/arxml',
      name: 'arxml',
      component: () => import('@/views/ArxmlView.vue'),
    },
    {
      path: '/dbc-import',
      name: 'dbc-import',
      component: () => import('@/views/DbcImportView.vue'),
    },
    {
      path: '/new-project',
      name: 'new-project',
      component: () => import('@/views/ProjectWizardView.vue'),
    },
  ],
})

export default router
