import { contextBridge, ipcRenderer } from 'electron'

export interface ElectronAPI {
  readFile: (filePath: string) => Promise<{ success: boolean; content?: string; error?: string }>
  writeFile: (filePath: string, content: string) => Promise<{ success: boolean; error?: string }>
  showOpenDialog: (options: any) => Promise<any>
  showSaveDialog: (options: any) => Promise<any>
  getAppInfo: () => Promise<{
    version: string
    platform: string
    electron: string
    chrome: string
    node: string
  }>
  onMenuAction: (callback: (action: string) => void) => void
  onFileOpened: (callback: (data: { path: string; content: string }) => void) => void
  onDbcOpened: (callback: (data: { path: string; content: string }) => void) => void
  onArxmlImported: (callback: (data: { path: string; content: string }) => void) => void
  onSaveAs: (callback: (data: { path: string }) => void) => void
  isElectron: true
}

const api: ElectronAPI = {
  readFile: (filePath) => ipcRenderer.invoke('read-file', filePath),
  writeFile: (filePath, content) => ipcRenderer.invoke('write-file', filePath, content),
  showOpenDialog: (options) => ipcRenderer.invoke('show-open-dialog', options),
  showSaveDialog: (options) => ipcRenderer.invoke('show-save-dialog', options),
  getAppInfo: () => ipcRenderer.invoke('get-app-info'),
  onMenuAction: (callback) => {
    ipcRenderer.on('menu-action', (_event, action) => callback(action))
  },
  onFileOpened: (callback) => {
    ipcRenderer.on('file-opened', (_event, data) => callback(data))
  },
  onDbcOpened: (callback) => {
    ipcRenderer.on('dbc-opened', (_event, data) => callback(data))
  },
  onArxmlImported: (callback) => {
    ipcRenderer.on('arxml-imported', (_event, data) => callback(data))
  },
  onSaveAs: (callback) => {
    ipcRenderer.on('save-as', (_event, data) => callback(data))
  },
  isElectron: true,
}

contextBridge.exposeInMainWorld('electronAPI', api)
