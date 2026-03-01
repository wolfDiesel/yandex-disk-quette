import { useBridge } from '@/contexts/BridgeContext'
import { useStore } from '@/store/useStore'
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from '@/components/ui/dropdown-menu'
import { Button } from '@/components/ui/button'
import { Settings, Play, Square, LogOut, HelpCircle } from 'lucide-react'
import { useState, useCallback } from 'react'

export function MenuBar() {
  const bridge = useBridge()
  const syncStatus = useStore((s) => s.statusBar.syncStatus)
  const syncing = syncStatus === 'syncing'
  const [aboutOpen, setAboutOpen] = useState(false)
  const [aboutInfo, setAboutInfo] = useState<{ appName: string; author: string; githubUrl: string } | null>(null)

  const openAbout = useCallback(() => {
    if (!bridge?.getAboutInfo) return
    bridge.getAboutInfo().then((json) => {
      try {
        const data = JSON.parse(json)
        setAboutInfo({
          appName: data.appName ?? 'Y.Disquette',
          author: data.author ?? '',
          githubUrl: data.githubUrl ?? '',
        })
        setAboutOpen(true)
      } catch {
        setAboutInfo({ appName: 'Y.Disquette', author: '', githubUrl: '' })
        setAboutOpen(true)
      }
    })
  }, [bridge])

  return (
    <>
      <div className="shrink-0 flex items-center gap-0.5 border-b border-border bg-muted/30 px-2 py-0.5">
        <DropdownMenu>
          <DropdownMenuTrigger asChild>
            <Button variant="ghost" size="sm" className="text-sm font-medium h-8 px-2">
              Файл
            </Button>
          </DropdownMenuTrigger>
          <DropdownMenuContent align="start">
            <DropdownMenuItem onClick={() => bridge?.openSettings?.()}>
              <Settings className="size-4 mr-2" />
              Настройки…
            </DropdownMenuItem>
            <DropdownMenuItem onClick={() => bridge?.startSync?.()} disabled={syncing}>
              <Play className="size-4 mr-2" />
              Синхронизировать
            </DropdownMenuItem>
            <DropdownMenuItem onClick={() => bridge?.stopSync?.()} disabled={!syncing}>
              <Square className="size-4 mr-2" />
              Остановить
            </DropdownMenuItem>
            <DropdownMenuSeparator />
            <DropdownMenuItem onClick={() => bridge?.quitApplication?.()}>
              <LogOut className="size-4 mr-2" />
              Выход
            </DropdownMenuItem>
          </DropdownMenuContent>
        </DropdownMenu>
        <DropdownMenu>
          <DropdownMenuTrigger asChild>
            <Button variant="ghost" size="sm" className="text-sm font-medium h-8 px-2">
              Справка
            </Button>
          </DropdownMenuTrigger>
          <DropdownMenuContent align="start">
            <DropdownMenuItem onClick={openAbout}>
              <HelpCircle className="size-4 mr-2" />
              О программе
            </DropdownMenuItem>
          </DropdownMenuContent>
        </DropdownMenu>
      </div>
      {aboutOpen && aboutInfo && (
        <AboutDialog
          appName={aboutInfo.appName}
          author={aboutInfo.author}
          githubUrl={aboutInfo.githubUrl}
          onClose={() => { setAboutOpen(false); setAboutInfo(null) }}
        />
      )}
    </>
  )
}

function AboutDialog({
  appName,
  author,
  githubUrl,
  onClose,
}: {
  appName: string
  author: string
  githubUrl: string
  onClose: () => void
}) {
  return (
    <div
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/50"
      onClick={onClose}
      role="dialog"
      aria-modal="true"
      aria-labelledby="about-title"
    >
      <div
        className="bg-background border border-border rounded-lg shadow-lg p-6 max-w-sm w-full mx-4"
        onClick={(e) => e.stopPropagation()}
      >
        <h2 id="about-title" className="text-lg font-semibold mb-4">
          {appName}
        </h2>
        {author && (
          <p className="text-sm text-muted-foreground mb-1">
            Автор: {author}
          </p>
        )}
        {githubUrl && (
          <a
            href={githubUrl}
            target="_blank"
            rel="noopener noreferrer"
            className="text-sm text-primary hover:underline block mb-4"
          >
            {githubUrl}
          </a>
        )}
        <div className="flex justify-end">
          <Button variant="default" size="sm" onClick={onClose}>
            OK
          </Button>
        </div>
      </div>
    </div>
  )
}
