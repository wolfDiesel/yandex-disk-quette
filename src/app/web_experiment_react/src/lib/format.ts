import type { ContentItem } from './bridge-types'

export function formatSize(bytes: number | undefined | null): string {
  if (bytes === undefined || bytes === null) return '—'
  if (bytes < 1024) return `${bytes} B`
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`
  if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`
  return `${(bytes / (1024 * 1024 * 1024)).toFixed(1)} GB`
}

export function itemTooltip(item: ContentItem): string {
  if (item.dir) return item.modified ?? ''
  const parts = [formatSize(item.size)]
  if (item.modified) parts.push(item.modified)
  return parts.join(' · ')
}

const EXT_ICONS: Record<string, string> = {
  pdf: '📕', doc: '📘', docx: '📘', xls: '📗', xlsx: '📗', ppt: '📙', pptx: '📙',
  jpg: '🖼️', jpeg: '🖼️', png: '🖼️', gif: '🖼️', webp: '🖼️', svg: '🖼️',
  mp4: '🎬', mkv: '🎬', avi: '🎬', webm: '🎬', mov: '🎬',
  mp3: '🎵', ogg: '🎵', wav: '🎵', flac: '🎵', m4a: '🎵',
  zip: '📦', '7z': '📦', rar: '📦', tar: '📦', gz: '📦',
  js: '📄', ts: '📄', py: '📄', html: '📄', css: '📄', json: '📄', xml: '📄', md: '📄', txt: '📄',
}

export function iconForItem(item: ContentItem): string {
  if (item.dir) return '📁'
  const name = (item.name ?? '').toLowerCase()
  const i = name.lastIndexOf('.')
  const ext = i >= 0 ? name.slice(i + 1) : ''
  return EXT_ICONS[ext] ?? '📄'
}
