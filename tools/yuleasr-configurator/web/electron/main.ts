import { app, BrowserWindow, Menu, ipcMain, dialog, shell } from 'electron'
import path from 'path'
import fs from 'fs'

const isDev = process.env.NODE_ENV === 'development'
const VITE_DEV_SERVER_URL = process.env.VITE_DEV_SERVER_URL || 'http://localhost:3000'

let mainWindow: BrowserWindow | null = null

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    minWidth: 1024,
    minHeight: 700,
    title: 'yuleASR Configurator',
    icon: path.join(__dirname, '../public/favicon.ico'),
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false,
    },
    titleBarStyle: 'hiddenInset',
    trafficLightPosition: { x: 12, y: 16 },
    show: false,
  })

  if (isDev && VITE_DEV_SERVER_URL) {
    mainWindow.loadURL(VITE_DEV_SERVER_URL)
    mainWindow.webContents.openDevTools()
  } else {
    mainWindow.loadFile(path.join(__dirname, '../dist/index.html'))
  }

  mainWindow.once('ready-to-show', () => {
    mainWindow?.show()
  })

  mainWindow.on('closed', () => {
    mainWindow = null
  })
}

function createMenu() {
  const template: Electron.MenuItemConstructorOptions[] = [
    {
      label: 'File',
      submenu: [
        {
          label: 'New Project',
          accelerator: 'CmdOrCtrl+N',
          click: () => mainWindow?.webContents.send('menu-action', 'new-project'),
        },
        {
          label: 'Open Configuration...',
          accelerator: 'CmdOrCtrl+O',
          click: async () => {
            const result = await dialog.showOpenDialog(mainWindow!, {
              properties: ['openFile'],
              filters: [
                { name: 'Configuration Files', extensions: ['yaml', 'yml', 'json'] },
                { name: 'All Files', extensions: ['*'] },
              ],
            })
            if (!result.canceled && result.filePaths.length > 0) {
              const filePath = result.filePaths[0]
              const content = fs.readFileSync(filePath, 'utf-8')
              mainWindow?.webContents.send('file-opened', { path: filePath, content })
            }
          },
        },
        {
          label: 'Open DBC File...',
          accelerator: 'CmdOrCtrl+Shift+O',
          click: async () => {
            const result = await dialog.showOpenDialog(mainWindow!, {
              properties: ['openFile'],
              filters: [
                { name: 'DBC Files', extensions: ['dbc'] },
                { name: 'All Files', extensions: ['*'] },
              ],
            })
            if (!result.canceled && result.filePaths.length > 0) {
              const filePath = result.filePaths[0]
              const content = fs.readFileSync(filePath, 'utf-8')
              mainWindow?.webContents.send('dbc-opened', { path: filePath, content })
            }
          },
        },
        { type: 'separator' },
        {
          label: 'Save',
          accelerator: 'CmdOrCtrl+S',
          click: () => mainWindow?.webContents.send('menu-action', 'save'),
        },
        {
          label: 'Save As...',
          accelerator: 'CmdOrCtrl+Shift+S',
          click: async () => {
            const result = await dialog.showSaveDialog(mainWindow!, {
              filters: [
                { name: 'YAML Files', extensions: ['yaml', 'yml'] },
                { name: 'JSON Files', extensions: ['json'] },
              ],
            })
            if (!result.canceled && result.filePath) {
              mainWindow?.webContents.send('save-as', { path: result.filePath })
            }
          },
        },
        { type: 'separator' },
        {
          label: 'Import ARXML...',
          click: async () => {
            const result = await dialog.showOpenDialog(mainWindow!, {
              properties: ['openFile'],
              filters: [
                { name: 'ARXML Files', extensions: ['arxml'] },
                { name: 'All Files', extensions: ['*'] },
              ],
            })
            if (!result.canceled && result.filePaths.length > 0) {
              const filePath = result.filePaths[0]
              const content = fs.readFileSync(filePath, 'utf-8')
              mainWindow?.webContents.send('arxml-imported', { path: filePath, content })
            }
          },
        },
        {
          label: 'Export ARXML...',
          click: () => mainWindow?.webContents.send('menu-action', 'export-arxml'),
        },
        { type: 'separator' },
        {
          label: 'Generate Code',
          accelerator: 'F5',
          click: () => mainWindow?.webContents.send('menu-action', 'generate'),
        },
        { type: 'separator' },
        { role: 'quit' },
      ],
    },
    {
      label: 'Edit',
      submenu: [
        {
          label: 'Undo',
          accelerator: 'CmdOrCtrl+Z',
          click: () => mainWindow?.webContents.send('menu-action', 'undo'),
        },
        {
          label: 'Redo',
          accelerator: 'CmdOrCtrl+Shift+Z',
          click: () => mainWindow?.webContents.send('menu-action', 'redo'),
        },
        { type: 'separator' },
        { role: 'cut' },
        { role: 'copy' },
        { role: 'paste' },
        { role: 'selectAll' },
      ],
    },
    {
      label: 'View',
      submenu: [
        {
          label: 'Toggle Module Tree',
          accelerator: 'CmdOrCtrl+1',
          click: () => mainWindow?.webContents.send('menu-action', 'toggle-tree'),
        },
        {
          label: 'Toggle Properties',
          accelerator: 'CmdOrCtrl+2',
          click: () => mainWindow?.webContents.send('menu-action', 'toggle-properties'),
        },
        {
          label: 'Toggle Console',
          accelerator: 'CmdOrCtrl+`',
          click: () => mainWindow?.webContents.send('menu-action', 'toggle-console'),
        },
        { type: 'separator' },
        {
          label: 'Reload',
          accelerator: 'CmdOrCtrl+R',
          click: () => mainWindow?.reload(),
        },
        {
          label: 'Toggle Developer Tools',
          accelerator: isDev ? 'Alt+CmdOrCtrl+I' : '',
          visible: isDev,
          click: () => mainWindow?.webContents.toggleDevTools(),
        },
        { type: 'separator' },
        { role: 'resetZoom' },
        { role: 'zoomIn' },
        { role: 'zoomOut' },
        { type: 'separator' },
        { role: 'togglefullscreen' },
      ],
    },
    {
      label: 'Help',
      submenu: [
        {
          label: 'Documentation',
          click: () => shell.openExternal('https://github.com/yuletech/yuleASR/wiki'),
        },
        {
          label: 'AUTOSAR Specification',
          click: () => shell.openExternal('https://www.autosar.org/standards'),
        },
        { type: 'separator' },
        {
          label: 'About yuleASR',
          click: () => {
            dialog.showMessageBox(mainWindow!, {
              type: 'info',
              title: 'About yuleASR Configurator',
              message: 'yuleASR Configurator',
              detail: `Version: 1.0.0\nPlatform: ${process.platform}\nElectron: ${process.versions.electron}\nChrome: ${process.versions.chrome}\nNode.js: ${process.versions.node}`,
            })
          },
        },
      ],
    },
  ]

  if (process.platform === 'darwin') {
    template.unshift({
      label: app.getName(),
      submenu: [
        { role: 'about' },
        { type: 'separator' },
        { role: 'services' },
        { type: 'separator' },
        { role: 'hide' },
        { role: 'hideOthers' },
        { role: 'unhide' },
        { type: 'separator' },
        { role: 'quit' },
      ],
    })
  }

  const menu = Menu.buildFromTemplate(template)
  Menu.setApplicationMenu(menu)
}

// IPC Handlers
ipcMain.handle('read-file', async (_event, filePath: string) => {
  try {
    const content = fs.readFileSync(filePath, 'utf-8')
    return { success: true, content }
  } catch (error: any) {
    return { success: false, error: error.message }
  }
})

ipcMain.handle('write-file', async (_event, filePath: string, content: string) => {
  try {
    fs.writeFileSync(filePath, content, 'utf-8')
    return { success: true }
  } catch (error: any) {
    return { success: false, error: error.message }
  }
})

ipcMain.handle('show-open-dialog', async (_event, options: Electron.OpenDialogOptions) => {
  return dialog.showOpenDialog(mainWindow!, options)
})

ipcMain.handle('show-save-dialog', async (_event, options: Electron.SaveDialogOptions) => {
  return dialog.showSaveDialog(mainWindow!, options)
})

ipcMain.handle('get-app-info', () => {
  return {
    version: app.getVersion(),
    platform: process.platform,
    electron: process.versions.electron,
    chrome: process.versions.chrome,
    node: process.versions.node,
  }
})

// App lifecycle
app.whenReady().then(() => {
  createWindow()
  createMenu()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow()
    }
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})
