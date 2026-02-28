import { useStore } from '@/store/useStore'
import { Sun, Monitor, Moon } from 'lucide-react'
import { cn } from '@/lib/utils'

type Theme = 'light' | 'dark' | 'system'

export function ThemeToggle({ onAfterChange }: { onAfterChange?: () => void }) {
  const theme = useStore((s) => s.theme)
  const setTheme = useStore((s) => s.setTheme)

  const handle = (t: Theme) => {
    setTheme(t)
    onAfterChange?.()
  }

  return (
    <div
      role="group"
      aria-label="Тема оформления"
      className="inline-flex rounded-lg border border-border bg-muted/50 p-0.5"
    >
      <ThemeButton
        value="light"
        current={theme}
        onClick={() => handle('light')}
        icon={<Sun className="size-4" />}
        title="Светлая"
      />
      <ThemeButton
        value="system"
        current={theme}
        onClick={() => handle('system')}
        icon={<Monitor className="size-4" />}
        title="Системная"
      />
      <ThemeButton
        value="dark"
        current={theme}
        onClick={() => handle('dark')}
        icon={<Moon className="size-4" />}
        title="Тёмная"
      />
    </div>
  )
}

function ThemeButton({
  value,
  current,
  onClick,
  icon,
  title,
}: {
  value: Theme
  current: Theme
  onClick: () => void
  icon: React.ReactNode
  title: string
}) {
  const isSelected = current === value
  return (
    <button
      type="button"
      onClick={onClick}
      title={title}
      aria-pressed={isSelected}
      aria-label={title}
      className={cn(
        'rounded-md p-2 transition-colors',
        isSelected
          ? 'bg-background text-foreground shadow-sm'
          : 'text-muted-foreground hover:bg-muted hover:text-foreground'
      )}
    >
      {icon}
    </button>
  )
}
