import { useCallback } from 'react'
import { useBridge } from '@/contexts/BridgeContext'
import { useStore } from '@/store/useStore'
import { Button } from '@/components/ui/button'
import { ScrollArea } from '@/components/ui/scroll-area'
import { FolderOpen, Settings, PanelLeftClose, PanelLeft, RefreshCw } from 'lucide-react'
import { cn } from '@/lib/utils'
import { NavPanel } from './NavPanel'
import { ThemeToggle } from './ThemeToggle'
import { SettingsPanel } from './SettingsPanel'
import { StatusBar } from './StatusBar'
import { MenuBar } from './MenuBar'

export function Layout() {
  const activePane = useStore((s) => s.activePane)
  const setActivePane = useStore((s) => s.setActivePane)
  const sidebarCollapsed = useStore((s) => s.sidebarCollapsed)
  const setSidebarCollapsed = useStore((s) => s.setSidebarCollapsed)
  const bridge = useBridge()

  const saveLayout = useCallback(() => {
    if (!bridge?.saveLayoutState) return
    const state = useStore.getState()
    bridge.saveLayoutState(JSON.stringify({ treeWidth: state.treeWidth, sidebarCollapsed: state.sidebarCollapsed, theme: state.theme }))
  }, [bridge])

  const toggleSidebar = useCallback(() => {
    setSidebarCollapsed(!sidebarCollapsed)
    setTimeout(() => saveLayout(), 0)
  }, [sidebarCollapsed, setSidebarCollapsed, saveLayout])

  return (
    <div className="flex h-full min-h-0 flex-col bg-background text-foreground overflow-hidden">
      <MenuBar />
      <div className="shrink-0 flex justify-end items-center pr-3 py-2 border-b border-border bg-muted/30">
        <ThemeToggle onAfterChange={saveLayout} />
      </div>
      <div className="flex flex-1 min-h-0 overflow-hidden">
        <aside
          className={cn('flex shrink-0 flex-col border-r border-border bg-muted/80 transition-[width] duration-200 ease-out', sidebarCollapsed ? 'w-[52px]' : 'w-60')}
        >
          <div className={cn('shrink-0 border-b border-border flex items-center min-h-12 px-2 py-3', sidebarCollapsed ? 'justify-center' : 'justify-between')}>
            {!sidebarCollapsed && <h1 className="text-sm font-semibold tracking-tight truncate">Y.Disquette</h1>}
            <Button
              variant="ghost"
              size="icon"
              className={cn('shrink-0', sidebarCollapsed && 'mx-auto')}
              onClick={toggleSidebar}
              title={sidebarCollapsed ? 'Развернуть панель' : 'Свернуть панель'}
              aria-label={sidebarCollapsed ? 'Развернуть панель' : 'Свернуть панель'}
            >
              {sidebarCollapsed ? <PanelLeft className="size-4" /> : <PanelLeftClose className="size-4" />}
            </Button>
          </div>
          <ScrollArea className="flex-1 py-2">
            <nav className={cn('flex flex-col gap-0.5', sidebarCollapsed ? 'px-1.5' : 'px-2')}>
              <button
                type="button"
                onClick={() => setActivePane('nav')}
                className={cn(
                  'flex items-center rounded-md text-sm border-l-2 transition-colors',
                  sidebarCollapsed ? 'justify-center p-2.5' : 'gap-3 px-4 py-2.5 text-left',
                  activePane === 'nav'
                    ? 'bg-primary/10 border-primary text-foreground'
                    : 'border-transparent text-muted-foreground hover:bg-accent hover:text-foreground'
                )}
                title="Навигация"
              >
                <FolderOpen className="size-4 shrink-0" aria-hidden />
                {!sidebarCollapsed && <span>Навигация</span>}
              </button>
              <button
                type="button"
                onClick={() => setActivePane('settings')}
                className={cn(
                  'flex items-center rounded-md text-sm border-l-2 transition-colors',
                  sidebarCollapsed ? 'justify-center p-2.5' : 'gap-3 px-4 py-2.5 text-left',
                  activePane === 'settings'
                    ? 'bg-primary/10 border-primary text-foreground'
                    : 'border-transparent text-muted-foreground hover:bg-accent hover:text-foreground'
                )}
                title="Настройки"
              >
                <Settings className="size-4 shrink-0" aria-hidden />
                {!sidebarCollapsed && <span>Настройки</span>}
              </button>
            </nav>
          </ScrollArea>
          <div className="shrink-0 border-t border-border p-2">
            <RefreshTreeButton collapsed={sidebarCollapsed} />
          </div>
        </aside>
        <main className="flex-1 flex flex-col min-w-0 min-h-0 overflow-hidden">
          {activePane === 'nav' && <NavPanel />}
          {activePane === 'settings' && <SettingsPanel />}
        </main>
      </div>
      <StatusBar />
    </div>
  )
}

function RefreshTreeButton({ collapsed }: { collapsed: boolean }) {
  const bridge = useBridge()
  if (!bridge) return null
  const onClick = () => {
    bridge.getTreeJson().then((json) => {
      try {
        const data = JSON.parse(json)
        useStore.getState().setTreeData(Array.isArray(data) ? data : [])
      } catch {
        useStore.getState().setTreeData([])
      }
    })
  }
  if (collapsed) {
    return (
      <Button variant="default" size="icon" className="w-full" onClick={onClick} title="Обновить дерево" aria-label="Обновить дерево">
        <RefreshCw className="size-4" />
      </Button>
    )
  }
  return (
    <Button variant="default" size="sm" className="w-full text-xs" onClick={onClick}>
      Обновить дерево
    </Button>
  )
}
