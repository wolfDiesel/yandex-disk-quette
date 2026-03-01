import { useBridge } from '@/contexts/BridgeContext'
import { useStore } from '@/store/useStore'
import {
  Breadcrumb,
  BreadcrumbItem,
  BreadcrumbList,
  BreadcrumbPage,
  BreadcrumbSeparator,
} from '@/components/ui/breadcrumb'
import { Button } from '@/components/ui/button'

function pathToSegments(path: string): string[] {
  if (!path || path === '/') return []
  return path.replace(/^\/|\/$/g, '').split('/').filter(Boolean)
}

function pathSegmentFullPath(path: string, index: number): string {
  const segs = pathToSegments(path)
  if (index < 0 || index >= segs.length) return '/'
  return '/' + segs.slice(0, index + 1).join('/')
}

export function BreadcrumbsBar() {
  const bridge = useBridge()
  const contentsPath = useStore((s) => s.contentsPath)
  const setContentsPathLoading = useStore((s) => s.setContentsPathLoading)
  const setSelectedTreePath = useStore((s) => s.setSelectedTreePath)
  const expandPath = useStore((s) => s.expandPath)

  const syncTreeToPath = (path: string) => {
    setSelectedTreePath(path)
    expandPath(path)
  }

  const handleSegmentClick = (path: string) => {
    if (!bridge?.requestContentsForPath) return
    setContentsPathLoading(path)
    bridge.requestContentsForPath(path)
    syncTreeToPath(path)
  }

  const segs = contentsPath === '/' ? [''] : pathToSegments(contentsPath ?? '')
  const hasPath = contentsPath !== null && contentsPath !== ''

  if (!hasPath) {
    return (
      <nav className="min-w-0 flex-1 flex items-center" aria-label="breadcrumb">
        <ol className="flex items-center gap-1 min-w-0 flex-wrap text-sm text-muted-foreground truncate">
          <li>Выберите папку</li>
        </ol>
      </nav>
    )
  }

  return (
    <Breadcrumb className="min-w-0 flex-1">
      <BreadcrumbList className="min-w-0 flex-wrap">
        {segs.map((seg, i) => (
          <BreadcrumbItem key={i} className="min-w-0 shrink max-w-[8rem]">
            {i > 0 && <BreadcrumbSeparator />}
            {i < segs.length - 1 ? (
              <Button
                variant="ghost"
                size="xs"
                className="h-auto py-0.5 px-1.5 text-foreground font-normal truncate max-w-full"
                onClick={() => handleSegmentClick(pathSegmentFullPath(contentsPath!, i))}
              >
                {seg || 'Корень'}
              </Button>
            ) : (
              <BreadcrumbPage className="font-medium truncate">
                {seg || 'Корень'}
              </BreadcrumbPage>
            )}
          </BreadcrumbItem>
        ))}
      </BreadcrumbList>
    </Breadcrumb>
  )
}
