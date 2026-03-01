'use strict';

var VIEW_MODES = ['list', 'table', 'icons'];
var EXT_ICONS = {
  pdf: '📕', doc: '📘', docx: '📘', xls: '📗', xlsx: '📗', ppt: '📙', pptx: '📙',
  jpg: '🖼️', jpeg: '🖼️', png: '🖼️', gif: '🖼️', webp: '🖼️', svg: '🖼️',
  mp4: '🎬', mkv: '🎬', avi: '🎬', webm: '🎬', mov: '🎬',
  mp3: '🎵', ogg: '🎵', wav: '🎵', flac: '🎵', m4a: '🎵',
  zip: '📦', '7z': '📦', rar: '📦', tar: '📦', gz: '📦',
  js: '📄', ts: '📄', py: '📄', html: '📄', css: '📄', json: '📄', xml: '📄', md: '📄', txt: '📄'
};

function iconForItem(item) {
  if (item.dir) return '📁';
  var name = (item.name || '').toLowerCase();
  var i = name.lastIndexOf('.');
  var ext = i >= 0 ? name.slice(i + 1) : '';
  return EXT_ICONS[ext] || '📄';
}

function formatSize(bytes) {
  if (bytes === undefined || bytes === null) return '—';
  if (bytes < 1024) return bytes + ' B';
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
  if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
  return (bytes / (1024 * 1024 * 1024)).toFixed(1) + ' GB';
}

function escapeAttr(s) {
  return String(s || '').replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

function pathToSegments(path) {
  if (!path || path === '/') return [];
  return path.replace(/^\/|\/$/g, '').split('/').filter(Boolean);
}

function pathSegmentFullPath(path, index) {
  var segs = pathToSegments(path);
  if (index < 0 || index >= segs.length) return '/';
  return '/' + segs.slice(0, index + 1).join('/');
}

function updateBreadcrumbs(path) {
  var list = document.getElementById('breadcrumbs-list');
  var placeholder = document.getElementById('breadcrumbs-placeholder');
  if (!list) return;
  list.innerHTML = '';
  var segs = pathToSegments(path);
  if (segs.length === 0 && path !== '/') {
    var ph = document.createElement('li');
    ph.className = 'text-muted-foreground truncate';
    ph.id = 'breadcrumbs-placeholder';
    ph.textContent = 'Выберите папку';
    list.appendChild(ph);
    return;
  }
  if (placeholder) placeholder.remove();
  if (segs.length === 0) {
    segs = [''];
  }
  var bridge = window.__bridge;
  var syncTree = window.__syncTreeToPath__;
  for (var i = 0; i < segs.length; i++) {
    if (i > 0) {
      var sep = document.createElement('li');
      sep.className = 'shrink-0 text-muted-foreground/70 select-none';
      sep.setAttribute('aria-hidden', 'true');
      sep.textContent = '/';
      list.appendChild(sep);
    }
    var fullPath = path === '/' && segs.length === 1 ? '/' : pathSegmentFullPath(path, i);
    var isLast = i === segs.length - 1;
    var li = document.createElement('li');
    li.className = 'min-w-0 shrink' + (isLast ? '' : ' max-w-[8rem]');
    var btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'rounded px-1.5 py-0.5 text-left min-w-0 truncate max-w-full hover:bg-accent focus:outline-none focus:ring-2 focus:ring-ring focus:ring-offset-1';
    btn.textContent = segs[i] || 'Корень';
    btn.title = fullPath;
    if (!isLast) {
      btn.classList.add('text-foreground');
      btn.addEventListener('click', function (p) {
        return function () {
          if (bridge && bridge.requestContentsForPath) bridge.requestContentsForPath(p);
          if (typeof window.__setContentsLoading === 'function') window.__setContentsLoading(true);
          if (syncTree) syncTree(p);
          updateBreadcrumbs(p);
        };
      }(fullPath));
    } else {
      btn.classList.add('text-foreground', 'font-medium');
      btn.disabled = true;
      btn.classList.add('cursor-default');
    }
    li.appendChild(btn);
    list.appendChild(li);
  }
}

window.__updateBreadcrumbs__ = updateBreadcrumbs;

function itemTooltip(it) {
  if (it.dir) return (it.modified || '').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  var parts = [formatSize(it.size)];
  if (it.modified) parts.push(it.modified.replace(/</g, '&lt;').replace(/>/g, '&gt;'));
  return parts.join(' · ');
}

function renderContentsList(items) {
  var html = '<ul class="list-none m-0 p-0 space-y-0.5">';
  for (var i = 0; i < items.length; i++) {
    var it = items[i];
    var icon = iconForItem(it);
    var name = (it.name || it.path || '').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    var path = escapeAttr(it.path);
    var dir = it.dir ? 'true' : 'false';
    var tip = escapeAttr(itemTooltip(it));
    html += '<li class="content-item flex items-center gap-2 py-2 px-2 rounded-md hover:bg-accent min-h-10 cursor-pointer" data-path="' + path + '" data-dir="' + dir + '" title="' + tip + '">';
    html += '<span class="shrink-0 w-6 text-center" aria-hidden="true">' + icon + '</span>';
    html += '<span class="min-w-0 truncate flex-1">' + name + '</span>';
    if (!it.dir && it.size !== undefined) html += '<span class="text-muted-foreground text-xs shrink-0">' + formatSize(it.size) + '</span>';
    html += '</li>';
  }
  html += '</ul>';
  return html;
}

function renderContentsTable(items) {
  var html = '<table class="w-full text-sm border-collapse"><thead><tr class="border-b border-border text-left text-muted-foreground">';
  html += '<th class="py-2 px-2 w-8"></th><th class="py-2 px-2 font-medium">Имя</th><th class="py-2 px-2 w-24">Размер</th><th class="py-2 px-2 w-32">Изменён</th></tr></thead><tbody>';
  for (var i = 0; i < items.length; i++) {
    var it = items[i];
    var icon = iconForItem(it);
    var name = (it.name || it.path || '').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    var path = escapeAttr(it.path);
    var dir = it.dir ? 'true' : 'false';
    var tip = escapeAttr(itemTooltip(it));
    html += '<tr class="content-item border-b border-border/50 hover:bg-accent/50 cursor-pointer" data-path="' + path + '" data-dir="' + dir + '" title="' + tip + '">';
    html += '<td class="py-1.5 px-2 text-center">' + icon + '</td>';
    html += '<td class="py-1.5 px-2 min-w-0 truncate max-w-md">' + name + '</td>';
    html += '<td class="py-1.5 px-2 text-muted-foreground">' + (it.dir ? '—' : formatSize(it.size)) + '</td>';
    html += '<td class="py-1.5 px-2 text-muted-foreground text-xs">' + (it.modified || '—') + '</td>';
    html += '</tr>';
  }
  html += '</tbody></table>';
  return html;
}

function renderContentsIcons(items) {
  var html = '<div class="grid grid-cols-[repeat(auto-fill,minmax(6rem,1fr))] gap-3">';
  for (var i = 0; i < items.length; i++) {
    var it = items[i];
    var icon = iconForItem(it);
    var name = (it.name || it.path || '').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    var path = escapeAttr(it.path);
    var dir = it.dir ? 'true' : 'false';
    var tip = escapeAttr(itemTooltip(it));
    html += '<div class="content-item flex flex-col items-center p-3 rounded-lg hover:bg-accent cursor-pointer" data-path="' + path + '" data-dir="' + dir + '" title="' + tip + '">';
    html += '<span class="text-3xl mb-1" aria-hidden="true">' + icon + '</span>';
    html += '<span class="text-xs text-center truncate w-full">' + name + '</span>';
    html += '</div>';
  }
  html += '</div>';
  return html;
}

function renderContents(items, viewMode) {
  var container = document.getElementById('contents-view');
  if (!container) return;
  if (!items || items.length === 0) {
    container.innerHTML = '<p class="text-sm text-muted-foreground">Папка пуста.</p>';
    container.setAttribute('data-view', viewMode);
    return;
  }
  if (viewMode === 'table') container.innerHTML = renderContentsTable(items);
  else if (viewMode === 'icons') container.innerHTML = renderContentsIcons(items);
  else container.innerHTML = renderContentsList(items);
  container.setAttribute('data-view', viewMode);
}

window.__setContentsLoading = function (loading) {
  var container = document.getElementById('contents-view');
  if (!container) return;
  if (loading) {
    container.innerHTML = '<p class="text-sm text-muted-foreground">Загрузка…</p>';
  }
};

window.__onContentsLoaded__ = function (path, items) {
  console.log('contents loaded', path, Array.isArray(items) ? items.length : 0);
  window.__setContentsLoading(false);
  var container = document.getElementById('contents-view');
  if (!container) return;
  var viewMode = container.getAttribute('data-view') || 'list';
  if (!VIEW_MODES.includes(viewMode)) viewMode = 'list';
  window.__lastContentsPath = path;
  window.__lastContentsItems = Array.isArray(items) ? items : [];
  renderContents(window.__lastContentsItems, viewMode);
};

function initSidebar() {
  var navItemNav = document.getElementById('nav-item-nav');
  var navItemSettings = document.getElementById('nav-item-settings');
  var paneNav = document.getElementById('pane-nav');
  var paneSettings = document.getElementById('pane-settings');
  if (!navItemNav || !navItemSettings || !paneNav || !paneSettings) return;

  function setActive(id) {
    navItemNav.classList.remove('bg-primary/10', 'border-primary', 'text-foreground');
    navItemNav.classList.add('border-transparent', 'text-muted-foreground');
    navItemSettings.classList.remove('bg-primary/10', 'border-primary', 'text-foreground');
    navItemSettings.classList.add('border-transparent', 'text-muted-foreground');
    if (id === 'nav') {
      navItemNav.classList.add('bg-primary/10', 'border-primary', 'text-foreground');
      navItemNav.classList.remove('border-transparent', 'text-muted-foreground');
      paneNav.classList.remove('hidden');
      paneSettings.classList.add('hidden');
    } else {
      navItemSettings.classList.add('bg-primary/10', 'border-primary', 'text-foreground');
      navItemSettings.classList.remove('border-transparent', 'text-muted-foreground');
      paneNav.classList.add('hidden');
      paneSettings.classList.remove('hidden');
    }
  }

  navItemNav.addEventListener('click', function (e) { e.preventDefault(); setActive('nav'); });
  navItemSettings.addEventListener('click', function (e) {
    e.preventDefault();
    setActive('settings');
    if (typeof window.__loadSettingsForm === 'function') window.__loadSettingsForm();
  });
}

function initViewModes() {
  var container = document.getElementById('contents-view');
  document.querySelectorAll('.view-mode').forEach(function (btn) {
    btn.addEventListener('click', function () {
      var mode = btn.getAttribute('data-view');
      document.querySelectorAll('.view-mode').forEach(function (b) { b.classList.remove('bg-accent', 'text-foreground'); });
      btn.classList.add('bg-accent', 'text-foreground');
      if (container && window.__lastContentsItems) {
        renderContents(window.__lastContentsItems, mode);
      }
      container.setAttribute('data-view', mode);
    });
  });
  var listBtn = document.querySelector('.view-mode[data-view="list"]');
  if (listBtn) listBtn.classList.add('bg-accent', 'text-foreground');
}

function initContentsViewClicks() {
  var container = document.getElementById('contents-view');
  if (!container || container._contentsClickBound) return;
  container._contentsClickBound = true;
  container.addEventListener('click', function (e) {
    var el = e.target.closest('.content-item');
    if (!el) return;
    var path = el.getAttribute('data-path');
    var isDir = el.getAttribute('data-dir') === 'true';
    if (!path) return;
    if (isDir) {
      updateBreadcrumbs(path);
      if (window.__bridge && typeof window.__bridge.requestContentsForPath === 'function') {
        window.__bridge.requestContentsForPath(path);
      }
      if (typeof window.__setContentsLoading === 'function') window.__setContentsLoading(true);
      if (typeof window.__syncTreeToPath__ === 'function') window.__syncTreeToPath__(path);
    }
  });
  container.addEventListener('dblclick', function (e) {
    var el = e.target.closest('.content-item');
    if (!el) return;
    var path = el.getAttribute('data-path');
    var isDir = el.getAttribute('data-dir') === 'true';
    if (!path || isDir) return;
    if (window.__bridge && typeof window.__bridge.openFileFromCloud === 'function') {
      window.__bridge.openFileFromCloud(path);
    }
  });
}

var _ctxPath = null;

function showErrorToast(msg) {
  var el = document.getElementById('error-toast');
  if (!el) return;
  el.textContent = msg || 'Ошибка';
  el.classList.remove('hidden');
  clearTimeout(el._toastTimer);
  el._toastTimer = setTimeout(function () { el.classList.add('hidden'); }, 5000);
}

function initContextMenu() {
  var container = document.getElementById('contents-view');
  var menu = document.getElementById('contents-context-menu');
  var btnDownload = document.getElementById('ctx-download');
  var btnDelete = document.getElementById('ctx-delete');
  if (!container || !menu || !btnDownload || !btnDelete) return;

  container.addEventListener('contextmenu', function (e) {
    e.preventDefault();
    var el = e.target.closest('.content-item');
    if (!el) return;
    _ctxPath = el.getAttribute('data-path');
    var isDir = el.getAttribute('data-dir') === 'true';
    if (!_ctxPath) return;
    btnDownload.disabled = isDir;
    menu.style.left = e.clientX + 'px';
    menu.style.top = e.clientY + 'px';
    menu.classList.remove('hidden');
  });

  function closeMenu() {
    menu.classList.add('hidden');
    _ctxPath = null;
  }

  btnDownload.addEventListener('click', function () {
    if (!_ctxPath || !window.__bridge || !window.__bridge.downloadFile) return;
    window.__bridge.downloadFile(_ctxPath);
    closeMenu();
  });

  btnDelete.addEventListener('click', function () {
    if (!_ctxPath || !window.__bridge || !window.__bridge.deleteFromDisk) return;
    if (!window.confirm('Удалить «' + _ctxPath + '» с Яндex.Диска?')) return;
    window.__bridge.deleteFromDisk(_ctxPath);
    closeMenu();
  });

  document.addEventListener('click', closeMenu);
  document.addEventListener('scroll', closeMenu, true);
}

function initSyncButtons() {
  var btnSync = document.getElementById('btn-sync');
  var btnStop = document.getElementById('btn-stop');
  if (!btnSync || !btnStop) return;
  btnSync.addEventListener('click', function () {
    if (window.__bridge && window.__bridge.startSync) window.__bridge.startSync();
  });
  btnStop.addEventListener('click', function () {
    if (window.__bridge && window.__bridge.stopSync) window.__bridge.stopSync();
  });
}

function initBridgeSignals() {
  var bridge = window.__bridge;
  if (!bridge || !bridge.downloadFinished) return;
  if (bridge._signalsConnected) return;
  bridge._signalsConnected = true;
  bridge.downloadFinished.connect(function (success, errorMessage) {
    if (!success && errorMessage) showErrorToast(errorMessage);
  });
  bridge.deleteFinished.connect(function (success, errorMessage) {
    if (!success && errorMessage) showErrorToast(errorMessage);
    else if (success && window.__lastContentsPath && bridge.requestContentsForPath) {
      bridge.requestContentsForPath(window.__lastContentsPath);
      if (typeof window.__setContentsLoading === 'function') window.__setContentsLoading(true);
    }
  });
}

window.__onBridgeReady__ = function () {
  initSyncButtons();
  initBridgeSignals();
};

function updateSyncButtonsFromStatus(syncStatus) {
  var btnSync = document.getElementById('btn-sync');
  var btnStop = document.getElementById('btn-stop');
  if (!btnSync || !btnStop) return;
  if (syncStatus === 'syncing') {
    btnStop.classList.remove('hidden');
    btnSync.disabled = true;
  } else {
    btnStop.classList.add('hidden');
    btnSync.disabled = false;
  }
}

document.addEventListener('DOMContentLoaded', function () {
  initSidebar();
  initViewModes();
  initContentsViewClicks();
  initContextMenu();
  initSyncButtons();
  if (window.__bridge) initBridgeSignals();
});
