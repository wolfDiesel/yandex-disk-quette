export interface TreeNode {
  path: string
  name: string
  dir: boolean
  children?: TreeNode[]
  checked?: boolean
}

export interface ContentItem {
  path: string
  name: string
  dir: boolean
  size?: number
  modified?: string
}

export type SyncStatus = 'off' | 'on' | 'syncing' | 'error'

export interface StatusBarState {
  quotaUsed: number
  quotaTotal: number
  syncStatus: SyncStatus
  syncMessage: string
  online?: boolean
  speed?: number
}

export interface SettingsForm {
  syncPath: string
  refreshIntervalSec: number
  cloudCheckIntervalSec: number
  maxRetries: number
  hideToTray: boolean
  closeToTray: boolean
}

export interface BridgeApi {
  getTreeJson: () => Promise<string>
  setPathChecked: (path: string, checked: boolean) => void
  requestChildrenForPath: (path: string) => void
  requestContentsForPath: (path: string) => void
  getSettings: () => Promise<string>
  saveSettings: (json: string) => Promise<boolean>
  getLayoutState: () => Promise<string>
  saveLayoutState: (json: string) => Promise<boolean>
  chooseSyncFolder: (currentPath: string) => Promise<string>
  startSync: () => void
  stopSync: () => void
  downloadFile: (cloudPath: string) => void
  openFileFromCloud: (cloudPath: string) => void
  deleteFromDisk: (cloudPath: string) => void
  downloadFinished?: { connect: (cb: (success: boolean, errorMessage: string) => void) => void }
  deleteFinished?: { connect: (cb: (success: boolean, errorMessage: string) => void) => void }
}

declare global {
  interface Window {
    __bridge?: BridgeApi
    __TREE_DATA__?: TreeNode[]
    qt?: { webChannelTransport?: unknown }
    QWebChannel?: new (transport: unknown, callback: (channel: { objects: { bridge: BridgeApi } }) => void) => void
    __onBridgeReady__?: () => void
    renderTree?: () => void
    bindTreeClicks?: () => void
    __injectChildren__?: (path: string, children: unknown) => void
    __onContentsLoaded__?: (path: string, items: unknown) => void
    __updateStatusBar__?: (jsonStr: string) => void
  }
}
