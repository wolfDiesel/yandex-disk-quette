import { useStore } from '@/store/useStore'
import { Progress } from '@/components/ui/progress'

const STATUS_TITLES: Record<string, string> = {
  off: 'Синхронизация выключена',
  on: 'Синхронизация включена',
  syncing: 'Синхронизация…',
  error: 'Ошибка синхронизации',
}

const STATUS_BG: Record<string, string> = {
  off: '#9e9e9e',
  on: '#4caf50',
  syncing: '#ff9800',
  error: '#f44336',
}

function formatBytes(bytes: number): string {
  if (bytes === undefined || bytes === null) return '—'
  if (bytes < 1024) return `${bytes} B`
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`
  return `${(bytes / (1024 * 1024 * 1024)).toFixed(1)} GB`
}

export function StatusBar() {
  const statusBar = useStore((s) => s.statusBar)
  const { quotaUsed, quotaTotal, syncStatus, syncMessage } = statusBar
  const showQuota = quotaTotal > 0
  const pct = showQuota ? Math.min(100, Math.round((quotaUsed * 100) / quotaTotal)) : 0

  return (
    <footer className="shrink-0 flex items-center gap-3 border-t border-border bg-muted/50 px-4 py-1.5 text-xs text-muted-foreground min-h-8">
      <span
        className="size-3.5 shrink-0 rounded-full"
        style={{ backgroundColor: STATUS_BG[syncStatus] ?? STATUS_BG.off }}
        title={STATUS_TITLES[syncStatus] ?? STATUS_TITLES.off}
        aria-label={STATUS_TITLES[syncStatus]}
      />
      {showQuota && (
        <>
          <div className="w-32 h-2 rounded-full bg-muted overflow-hidden shrink-0">
            <Progress value={pct} className="h-full" />
          </div>
          <span className="shrink-0">{formatBytes(quotaUsed)} / {formatBytes(quotaTotal)}</span>
        </>
      )}
      {!showQuota && <span className="shrink-0">— / —</span>}
      <span className="min-w-0 truncate flex-1">{syncMessage}</span>
    </footer>
  )
}
