import { useEffect, useState } from 'react'
import { Toaster as SonnerToaster, toast } from 'sonner'
import { BridgeProvider } from '@/contexts/BridgeContext'
import { useStore } from '@/store/useStore'
import { Layout } from '@/components/Layout'

function ToastFromStore() {
  const toastMessage = useStore((s) => s.toastMessage)
  const showToast = useStore((s) => s.showToast)

  useEffect(() => {
    if (!toastMessage) return
    toast.error(toastMessage, { duration: 5000 })
    const t = setTimeout(() => showToast(null), 5000)
    return () => clearTimeout(t)
  }, [toastMessage, showToast])

  return null
}

function useResolvedTheme() {
  const theme = useStore((s) => s.theme)
  const [systemDark, setSystemDark] = useState(() =>
    typeof window !== 'undefined' ? window.matchMedia('(prefers-color-scheme: dark)').matches : false
  )
  useEffect(() => {
    const m = window.matchMedia('(prefers-color-scheme: dark)')
    const on = () => setSystemDark(m.matches)
    m.addEventListener('change', on)
    return () => m.removeEventListener('change', on)
  }, [])
  return theme === 'system' ? (systemDark ? 'dark' : 'light') : theme
}

function ThemeSync({ children }: { children: React.ReactNode }) {
  const resolved = useResolvedTheme()
  useEffect(() => {
    document.documentElement.classList.toggle('dark', resolved === 'dark')
  }, [resolved])
  return <>{children}</>
}

function AppContent() {
  const resolvedTheme = useResolvedTheme()
  return (
    <ThemeSync>
      <ToastFromStore />
      <div className="flex-1 min-h-0 flex flex-col overflow-hidden">
        <Layout />
      </div>
      <SonnerToaster theme={resolvedTheme} />
    </ThemeSync>
  )
}

export default function App() {
  return (
    <BridgeProvider>
      <AppContent />
    </BridgeProvider>
  )
}
