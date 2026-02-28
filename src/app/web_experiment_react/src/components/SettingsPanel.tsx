import { useEffect } from 'react'
import { useBridge } from '@/contexts/BridgeContext'
import { useStore } from '@/store/useStore'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Checkbox } from '@/components/ui/checkbox'

export function SettingsPanel() {
  const bridge = useBridge()
  const settingsForm = useStore((s) => s.settingsForm)
  const setSettingsForm = useStore((s) => s.setSettingsForm)

  const activePane = useStore((s) => s.activePane)

  useEffect(() => {
    if (activePane !== 'settings' || !bridge?.getSettings) return
    bridge.getSettings().then((json) => {
      try {
        const data = JSON.parse(json)
        setSettingsForm({
          syncPath: data.syncPath ?? '',
          refreshIntervalSec: data.refreshIntervalSec ?? 60,
          cloudCheckIntervalSec: data.cloudCheckIntervalSec ?? 30,
          maxRetries: data.maxRetries ?? 3,
          hideToTray: !!data.hideToTray,
          closeToTray: !!data.closeToTray,
        })
      } catch {
        // ignore
      }
    })
  }, [activePane, bridge, setSettingsForm])

  const handleBrowse = () => {
    if (!bridge?.chooseSyncFolder) return
    bridge.chooseSyncFolder(settingsForm.syncPath).then((path) => {
      if (path) setSettingsForm({ syncPath: path })
    })
  }

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault()
    if (!bridge?.saveSettings) return
    bridge.saveSettings(JSON.stringify(settingsForm))
  }

  return (
    <div className="flex-1 overflow-auto">
      <div className="p-6 max-w-xl">
        <h2 className="text-base font-semibold mb-4">Настройки</h2>
        <form onSubmit={handleSubmit} className="space-y-4">
          <div>
            <Label htmlFor="settings-sync-path">Папка синхронизации</Label>
            <div className="flex gap-2 mt-1">
              <Input
                id="settings-sync-path"
                value={settingsForm.syncPath}
                onChange={(e) => setSettingsForm({ syncPath: e.target.value })}
                placeholder="/path/to/folder"
                className="flex-1"
              />
              <Button type="button" variant="secondary" onClick={handleBrowse}>
                Обзор
              </Button>
            </div>
          </div>
          <div className="grid grid-cols-2 gap-4">
            <div>
              <Label htmlFor="settings-refresh-sec">Интервал обновления (сек)</Label>
              <Input
                id="settings-refresh-sec"
                type="number"
                min={5}
                max={3600}
                value={settingsForm.refreshIntervalSec}
                onChange={(e) => setSettingsForm({ refreshIntervalSec: parseInt(e.target.value, 10) || 60 })}
                className="mt-1"
              />
            </div>
            <div>
              <Label htmlFor="settings-cloud-sec">Проверка облака (сек)</Label>
              <Input
                id="settings-cloud-sec"
                type="number"
                min={5}
                max={3600}
                value={settingsForm.cloudCheckIntervalSec}
                onChange={(e) => setSettingsForm({ cloudCheckIntervalSec: parseInt(e.target.value, 10) || 30 })}
                className="mt-1"
              />
            </div>
          </div>
          <div>
            <Label htmlFor="settings-max-retries">Повторы при ошибке</Label>
            <Input
              id="settings-max-retries"
              type="number"
              min={1}
              max={10}
              value={settingsForm.maxRetries}
              onChange={(e) => setSettingsForm({ maxRetries: parseInt(e.target.value, 10) || 3 })}
              className="mt-1 w-24"
            />
          </div>
          <div className="flex gap-4">
            <label className="flex items-center gap-2 cursor-pointer">
              <Checkbox
                checked={settingsForm.hideToTray}
                onCheckedChange={(v) => setSettingsForm({ hideToTray: !!v })}
              />
              <span className="text-sm">Сворачивать в трей</span>
            </label>
            <label className="flex items-center gap-2 cursor-pointer">
              <Checkbox
                checked={settingsForm.closeToTray}
                onCheckedChange={(v) => setSettingsForm({ closeToTray: !!v })}
              />
              <span className="text-sm">Закрывать в трей</span>
            </label>
          </div>
          <div className="pt-2">
            <Button type="submit">Сохранить</Button>
          </div>
        </form>
      </div>
    </div>
  )
}
