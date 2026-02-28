import { useState, useCallback, useEffect } from 'react'
import { useBridge } from '@/contexts/BridgeContext'
import { useStore } from '@/store/useStore'
import type { ContentItem } from '@/lib/bridge-types'
import { formatSize, itemTooltip, iconForItem } from '@/lib/format'
import { Button } from '@/components/ui/button'
import { Skeleton } from '@/components/ui/skeleton'
import { List, LayoutGrid, Table } from 'lucide-react'
import { cn } from '@/lib/utils'

type ContextMenuState = { path: string; isDir: boolean; x: number; y: number } | null

export function Contents() {
  const contentsPath = useStore((s) => s.contentsPath)
  const contentsItems = useStore((s) => s.contentsItems)
  const contentsLoading = useStore((s) => s.contentsLoading)
  const viewMode = useStore((s) => s.viewMode)
  const setContentsPathLoading = useStore((s) => s.setContentsPathLoading)
  const setSelectedTreePath = useStore((s) => s.setSelectedTreePath)
  const expandPath = useStore((s) => s.expandPath)
  const bridge = useBridge()
  const [ctxMenu, setCtxMenu] = useState<ContextMenuState>(null)

  useEffect(() => {
    if (!ctxMenu) return
    const close = () => setCtxMenu(null)
    window.addEventListener('click', close)
    window.addEventListener('scroll', close, true)
    return () => {
      window.removeEventListener('click', close)
      window.removeEventListener('scroll', close, true)
    }
  }, [ctxMenu])

  const syncTreeToPath = useCallback(
    (path: string) => {
      setSelectedTreePath(path)
      expandPath(path)
    },
    [setSelectedTreePath, expandPath]
  )

  const handleFolderClick = useCallback(
    (path: string) => {
      if (!bridge?.requestContentsForPath) return
      setContentsPathLoading(path)
      bridge.requestContentsForPath(path)
      syncTreeToPath(path)
    },
    [bridge, setContentsPathLoading, syncTreeToPath]
  )

  const handleFileDblClick = useCallback(
    (path: string) => {
      bridge?.openFileFromCloud(path)
    },
    [bridge]
  )

  if (contentsPath === null) {
    return (
      <div className="flex-1 overflow-auto p-4 flex items-center justify-center text-sm text-muted-foreground">
        Кликните по папке слева.
      </div>
    )
  }

  if (contentsLoading) {
    return (
      <div className="flex-1 overflow-auto p-4 min-h-0">
        {viewMode === 'list' && <ContentsListSkeleton />}
        {viewMode === 'table' && <ContentsTableSkeleton />}
        {viewMode === 'icons' && <ContentsIconsSkeleton />}
      </div>
    )
  }

  if (!contentsItems.length) {
    return (
      <div className="flex-1 overflow-auto p-4 text-sm text-muted-foreground">
        Папка пуста.
      </div>
    )
  }

  return (
    <div className="flex-1 overflow-auto p-4 min-h-0 relative">
      {viewMode === 'list' && (
        <ul className="list-none m-0 p-0 space-y-0.5">
          {contentsItems.map((it) => (
            <ContentRow
              key={it.path}
              item={it}
              onFolderClick={handleFolderClick}
              onFileDblClick={handleFileDblClick}
              onContextMenu={(e) => { e.preventDefault(); setCtxMenu({ path: it.path, isDir: it.dir, x: e.clientX, y: e.clientY }) }}
            />
          ))}
        </ul>
      )}
      {viewMode === 'table' && (
        <table className="w-full text-sm border-collapse">
          <thead>
            <tr className="border-b border-border text-left text-muted-foreground">
              <th className="py-2 px-2 w-8" />
              <th className="py-2 px-2 font-medium">Имя</th>
              <th className="py-2 px-2 w-24">Размер</th>
              <th className="py-2 px-2 w-32">Изменён</th>
            </tr>
          </thead>
          <tbody>
            {contentsItems.map((it) => (
              <ContentTableRow
                key={it.path}
                item={it}
                onFolderClick={handleFolderClick}
                onFileDblClick={handleFileDblClick}
                onContextMenu={(e) => { e.preventDefault(); setCtxMenu({ path: it.path, isDir: it.dir, x: e.clientX, y: e.clientY }) }}
              />
            ))}
          </tbody>
        </table>
      )}
      {viewMode === 'icons' && (
        <div className="grid grid-cols-[repeat(auto-fill,minmax(6rem,1fr))] gap-3">
          {contentsItems.map((it) => (
            <ContentIconCard
              key={it.path}
              item={it}
              onFolderClick={handleFolderClick}
              onFileDblClick={handleFileDblClick}
              onContextMenu={(e) => { e.preventDefault(); setCtxMenu({ path: it.path, isDir: it.dir, x: e.clientX, y: e.clientY }) }}
            />
          ))}
        </div>
      )}
      {ctxMenu && (
        <div
          className="fixed z-50 min-w-40 rounded-md border border-border bg-background shadow-lg py-1"
          style={{ left: ctxMenu.x, top: ctxMenu.y }}
        >
          <button
            type="button"
            className="w-full text-left px-3 py-2 text-sm hover:bg-accent disabled:opacity-50"
            disabled={ctxMenu.isDir}
            onClick={() => { bridge?.downloadFile(ctxMenu.path); setCtxMenu(null) }}
          >
            Скачать
          </button>
          <button
            type="button"
            className="w-full text-left px-3 py-2 text-sm hover:bg-accent text-destructive"
            onClick={() => {
              if (window.confirm(`Удалить «${ctxMenu.path}» с Яндекс.Диска?`)) bridge?.deleteFromDisk(ctxMenu.path)
              setCtxMenu(null)
            }}
          >
            Удалить с Диска
          </button>
        </div>
      )}
    </div>
  )
}

function ContentsListSkeleton() {
  return (
    <ul className="list-none m-0 p-0 space-y-0.5">
      {Array.from({ length: 10 }).map((_, i) => (
        <li key={i} className="flex items-center gap-2 py-2 px-2 rounded-md min-h-10">
          <Skeleton className="shrink-0 size-6 rounded" />
          <Skeleton className="h-4 flex-1 max-w-[60%]" />
          {i % 3 !== 0 && <Skeleton className="h-4 w-12 shrink-0" />}
        </li>
      ))}
    </ul>
  )
}

function ContentsTableSkeleton() {
  return (
    <table className="w-full text-sm border-collapse">
      <thead>
        <tr className="border-b border-border text-left">
          <th className="py-2 px-2 w-8" />
          <th className="py-2 px-2 font-medium">Имя</th>
          <th className="py-2 px-2 w-24">Размер</th>
          <th className="py-2 px-2 w-32">Изменён</th>
        </tr>
      </thead>
      <tbody>
        {Array.from({ length: 10 }).map((_, i) => (
          <tr key={i} className="border-b border-border/50">
            <td className="py-1.5 px-2"><Skeleton className="size-5 rounded mx-auto" /></td>
            <td className="py-1.5 px-2"><Skeleton className="h-4 max-w-[12rem]" /></td>
            <td className="py-1.5 px-2"><Skeleton className="h-4 w-14" /></td>
            <td className="py-1.5 px-2"><Skeleton className="h-4 w-24" /></td>
          </tr>
        ))}
      </tbody>
    </table>
  )
}

function ContentsIconsSkeleton() {
  return (
    <div className="grid grid-cols-[repeat(auto-fill,minmax(6rem,1fr))] gap-3">
      {Array.from({ length: 12 }).map((_, i) => (
        <div key={i} className="flex flex-col items-center p-3">
          <Skeleton className="size-12 rounded-lg mb-2" />
          <Skeleton className="h-3 w-full max-w-[5rem]" />
        </div>
      ))}
    </div>
  )
}

interface ContentRowProps {
  item: ContentItem
  onFolderClick: (path: string) => void
  onFileDblClick: (path: string) => void
  onContextMenu: (e: React.MouseEvent) => void
}

function ContentRow({ item, onFolderClick, onFileDblClick, onContextMenu }: ContentRowProps) {
  const isDir = item.dir
  return (
    <li
      className="flex items-center gap-2 py-2 px-2 rounded-md hover:bg-accent min-h-10 cursor-pointer"
      data-path={item.path}
      data-dir={String(isDir)}
      onClick={() => isDir && onFolderClick(item.path)}
      onDoubleClick={() => !isDir && onFileDblClick(item.path)}
      onContextMenu={onContextMenu}
      title={itemTooltip(item)}
    >
      <span className="shrink-0 w-6 text-center" aria-hidden>
        {iconForItem(item)}
      </span>
      <span className="min-w-0 truncate flex-1">{item.name || item.path}</span>
      {!isDir && item.size !== undefined && (
        <span className="text-muted-foreground text-xs shrink-0">{formatSize(item.size)}</span>
      )}
    </li>
  )
}

function ContentTableRow({ item, onFolderClick, onFileDblClick, onContextMenu }: ContentRowProps) {
  const isDir = item.dir
  return (
    <tr
      className="border-b border-border/50 hover:bg-accent/50 cursor-pointer"
      data-path={item.path}
      data-dir={String(isDir)}
      onClick={() => isDir && onFolderClick(item.path)}
      onDoubleClick={() => !isDir && onFileDblClick(item.path)}
      onContextMenu={onContextMenu}
      title={itemTooltip(item)}
    >
      <td className="py-1.5 px-2 text-center">{iconForItem(item)}</td>
      <td className="py-1.5 px-2 min-w-0 truncate max-w-md">{item.name || item.path}</td>
      <td className="py-1.5 px-2 text-muted-foreground">{isDir ? '—' : formatSize(item.size)}</td>
      <td className="py-1.5 px-2 text-muted-foreground text-xs">{item.modified ?? '—'}</td>
    </tr>
  )
}

function ContentIconCard({ item, onFolderClick, onFileDblClick, onContextMenu }: ContentRowProps) {
  const isDir = item.dir
  return (
    <div
      className="flex flex-col items-center p-3 rounded-lg hover:bg-accent cursor-pointer"
      data-path={item.path}
      data-dir={String(isDir)}
      onClick={() => isDir && onFolderClick(item.path)}
      onDoubleClick={() => !isDir && onFileDblClick(item.path)}
      onContextMenu={onContextMenu}
      title={itemTooltip(item)}
    >
      <span className="text-3xl mb-1" aria-hidden>
        {iconForItem(item)}
      </span>
      <span className="text-xs text-center truncate w-full">{item.name || item.path}</span>
    </div>
  )
}

export function ViewModeButtons() {
  const viewMode = useStore((s) => s.viewMode)
  const setViewMode = useStore((s) => s.setViewMode)
  return (
    <div className="flex items-center gap-2 shrink-0">
      <Button
        variant="ghost"
        size="icon"
        className={cn(viewMode === 'list' && 'bg-accent')}
        onClick={() => setViewMode('list')}
        title="Список"
      >
        <List className="size-4" />
      </Button>
      <Button
        variant="ghost"
        size="icon"
        className={cn(viewMode === 'table' && 'bg-accent')}
        onClick={() => setViewMode('table')}
        title="Таблица"
      >
        <Table className="size-4" />
      </Button>
      <Button
        variant="ghost"
        size="icon"
        className={cn(viewMode === 'icons' && 'bg-accent')}
        onClick={() => setViewMode('icons')}
        title="Значки"
      >
        <LayoutGrid className="size-4" />
      </Button>
    </div>
  )
}
