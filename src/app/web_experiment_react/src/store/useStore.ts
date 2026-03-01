import type { ContentItem, SettingsForm, StatusBarState, TreeNode } from '@/lib/bridge-types'
import { create } from 'zustand'

function setChildrenAtPath(nodes: TreeNode[], path: string, children: TreeNode[]): TreeNode[] {
  return nodes.map((n) => {
    if (n.path !== path) {
      if (n.children) {
        return { ...n, children: setChildrenAtPath(n.children, path, children) }
      }
      return n
    }
    return { ...n, children }
  })
}

const defaultSettings: SettingsForm = {
  syncPath: '',
  refreshIntervalSec: 60,
  cloudCheckIntervalSec: 30,
  maxRetries: 3,
  hideToTray: false,
  closeToTray: false,
}

function getParentPath(path: string): string {
  if (!path || path === '/') return ''
  const s = path.replace(/\/$/, '')
  const i = s.lastIndexOf('/')
  return i <= 0 ? '/' : s.slice(0, i)
}

interface AppState {
  treeData: TreeNode[]
  contentsPath: string | null
  contentsItems: ContentItem[]
  contentsLoading: boolean
  statusBar: StatusBarState
  settingsForm: SettingsForm
  activePane: 'nav' | 'settings'
  viewMode: 'list' | 'table' | 'icons'
  toastMessage: string | null
  selectedTreePath: string | null
  expandedPaths: Set<string>
  treeWidth: number
  sidebarCollapsed: boolean
  theme: 'light' | 'dark' | 'system'
  openingFilePath: string | null
}

interface AppActions {
  setTreeData: (data: TreeNode[]) => void
  injectChildren: (path: string, children: TreeNode[]) => void
  setContents: (path: string, items: ContentItem[]) => void
  setContentsPathLoading: (path: string) => void
  setContentsLoading: (loading: boolean) => void
  updateStatusBar: (data: Partial<StatusBarState>) => void
  setSettingsForm: (data: Partial<SettingsForm> | SettingsForm) => void
  setActivePane: (pane: 'nav' | 'settings') => void
  setViewMode: (mode: 'list' | 'table' | 'icons') => void
  showToast: (message: string | null) => void
  setSelectedTreePath: (path: string | null) => void
  toggleExpanded: (path: string) => void
  expandPath: (path: string) => void
  setTreeWidth: (width: number) => void
  setSidebarCollapsed: (collapsed: boolean) => void
  setTheme: (theme: 'light' | 'dark' | 'system') => void
  setOpeningFilePath: (path: string | null) => void
}

const initialStatus: StatusBarState = {
  quotaUsed: 0,
  quotaTotal: 0,
  syncStatus: 'off',
  syncMessage: '',
  online: true,
  speed: 0,
}

export const useStore = create<AppState & AppActions>((set) => ({
  treeData: [],
  contentsPath: null,
  contentsItems: [],
  contentsLoading: false,
  statusBar: initialStatus,
  settingsForm: defaultSettings,
  activePane: 'nav',
  viewMode: 'list',
  toastMessage: null,
  selectedTreePath: null,
  expandedPaths: new Set<string>(),
  treeWidth: 280,
  sidebarCollapsed: false,
  theme: 'system',
  openingFilePath: null,

  setOpeningFilePath: (path) => set({ openingFilePath: path }),

  setTreeData: (data) =>
    set((state) => {
      const arr = Array.isArray(data) ? data : []
      const nextExpanded = new Set(state.expandedPaths)
      arr.forEach((n) => n.path && nextExpanded.add(n.path))
      return { treeData: arr, expandedPaths: nextExpanded }
    }),

  injectChildren: (path, children) =>
    set((state) => ({
      treeData: setChildrenAtPath(state.treeData, path, children),
    })),

  setContents: (path, items) =>
    set({
      contentsPath: path,
      contentsItems: Array.isArray(items) ? items : [],
      contentsLoading: false,
    }),

  setContentsPathLoading: (path) =>
    set({
      contentsPath: path,
      contentsItems: [],
      contentsLoading: true,
    }),

  setContentsLoading: (loading) => set({ contentsLoading: loading }),

  updateStatusBar: (data) =>
    set((state) => ({
      statusBar: { ...state.statusBar, ...data },
    })),

  setSettingsForm: (data) =>
    set((state) => ({
      settingsForm: { ...state.settingsForm, ...data },
    })),

  setActivePane: (activePane) => set({ activePane }),
  setViewMode: (viewMode) => set({ viewMode }),
  showToast: (toastMessage) => set({ toastMessage }),

  setSelectedTreePath: (selectedTreePath) => set({ selectedTreePath }),

  toggleExpanded: (path) =>
    set((state) => {
      const next = new Set(state.expandedPaths)
      if (next.has(path)) next.delete(path)
      else next.add(path)
      return { expandedPaths: next }
    }),

  expandPath: (path) =>
    set((state) => {
      const next = new Set(state.expandedPaths)
      let p: string | null = path
      while (p && p !== '/') {
        const parent = getParentPath(p)
        if (parent) next.add(parent)
        p = parent || null
      }
      next.add(path)
      return { expandedPaths: next }
    }),

  setTreeWidth: (w) => set({ treeWidth: Math.min(600, Math.max(160, w)) }),
  setSidebarCollapsed: (sidebarCollapsed) => set({ sidebarCollapsed }),
  setTheme: (theme) => set({ theme }),
}))
