'use strict';

function getSettingsFormEls() {
  return {
    syncPath: document.getElementById('settings-sync-path'),
    refreshSec: document.getElementById('settings-refresh-sec'),
    cloudSec: document.getElementById('settings-cloud-sec'),
    maxRetries: document.getElementById('settings-max-retries'),
    hideToTray: document.getElementById('settings-hide-tray'),
    closeToTray: document.getElementById('settings-close-tray')
  };
}

function fillForm(data) {
  var els = getSettingsFormEls();
  if (!els.syncPath) return;
  if (data.syncPath !== undefined) els.syncPath.value = data.syncPath || '';
  if (data.refreshIntervalSec !== undefined) els.refreshSec.value = data.refreshIntervalSec;
  if (data.cloudCheckIntervalSec !== undefined) els.cloudSec.value = data.cloudCheckIntervalSec;
  if (data.maxRetries !== undefined) els.maxRetries.value = data.maxRetries;
  if (data.hideToTray !== undefined) els.hideToTray.checked = data.hideToTray;
  if (data.closeToTray !== undefined) els.closeToTray.checked = data.closeToTray;
}

function readForm() {
  var els = getSettingsFormEls();
  if (!els.syncPath) return null;
  return {
    syncPath: els.syncPath.value.trim(),
    refreshIntervalSec: parseInt(els.refreshSec.value, 10) || 60,
    cloudCheckIntervalSec: parseInt(els.cloudSec.value, 10) || 30,
    maxRetries: parseInt(els.maxRetries.value, 10) || 3,
    hideToTray: els.hideToTray.checked,
    closeToTray: els.closeToTray.checked
  };
}

window.__loadSettingsForm = function () {
  if (!window.__bridge || typeof window.__bridge.getSettings !== 'function') return;
  window.__bridge.getSettings().then(function (jsonStr) {
    try {
      var data = JSON.parse(jsonStr);
      fillForm(data);
    } catch (e) {
      console.warn('settings load parse error', e);
    }
  });
};

function initSettingsPane() {
  var form = document.getElementById('settings-form');
  var browseBtn = document.getElementById('settings-browse');
  if (!form || !browseBtn) return;

  browseBtn.addEventListener('click', function () {
    if (!window.__bridge || typeof window.__bridge.chooseSyncFolder !== 'function') return;
    var els = getSettingsFormEls();
    var start = els.syncPath ? els.syncPath.value.trim() : '';
    window.__bridge.chooseSyncFolder(start).then(function (path) {
      if (path && els.syncPath) els.syncPath.value = path;
    });
  });

  form.addEventListener('submit', function (e) {
    e.preventDefault();
    var data = readForm();
    if (!data) return;
    if (!window.__bridge || typeof window.__bridge.saveSettings !== 'function') return;
    window.__bridge.saveSettings(JSON.stringify(data)).then(function (ok) {
      if (ok) console.log('Settings saved');
      else console.warn('Settings save failed');
    });
  });
}

document.addEventListener('DOMContentLoaded', function () {
  initSettingsPane();
});
