# Статус миграции веб-эксперимента на React

Проверка по плану `web-experiment-react-migration-plan.md`.

---

## Фаза 0 — сделано полностью

| Пункт | Статус |
|-------|--------|
| 0.1 Директория `web_experiment_react/`, старый `web_experiment/` не тронут | ✅ |
| 0.2 Vite + React + TypeScript (`npm create vite . -- --template react-ts`) | ✅ |
| 0.3 Tailwind (@tailwindcss/vite) + shadcn/ui init (New York, neutral) | ✅ |
| 0.3 Компоненты: Button, ScrollArea, Separator, Input, Label, Checkbox, Progress, Breadcrumb, DropdownMenu, Sonner | ✅ |
| 0.4 Сборка `npm run build` → dist/index.html + dist/assets/* | ✅ |
| 0.4 В index.html: qwebchannel.js до бандла, base: './' в vite.config | ✅ |
| 0.4 QRC + переключение URL в WebExperimentWindow | ✅ Фаза 4 |

---

## Фаза 1 — сделано полностью

| Пункт | Статус |
|-------|--------|
| 1.1 Типы: BridgeApi (все методы + downloadFinished/deleteFinished), TreeNode, ContentItem, StatusBarState, SettingsForm | ✅ |
| 1.1 Расширение Window: __bridge, __TREE_DATA__, qt, QWebChannel, __onBridgeReady__, renderTree, bindTreeClicks, __injectChildren__, __onContentsLoaded__, __updateStatusBar__ | ✅ |
| 1.2 BridgeProvider: QWebChannel, сохранение bridge в state/context, вызов __onBridgeReady__ | ✅ |
| 1.2 Подписка на downloadFinished/deleteFinished (тост при ошибке, обновление контента при успешном удалении) | ✅ |
| 1.3 renderTree() → setTreeData(__TREE_DATA__) | ✅ |
| 1.3 bindTreeClicks → no-op | ✅ |
| 1.3 __injectChildren__(path, children) → injectChildren в store | ✅ |
| 1.3 __onContentsLoaded__(path, items) → setContents (и contentsLoading: false) | ✅ |
| 1.3 __updateStatusBar__(jsonStr) → parse + updateStatusBar | ✅ |

Видимость кнопок Синхр./Стоп по syncStatus будет в UI (Фаза 2), данных в store достаточно.

---

## Store (Фаза 3 — частично)

| Слайс | Статус |
|-------|--------|
| tree (treeData, setTreeData, injectChildren) | ✅ |
| contents (contentsPath, contentsItems, contentsLoading, setContents, setContentsLoading) | ✅ |
| statusBar (updateStatusBar) | ✅ |
| settings (settingsForm, setSettingsForm) — для формы настроек | ✅ добавлено при проверке |
| ui (activePane, viewMode, toastMessage, setActivePane, setViewMode, showToast) | ✅ |

---

## Фаза 2 — сделано

- Layout, сайдбар, переключение Навигация/Настройки, NavPanel (BreadcrumbsBar, Синхр./Стоп, ViewModeButtons, Tree, Contents).
- App рендерит Layout, тосты через Sonner по toastMessage из store.
- Дерево: setContentsPathLoading при клике по папке, ленивые дети, раскрытие пути; корневые узлы раскрываются при setTreeData.
- Контент: три режима, контекстное меню (Скачать/Удалить с Диска), переход по папке через setContentsPathLoading + syncTreeToPath.
- BreadcrumbsBar, StatusBar, SettingsPanel (загрузка при открытии панели).

---

## Фаза 4 — сделано

- CMake target `web_experiment_react_build` (npm install + npm run build в web_experiment_react).
- Генерация web_experiment_react.qrc (generate_react_qrc.cmake) — index.html + assets из dist.
- URL в WebExperimentWindow: `qrc:/web_experiment_react/index.html`.

---

## Сверка по чек-листу плана

**Дерево:** стрелка, чекбокс, иконка папки (закрытая/открытая), имя ✅ | ленивая подгрузка, плейсхолдер «…» ✅ | «Loading…» / «No tree data.» ✅ | чекбокс → setPathChecked(path, checked) ✅ | выделение (selected), зелёная стрелка, scrollIntoView, expandPath(все предки) ✅ | Enter/Space → toggle expanded ✅

**Контент:** три состояния + «Папка пуста.» ✅ | три режима (list/table/icons), state ✅ | иконки EXT_ICONS, тултипы (formatSize · modified) ✅ | клик по папке / двойной клик по файлу ✅ | контекстное меню: Скачать (disabled для папок), Удалить с Диска (confirm), позиция под курсором, закрытие по клику ✅ | data-path, data-dir на элементах ✅

**Breadcrumbs:** «Выберите папку», корень «Корень», последний не кликабелен (font-medium), клик → requestContentsForPath + syncTreeToPath ✅

**Синхронизация и мост:** Синхр./Стоп по syncStatus ✅ | тост при ошибке (5 с), при успешном deleteFinished — requestContentsForPath ✅ | __onBridgeReady__ ✅

**Статус-бар:** индикатор (цвет + title по off/on/syncing/error) ✅ | квота (прогресс + used/total при total>0, «— / —» при 0) ✅ | syncMessage truncate ✅

**Настройки:** все поля, Обзор (chooseSyncFolder), загрузка при открытии панели, сохранение ✅

**Прочее:** Обновить дерево → getTreeJson + setTreeData ✅ | выделение активной панели в сайдбаре ✅ | типы BridgeApi, TreeNode, ContentItem, StatusBarState, SettingsForm, Window ✅

Исправлено при сверке: добавлены data-path/data-dir на элементах контента; опечатка «Яндex» → «Яндекс» в подтверждении удаления.

---

## Фаза 5 — сделано

- Удалены таргет `web_experiment_build` и зависимость от него в `Y.Disquette`.
- Из `app_resources.qrc` убраны все записи `web_experiment/` (оставлен только `app_icon.png`).
- Папка `web_experiment/` не удалялась (можно оставить для истории или удалить после бэкапа).
- Для стабильной сборки: Vite настроен на фиксированные имена артефактов (`assets/index.js`, `assets/index.css`), в `src/app/` добавлен статический `web_experiment_react.qrc`; скрипт `generate_react_qrc.cmake` удалён.

---

## Итог

- **Фазы 0, 1, 2, 4, 5** выполнены. Окно веб-эксперимента использует только React; старый фронт от сборки и QRC отключён.
