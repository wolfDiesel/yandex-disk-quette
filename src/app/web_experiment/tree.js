'use strict';
console.warn('tree.js loaded');

function renderTree() {
  const data = window.__TREE_DATA__;
  const root = document.getElementById('tree-root');
  if (!root || !Array.isArray(data)) {
    if (root) { root.textContent = data ? 'No tree data.' : 'Loading…'; root.classList.add('text-muted-foreground'); }
    return;
  }
  root.innerHTML = '';
  root.classList.remove('text-muted-foreground');
  var total = countNodes(data);
  console.log('renderTree: ' + data.length + ' top-level, ' + total + ' total nodes');
  root.appendChild(buildTreeEl(data, true));
}

function countNodes(nodes) {
  var n = 0;
  for (var i = 0; i < nodes.length; i++) {
    n += 1;
    if (nodes[i].children && nodes[i].children.length) n += countNodes(nodes[i].children);
  }
  return n;
}

function buildTreeEl(nodes, defaultExpanded) {
  const ul = document.createElement('ul');
  ul.className = 'list-none m-0 p-0 space-y-0.5';
  for (const n of nodes) {
    const li = document.createElement('li');
    const hasChildren = n.dir && n.children && n.children.length > 0;
    if (n.dir) {
      li.className = 'node-dir';
      li.setAttribute('data-path', n.path || '/');
      if (n.checked) li.classList.add('checked');
      if (!defaultExpanded) li.classList.add('collapsed');
      else li.classList.add('expanded');
    } else {
      li.className = 'node-file';
    }
    const label = document.createElement('span');
    label.className = 'flex items-center gap-2 py-1.5 px-2 rounded-md cursor-default hover:bg-accent min-h-8';
    if (n.dir) label.classList.add('cursor-pointer');
    if (n.dir) {
      var arrow = document.createElement('span');
      arrow.className = 'arrow-toggle';
      arrow.textContent = '▸';
      arrow.setAttribute('aria-hidden', 'true');
      arrow.style.minWidth = '1.25rem';
      arrow.style.minHeight = '1.25rem';
      arrow.style.display = 'inline-flex';
      arrow.style.alignItems = 'center';
      arrow.style.justifyContent = 'center';
      arrow.style.cursor = 'pointer';
      label.appendChild(arrow);
      const cb = document.createElement('input');
      cb.type = 'checkbox';
      cb.className = 'shrink-0 rounded border-input';
      cb.checked = !!n.checked;
      cb.setAttribute('aria-label', n.name || n.path || '');
      cb.addEventListener('click', function (e) {
        e.stopPropagation();
      });
      cb.addEventListener('change', function () {
        if (window.__bridge && typeof window.__bridge.setPathChecked === 'function') {
          window.__bridge.setPathChecked(n.path || '/', cb.checked);
        }
      });
      label.appendChild(cb);
      var folderIconWrap = document.createElement('span');
      folderIconWrap.className = 'tree-folder-icon';
      folderIconWrap.setAttribute('aria-hidden', 'true');
      folderIconWrap.style.cssText = 'position:relative;display:inline-flex;width:16px;height:16px;flex-shrink:0;align-items:center;justify-content:center';
      var closedSvg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
      closedSvg.setAttribute('viewBox', '0 0 24 24');
      closedSvg.setAttribute('fill', 'currentColor');
      closedSvg.setAttribute('width', '16');
      closedSvg.setAttribute('height', '16');
      closedSvg.setAttribute('class', 'tree-icon tree-icon-closed');
      closedSvg.style.cssText = 'position:absolute;inset:0;width:100%;height:100%';
      var closedPath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      closedPath.setAttribute('d', 'M10 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2h-8l-2-2z');
      closedSvg.appendChild(closedPath);
      var openSvg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
      openSvg.setAttribute('viewBox', '0 0 24 24');
      openSvg.setAttribute('fill', 'currentColor');
      openSvg.setAttribute('width', '16');
      openSvg.setAttribute('height', '16');
      openSvg.setAttribute('class', 'tree-icon tree-icon-open');
      openSvg.style.cssText = 'position:absolute;inset:0;width:100%;height:100%';
      var openPath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      openPath.setAttribute('d', 'M20 6h-8l-2-2H4c-1.1 0-2 .9-2 2v12c0 .55.22 1.05.59 1.41.36.37.86.59 1.41.59h16c.55 0 1.05-.22 1.41-.59.37-.36.59-.86.59-1.41V8c0-1.1-.9-2-2-2zm0 12H4V8h16v10z');
      openSvg.appendChild(openPath);
      folderIconWrap.appendChild(closedSvg);
      folderIconWrap.appendChild(openSvg);
      label.appendChild(folderIconWrap);
      label.setAttribute('role', 'button');
      label.setAttribute('tabIndex', '0');
      label.setAttribute('aria-expanded', !li.classList.contains('collapsed'));
      label.addEventListener('keydown', function (e) {
        if (e.key === 'Enter' || e.key === ' ') {
          e.preventDefault();
          li.classList.toggle('collapsed');
          li.classList.toggle('expanded');
          label.setAttribute('aria-expanded', li.classList.contains('expanded'));
        }
      });
    } else {
      const spacer = document.createElement('span');
      spacer.className = 'w-3 shrink-0';
      spacer.setAttribute('aria-hidden', 'true');
      label.appendChild(spacer);
    }
    const name = document.createElement('span');
    name.className = 'min-w-0 truncate flex-1';
    name.textContent = n.name || n.path || '/';
    if (n.checked) name.classList.add('font-medium');
    label.appendChild(name);
    li.appendChild(label);
    if (n.dir) {
      var childWrap = document.createElement('div');
      childWrap.className = 'tree-children';
      if (n.children && n.children.length) childWrap.appendChild(buildTreeEl(n.children, false));
      li.appendChild(childWrap);
    }
    ul.appendChild(li);
  }
  return ul;
}

function escapePathForSelector(path) {
  return (path || '').replace(/\\/g, '\\\\').replace(/"/g, '\\"');
}

function getParentPath(path) {
  if (!path || path === '/') return '';
  var s = path.replace(/\/$/, '');
  var i = s.lastIndexOf('/');
  return i <= 0 ? '/' : s.slice(0, i);
}

function findTreeLiByPath(path) {
  var root = document.getElementById('tree-root');
  if (!root || !path) return null;
  var escaped = escapePathForSelector(path);
  return root.querySelector('li.node-dir[data-path="' + escaped + '"]');
}

function getParentLi(li) {
  var ul = li.parentElement;
  if (!ul || !ul.parentElement || !ul.parentElement.parentElement) return null;
  return ul.parentElement.parentElement;
}

function expandAncestors(li) {
  var parent = getParentLi(li);
  while (parent && parent.classList && parent.classList.contains('node-dir')) {
    if (parent.classList.contains('collapsed')) {
      parent.classList.remove('collapsed');
      parent.classList.add('expanded');
      var lab = parent.querySelector('[role="button"]');
      if (lab) lab.setAttribute('aria-expanded', 'true');
      var wrap = parent.querySelector('.tree-children');
      if (wrap && !wrap.querySelector('ul')) {
        var path = parent.getAttribute('data-path') || '/';
        if (window.__bridge && typeof window.__bridge.requestChildrenForPath === 'function') {
          wrap.innerHTML = '<span class="text-muted-foreground text-xs px-2 py-1">…</span>';
          window.__bridge.requestChildrenForPath(path);
        }
      }
    }
    parent = getParentLi(parent);
  }
}

function removeCurrentFolderArrow() {
  var root = document.getElementById('tree-root');
  if (!root) return;
  root.querySelectorAll('.current-folder-arrow').forEach(function (el) { el.remove(); });
}

function addCurrentFolderArrow(li) {
  var label = li && li.querySelector('[role="button"]');
  if (!label || label.querySelector('.current-folder-arrow')) return;
  var arrow = document.createElement('span');
  arrow.className = 'current-folder-arrow';
  arrow.setAttribute('aria-hidden', 'true');
  label.appendChild(arrow);
}

function syncTreeToPath(path) {
  var root = document.getElementById('tree-root');
  if (!root) return;
  root.querySelectorAll('li.node-dir.selected').forEach(function (el) {
    el.classList.remove('selected');
    var lab = el.querySelector('[role="button"]');
    var arr = lab && lab.querySelector('.current-folder-arrow');
    if (arr) arr.remove();
  });
  var li = findTreeLiByPath(path);
  if (li) {
    expandAncestors(li);
    li.classList.add('selected');
    addCurrentFolderArrow(li);
    li.scrollIntoView({ block: 'nearest', behavior: 'smooth' });
    window.__pendingSyncPath = null;
    return;
  }
  var parentPath = getParentPath(path);
  if (!parentPath && path !== '/') return;
  var parentLi = parentPath ? findTreeLiByPath(parentPath) : root.querySelector('li.node-dir[data-path="/"]');
  if (parentLi && parentLi.classList.contains('collapsed')) {
    parentLi.classList.remove('collapsed');
    parentLi.classList.add('expanded');
    var lab = parentLi.querySelector('[role="button"]');
    if (lab) lab.setAttribute('aria-expanded', 'true');
    var wrap = parentLi.querySelector('.tree-children');
    if (wrap && !wrap.querySelector('ul') && window.__bridge && window.__bridge.requestChildrenForPath) {
      wrap.innerHTML = '<span class="text-muted-foreground text-xs px-2 py-1">…</span>';
      window.__bridge.requestChildrenForPath(parentPath);
    }
    window.__pendingSyncPath = path;
  }
}

window.__syncTreeToPath__ = syncTreeToPath;

window.__injectChildren__ = function (path, children) {
  if (!path || !Array.isArray(children)) return;
  var root = document.getElementById('tree-root');
  if (!root) return;
  var lis = root.querySelectorAll('li.node-dir[data-path]');
  for (var i = 0; i < lis.length; i++) {
    if (lis[i].getAttribute('data-path') === path) {
      var wrap = lis[i].querySelector('.tree-children');
      if (!wrap) return;
      wrap.innerHTML = '';
      var ul = buildTreeEl(children, false);
      wrap.appendChild(ul);
      if (window.__pendingSyncPath) {
        var p = window.__pendingSyncPath;
        window.__pendingSyncPath = null;
        setTimeout(function () { syncTreeToPath(p); }, 0);
      }
      return;
    }
  }
};

function bindTreeClicks() {
  var root = document.getElementById('tree-root');
  if (!root || root._treeClickBound) return;
  root._treeClickBound = true;
  root.addEventListener('click', function (e) {
    if (e.target.closest('input[type="checkbox"]')) return;
    var arrow = e.target.closest('.arrow-toggle');
    if (arrow) {
      e.preventDefault();
      e.stopPropagation();
      var li = arrow.closest('li.node-dir');
      if (!li) return;
      var wrap = li.querySelector('.tree-children');
      if (!wrap) return;
      var wasCollapsed = li.classList.contains('collapsed');
      li.classList.toggle('collapsed');
      li.classList.toggle('expanded');
      var lab = li.querySelector('[role="button"]');
      if (lab) lab.setAttribute('aria-expanded', li.classList.contains('expanded'));
      if (wasCollapsed && li.classList.contains('expanded') && !wrap.querySelector('ul')) {
        var path = li.getAttribute('data-path') || '/';
        if (window.__bridge && typeof window.__bridge.requestChildrenForPath === 'function') {
          wrap.innerHTML = '<span class="text-muted-foreground text-xs px-2 py-1">…</span>';
          window.__bridge.requestChildrenForPath(path);
        }
      }
      return;
    }
    var li = e.target.closest('li.node-dir');
    if (!li) return;
    e.preventDefault();
    e.stopPropagation();
    var path = li.getAttribute('data-path') || '/';
    root.querySelectorAll('li.node-dir.selected').forEach(function (el) {
      el.classList.remove('selected');
      var lab = el.querySelector('[role="button"]');
      var arr = lab && lab.querySelector('.current-folder-arrow');
      if (arr) arr.remove();
    });
    li.classList.add('selected');
    addCurrentFolderArrow(li);
    if (typeof window.__updateBreadcrumbs__ === 'function') window.__updateBreadcrumbs__(path);
    if (window.__bridge && typeof window.__bridge.requestContentsForPath === 'function') {
      window.__bridge.requestContentsForPath(path);
    }
    if (typeof window.__setContentsLoading === 'function') window.__setContentsLoading(true);
    else {
      var cv = document.getElementById('contents-view');
      if (cv) cv.innerHTML = '<p class="text-sm text-muted-foreground">Загрузка…</p>';
    }
  }, true);
  console.log('tree-root click bound (capture)');
}

document.addEventListener('DOMContentLoaded', function () {
  if (typeof qt !== 'undefined' && qt.webChannelTransport) {
    new QWebChannel(qt.webChannelTransport, function (ch) {
      window.__bridge = ch.objects.bridge;
      if (typeof window.__onBridgeReady__ === 'function') window.__onBridgeReady__();
    });
  }
  if (window.__TREE_DATA__) { renderTree(); bindTreeClicks(); }
  var refreshBtn = document.getElementById('refresh-tree');
  if (refreshBtn) {
    refreshBtn.addEventListener('click', function () {
      if (window.__bridge && typeof window.__bridge.getTreeJson === 'function') {
        window.__bridge.getTreeJson().then(function (jsonStr) {
          try {
            window.__TREE_DATA__ = JSON.parse(jsonStr);
            renderTree();
          } catch (err) {
            document.getElementById('tree-root').textContent = 'Invalid tree data';
          }
        });
      } else {
        window.location.reload();
      }
    });
  }
});
