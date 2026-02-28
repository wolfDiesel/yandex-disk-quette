import type { BridgeApi } from '@/lib/bridge-types'
import { useStore } from '@/store/useStore'
import { createContext, useContext, useEffect, useState, type ReactNode } from 'react'

export const BridgeContext = createContext<BridgeApi | null>(null)

export function BridgeProvider({ children }: { children: ReactNode }) {
  const [bridge, setBridge] = useState<BridgeApi | null>(null)

  useEffect(() => {
    const QWebChannel = (window as Window & { QWebChannel?: typeof window.QWebChannel }).QWebChannel
    const transport = window.qt?.webChannelTransport
    if (QWebChannel && transport) {
      new QWebChannel(transport, (ch: { objects: { bridge: BridgeApi } }) => {
        const b = ch.objects.bridge
        window.__bridge = b
        setBridge(b)
        if (typeof b.getLayoutState === 'function') {
          b.getLayoutState().then((json) => {
            try {
              const data = JSON.parse(json)
              const store = useStore.getState()
              if (typeof data.treeWidth === 'number') store.setTreeWidth(data.treeWidth)
              if (typeof data.sidebarCollapsed === 'boolean') store.setSidebarCollapsed(data.sidebarCollapsed)
              if (data.theme === 'light' || data.theme === 'dark' || data.theme === 'system') store.setTheme(data.theme)
            } catch {
              // ignore
            }
          })
        }
        if (b.downloadFinished?.connect) {
          b.downloadFinished.connect((success: boolean, errorMessage: string) => {
            useStore.getState().setOpeningFilePath(null)
            if (!success && errorMessage) useStore.getState().showToast(errorMessage)
          })
        }
        if (b.deleteFinished?.connect) {
          b.deleteFinished.connect((success: boolean, errorMessage: string) => {
            if (!success && errorMessage) useStore.getState().showToast(errorMessage)
            else if (success) {
              const path = useStore.getState().contentsPath
              if (path && b.requestContentsForPath) b.requestContentsForPath(path)
            }
          })
        }
        window.__onBridgeReady__?.()
      })
    }
  }, [])

  useEffect(() => {
    window.renderTree = () => {
      const data = window.__TREE_DATA__
      useStore.getState().setTreeData(Array.isArray(data) ? data : [])
    }
    window.bindTreeClicks = () => {}
    window.__injectChildren__ = (path: string, children: unknown) => {
      useStore.getState().injectChildren(path, Array.isArray(children) ? children : [])
    }
    window.__onContentsLoaded__ = (path: string, items: unknown) => {
      useStore.getState().setContents(path, Array.isArray(items) ? items : [])
    }
    window.__updateStatusBar__ = (jsonStr: string) => {
      try {
        const data = typeof jsonStr === 'string' ? JSON.parse(jsonStr) : jsonStr
        const status = data.syncStatus ?? 'off'
        useStore.getState().updateStatusBar({
          quotaUsed: data.quotaUsed ?? 0,
          quotaTotal: data.quotaTotal ?? 0,
          syncStatus: status,
          syncMessage: data.syncMessage ?? '',
          online: data.online !== false,
          speed: typeof data.speed === 'number' ? data.speed : 0,
        })
      } catch {
        // ignore
      }
    }
    return () => {
      delete (window as unknown as { renderTree?: () => void }).renderTree
      delete (window as unknown as { bindTreeClicks?: () => void }).bindTreeClicks
      delete (window as unknown as { __injectChildren__?: (path: string, children: unknown) => void }).__injectChildren__
      delete (window as unknown as { __onContentsLoaded__?: (path: string, items: unknown) => void }).__onContentsLoaded__
      delete (window as unknown as { __updateStatusBar__?: (jsonStr: string) => void }).__updateStatusBar__
    }
  }, [])

  return <BridgeContext.Provider value={bridge}>{children}</BridgeContext.Provider>
}

export function useBridge(): BridgeApi | null {
  return useContext(BridgeContext)
}
