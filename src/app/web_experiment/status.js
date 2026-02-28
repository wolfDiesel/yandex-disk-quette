'use strict';

function formatBytes(bytes) {
  if (bytes === undefined || bytes === null) return '—';
  if (bytes < 1024) return bytes + ' B';
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
  if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
  return (bytes / (1024 * 1024 * 1024)).toFixed(1) + ' GB';
}

var STATUS_TITLES = {
  off: 'Синхронизация выключена',
  on: 'Синхронизация включена',
  syncing: 'Синхронизация…',
  error: 'Ошибка синхронизации'
};

var STATUS_BG = {
  off: '#9e9e9e',
  on: '#4caf50',
  syncing: '#ff9800',
  error: '#f44336'
};

window.__updateStatusBar__ = function (jsonStr) {
  try {
    var data = typeof jsonStr === 'string' ? JSON.parse(jsonStr) : jsonStr;
  } catch (e) {
    return;
  }
  var indicator = document.getElementById('status-indicator');
  var quotaBarWrap = document.getElementById('status-quota-bar');
  var quotaFill = document.getElementById('status-quota-fill');
  var quotaText = document.getElementById('status-quota-text');
  var syncMsg = document.getElementById('status-sync-msg');
  if (!indicator || !quotaText) return;

  var status = data.syncStatus || 'off';
  indicator.title = STATUS_TITLES[status] || STATUS_TITLES.off;
  indicator.style.backgroundColor = STATUS_BG[status] || STATUS_BG.off;

  var used = data.quotaUsed;
  var total = data.quotaTotal;
  if (total > 0) {
    quotaBarWrap.classList.remove('hidden');
    var pct = Math.min(100, Math.round((used * 100) / total));
    quotaFill.style.width = pct + '%';
    quotaText.textContent = formatBytes(used) + ' / ' + formatBytes(total);
  } else {
    quotaBarWrap.classList.add('hidden');
    quotaText.textContent = '— / —';
  }

  if (syncMsg) syncMsg.textContent = data.syncMessage || '';
  if (typeof window.updateSyncButtonsFromStatus === 'function') window.updateSyncButtonsFromStatus(status);
};
