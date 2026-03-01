import { useRef, useEffect } from 'react'
import { useBridge } from '@/contexts/BridgeContext'
import { useStore } from '@/store/useStore'
import type { TreeNode } from '@/lib/bridge-types'
import { ScrollArea } from '@/components/ui/scroll-area'
import { Checkbox } from '@/components/ui/checkbox'
import { ChevronRight, Folder, FolderOpen } from 'lucide-react'
import { cn } from '@/lib/utils'

export function Tree() {
  const treeData = useStore((s) => s.treeData)
  const treeWidth = useStore((s) => s.treeWidth)
  const selectedTreePath = useStore((s) => s.selectedTreePath)
  const expandedPaths = useStore((s) => s.expandedPaths)
  const toggleExpanded = useStore((s) => s.toggleExpanded)
  const setSelectedTreePath = useStore((s) => s.setSelectedTreePath)
  const setContentsPathLoading = useStore((s) => s.setContentsPathLoading)
  const expandPath = useStore((s) => s.expandPath)
  const bridge = useBridge()

  if (!Array.isArray(treeData) || treeData.length === 0) {
    return (
      <div
        className="h-full min-h-0 shrink-0 border-r border-border p-2 text-sm text-muted-foreground overflow-hidden"
        style={{ width: treeWidth }}
      >
        {treeData ? 'No tree data.' : 'Loading…'}
      </div>
    )
  }

  const handleFolderClick = (path: string) => {
    setContentsPathLoading(path)
    bridge?.requestContentsForPath(path)
    bridge?.requestChildrenForPath(path)
    setSelectedTreePath(path)
    expandPath(path)
  }

  return (
    <div className="h-full min-h-0 shrink-0 flex flex-col overflow-hidden" style={{ width: treeWidth }}>
      <ScrollArea className="flex-1 min-h-0 border-r border-border">
        <div className="p-2">
        <ul className="list-none m-0 p-0 space-y-0.5">
          {treeData.map((n) => (
            <TreeNodeRow
              key={n.path}
              node={n}
              level={0}
              expandedPaths={expandedPaths}
              selectedTreePath={selectedTreePath}
              onToggleExpanded={toggleExpanded}
              onFolderClick={handleFolderClick}
              bridge={bridge}
            />
          ))}
        </ul>
      </div>
      </ScrollArea>
    </div>
  )
}

interface TreeNodeRowProps {
  node: TreeNode
  level: number
  expandedPaths: Set<string>
  selectedTreePath: string | null
  onToggleExpanded: (path: string) => void
  onFolderClick: (path: string) => void
  bridge: ReturnType<typeof useBridge>
}

function TreeNodeRow({
  node,
  level,
  expandedPaths,
  selectedTreePath,
  onToggleExpanded,
  onFolderClick,
  bridge,
}: TreeNodeRowProps) {
  const path = node.path || '/'
  const isExpanded = expandedPaths.has(path)
  const isSelected = selectedTreePath === path
  const hasChildren = node.children && node.children.length > 0
  const childrenLoaded = node.children !== undefined
  const liRef = useRef<HTMLLIElement>(null)

  useEffect(() => {
    if (isSelected && liRef.current) {
      liRef.current.scrollIntoView({ block: 'nearest', behavior: 'smooth' })
    }
  }, [isSelected])

  const handleExpandClick = (e: React.MouseEvent) => {
    e.stopPropagation()
    if (!hasChildren && bridge?.requestChildrenForPath) {
      bridge.requestChildrenForPath(path)
    }
    onToggleExpanded(path)
  }

  const handleLabelClick = () => {
    onFolderClick(path)
  }

  const handleCheckboxChange = (checked: boolean | 'indeterminate') => {
    bridge?.setPathChecked(path, checked === true)
  }

  return (
    <li ref={liRef} className="node-dir" data-path={path}>
      <div
        role="button"
        tabIndex={0}
        className={cn(
          'flex items-center gap-2 py-1.5 px-2 rounded-md cursor-pointer hover:bg-accent min-h-8',
          isSelected && 'bg-primary/10 text-primary font-medium'
        )}
        onClick={handleLabelClick}
        onKeyDown={(e) => {
          if (e.key === 'Enter' || e.key === ' ') {
            e.preventDefault()
            onToggleExpanded(path)
          }
        }}
      >
        <button
          type="button"
          aria-label={isExpanded ? 'Свернуть' : 'Развернуть'}
          className="shrink-0 size-5 flex items-center justify-center text-muted-foreground"
          onClick={handleExpandClick}
        >
          <ChevronRight className={cn('size-4 transition-transform', isExpanded && 'rotate-90')} />
        </button>
        <Checkbox
          checked={!!node.checked}
          onCheckedChange={handleCheckboxChange}
          onClick={(e) => e.stopPropagation()}
        />
        <span className="shrink-0 text-amber-600 dark:text-amber-500">
          {isExpanded ? <FolderOpen className="size-4" /> : <Folder className="size-4" />}
        </span>
        <span className="min-w-0 truncate flex-1">{node.name || path || '/'}</span>
        {isSelected && (
          <span
            className="shrink-0 inline-block w-3 h-3 ml-0.5 rounded-sm bg-green-600 dark:bg-green-500"
            style={{ clipPath: 'polygon(0 0, 100% 50%, 0 100%)' }}
            aria-hidden
          />
        )}
      </div>
      {node.dir && isExpanded && (
        <div className="pl-4 ml-1 border-l border-border">
          {hasChildren ? (
            <ul className="list-none m-0 p-0 space-y-0.5">
              {node.children!.map((child) => (
              <TreeNodeRow
                key={child.path}
                node={child}
                level={level + 1}
                expandedPaths={expandedPaths}
                selectedTreePath={selectedTreePath}
                onToggleExpanded={onToggleExpanded}
                onFolderClick={onFolderClick}
                bridge={bridge}
              />
            ))}
          </ul>
          ) : !childrenLoaded ? (
            <span className="text-muted-foreground text-xs px-2 py-1">…</span>
          ) : null}
        </div>
      )}
    </li>
  )
}
