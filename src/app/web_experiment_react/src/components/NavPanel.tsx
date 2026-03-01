import { useRef, useCallback } from 'react'
import { useBridge } from '@/contexts/BridgeContext'
import { useStore } from '@/store/useStore'
import { Button } from '@/components/ui/button'
import { Separator } from '@/components/ui/separator'
import { cn } from '@/lib/utils'
import { BreadcrumbsBar } from './BreadcrumbsBar'
import { ViewModeButtons } from './Contents'
import { Tree } from './Tree'
import { Contents } from './Contents'

const TREE_WIDTH_MIN = 160
const TREE_WIDTH_MAX = 600

export function NavPanel() {
  const bridge = useBridge()
  const statusBar = useStore((s) => s.statusBar)
  const syncing = statusBar.syncStatus === 'syncing'
  const setTreeWidth = useStore((s) => s.setTreeWidth)
  const rowRef = useRef<HTMLDivElement>(null)

  const saveLayout = useCallback(() => {
    if (!bridge?.saveLayoutState) return
    const state = useStore.getState()
    bridge.saveLayoutState(JSON.stringify({ treeWidth: state.treeWidth, sidebarCollapsed: state.sidebarCollapsed, theme: state.theme }))
  }, [bridge])

  const handleResizerMouseDown = useCallback(
    (e: React.MouseEvent) => {
      e.preventDefault()
      const onMove = (e: MouseEvent) => {
        const row = rowRef.current
        if (!row) return
        const left = row.getBoundingClientRect().left
        const next = Math.min(TREE_WIDTH_MAX, Math.max(TREE_WIDTH_MIN, e.clientX - left))
        setTreeWidth(next)
      }
      const onUp = () => {
        window.removeEventListener('mousemove', onMove)
        window.removeEventListener('mouseup', onUp)
        saveLayout()
      }
      window.addEventListener('mousemove', onMove)
      window.addEventListener('mouseup', onUp)
    },
    [setTreeWidth, saveLayout]
  )

  return (
    <div className="flex-1 flex flex-col min-h-0 min-w-0">
      <div className="shrink-0 flex items-center gap-2 border-b border-border bg-muted/30 px-4 py-2">
        <BreadcrumbsBar />
        <Separator orientation="vertical" className="h-5 shrink-0" />
        {bridge && (
          <>
            <Button
              variant="default"
              size="xs"
              className="shrink-0"
              disabled={syncing}
              onClick={() => bridge.startSync()}
              title="Синхронизировать"
            >
              Синхр.
            </Button>
            <Button
              variant="secondary"
              size="xs"
              className={cn('shrink-0', syncing ? '' : 'hidden')}
              onClick={() => bridge.stopSync()}
              title="Остановить"
            >
              Стоп
            </Button>
            <Separator orientation="vertical" className="h-5 shrink-0" />
          </>
        )}
        <ViewModeButtons />
      </div>
      <div ref={rowRef} className="flex-1 flex min-h-0 min-w-0 overflow-hidden">
        <Tree />
        <div
          role="separator"
          aria-orientation="vertical"
          className="w-1.5 shrink-0 cursor-col-resize bg-border hover:bg-primary/30 active:bg-primary/50 transition-colors flex items-stretch"
          onMouseDown={handleResizerMouseDown}
          title="Изменить ширину панели"
        />
        <Contents />
      </div>
    </div>
  )
}
