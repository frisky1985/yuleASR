export {}

declare global {
  interface Window {
    electronAPI?: {
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
  }
}
