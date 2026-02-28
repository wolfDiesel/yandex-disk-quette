#include "main_content_widget.hpp"
#include "composition_root.hpp"
#include "cloud_path_util.hpp"
#include "json_config.hpp"
#include "settings_dialog.hpp"
#include "settings/domain/app_settings.hpp"
#include <disk_tree/domain/node.hpp>
#include <disk_tree/domain/quota.hpp>
#include <sync/infrastructure/disk_resource_client.hpp>
#include <sync/infrastructure/sync_service.hpp>
#include <sync/infrastructure/sync_index.hpp>
#include <sync/domain/sync_status.hpp>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QIcon>
#include <QMetaObject>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QShowEvent>
#include <QSizePolicy>
#include <QTimer>
#include <QSplitter>
#include <QStyle>
#include <QTreeView>
#include <QUrl>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QFileSystemWatcher>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <algorithm>

#include "ui_main_content.h"

namespace ydisquette {

enum { PathRole = Qt::UserRole, IsDirRole = Qt::UserRole + 1, IsPlaceholderRole = Qt::UserRole + 2 };

static QString utf8(const std::string& s) {
    return QString::fromUtf8(s.data(), static_cast<int>(s.size()));
}

static Qt::CheckState checkStateForPath(const std::string& nodePath,
                                        const std::vector<std::string>& selectedPaths) {
    const std::string norm = ydisquette::normalizeCloudPath(nodePath);
    bool inStore = false;
    for (const std::string& s : selectedPaths) {
        if (ydisquette::normalizeCloudPath(s) == norm) { inStore = true; break; }
    }
    if (inStore) return Qt::Checked;
    bool hasDescendant = false;
    for (const std::string& s : selectedPaths) {
        const std::string sn = ydisquette::normalizeCloudPath(s);
        if (sn == norm) continue;
        if (norm == "/") {
            if (!sn.empty()) { hasDescendant = true; break; }
        } else if (sn.size() > norm.size() && sn.compare(0, norm.size(), norm) == 0 && sn[norm.size()] == '/') {
            hasDescendant = true;
            break;
        }
    }
    return hasDescendant ? Qt::PartiallyChecked : Qt::Unchecked;
}

static QString openTempBase() {
    return QDir::temp().filePath(QStringLiteral("Y.Disquette.open"));
}

static QIcon themeIcon(QWidget* w, std::initializer_list<const char*> themeNames, QStyle::StandardPixmap fallback) {
    for (const char* name : themeNames) {
        QIcon icon = QIcon::fromTheme(QString::fromUtf8(name));
        if (!icon.isNull())
            return icon;
    }
    if (w)
        return w->style()->standardIcon(fallback);
    return QIcon();
}

QString MainContentWidget::formatBytes(int64_t bytes) {
    if (bytes < 1024) return QString::number(bytes) + QStringLiteral(" B");
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024) + QStringLiteral(" KB");
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024 * 1024)) + QStringLiteral(" MB");
    return QString::number(bytes / (1024 * 1024 * 1024)) + QStringLiteral(" GB");
}

static const int kCloudCheckFirstRunDelayMs = 1500;

static const int kInternetCheckIntervalMs = 30000;

MainContentWidget::MainContentWidget(CompositionRoot& root, QWidget* parent)
    : QWidget(parent), root_(&root), ui_(new Ui::MainContentWidget),
      treeModel_(new QStandardItemModel(this)), contentsModel_(new QStandardItemModel(this)),
      refreshTimer_(new QTimer(this)), cloudCheckTimer_(new QTimer(this)),
      syncWatcher_(new QFileSystemWatcher(this)), syncLocalDebounceTimer_(new QTimer(this)) {
    internetCheckTimer_ = new QTimer(this);
    internetCheckNam_ = new QNetworkAccessManager(this);
    syncLocalDebounceTimer_->setSingleShot(true);
    connect(syncWatcher_, &QFileSystemWatcher::directoryChanged, this, &MainContentWidget::onSyncPathChanged);
    connect(syncWatcher_, &QFileSystemWatcher::fileChanged, this, &MainContentWidget::onSyncPathChanged);
    connect(syncLocalDebounceTimer_, &QTimer::timeout, this, &MainContentWidget::onSyncLocalDebounce);
    ui_->setupUi(this);
    ui_->treeView_->setHeaderHidden(false);
    treeModel_->setColumnCount(2);
    treeModel_->setHorizontalHeaderLabels({tr("Name"), tr("Size")});
    ui_->treeView_->setModel(treeModel_);
    ui_->treeView_->setRootIndex(treeModel_->invisibleRootItem()->index());
    ui_->treeView_->setIconSize(QSize(18, 18));
    ui_->treeView_->setAlternatingRowColors(true);
    ui_->treeView_->setIndentation(14);
    ui_->treeView_->setAnimated(true);
    ui_->treeView_->setUniformRowHeights(true);
    ui_->treeView_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui_->treeView_->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui_->treeView_->installEventFilter(this);
    updateTreeSizeColumnWidth();
    contentsModel_->setColumnCount(1);
    ui_->contentsView_->setModel(contentsModel_);
    ui_->contentsView_->setIconSize(QSize(22, 22));
    QSizePolicy sp = ui_->contentsView_->sizePolicy();
    sp.setHorizontalStretch(1);
    ui_->contentsView_->setSizePolicy(sp);
    sp = ui_->treeView_->sizePolicy();
    sp.setHorizontalStretch(0);
    ui_->treeView_->setSizePolicy(sp);
    ui_->mainSplitter_->setSizes({320, 480});
    if (QVBoxLayout* vlay = qobject_cast<QVBoxLayout*>(this->layout())) {
        vlay->setStretchFactor(ui_->mainSplitter_, 1);
    }
    connect(ui_->treeView_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainContentWidget::onTreeSelectionChanged);
    connect(ui_->treeView_, &QTreeView::expanded, this, &MainContentWidget::onItemExpanded);
    connect(treeModel_, &QStandardItemModel::itemChanged, this, &MainContentWidget::onItemChanged);
    connect(ui_->syncBtn_, &QPushButton::clicked, this, &MainContentWidget::onSyncClicked);
    ui_->syncBtn_->hide();
    connect(&root_->syncService(), &sync::SyncService::statusChanged,
            this, &MainContentWidget::onSyncStatusChanged);
    connect(&root_->syncService(), &sync::SyncService::tokenExpired,
            this, &MainContentWidget::onTokenExpired);
    connect(&root_->syncService(), &sync::SyncService::syncError,
            this, &MainContentWidget::onSyncError);
    connect(&root_->syncService(), &sync::SyncService::syncProgressMessage,
            this, &MainContentWidget::onSyncProgressMessage);
    connect(&root_->syncService(), &sync::SyncService::indexStateLoaded,
            this, &MainContentWidget::onIndexStateLoaded);
    connect(&root_->syncService(), &sync::SyncService::pathsCreatedInCloud,
            this, &MainContentWidget::onPathsCreatedInCloud);
    connect(&root_->syncService(), &sync::SyncService::syncThroughput,
            this, &MainContentWidget::onSyncThroughput);
    ui_->contentsView_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_->contentsView_, &QListView::customContextMenuRequested,
            this, &MainContentWidget::onContentsContextMenuRequested);
    connect(ui_->contentsView_, &QListView::doubleClicked,
            this, &MainContentWidget::onContentsDoubleClicked);
    connect(qApp, &QApplication::aboutToQuit, []() {
        QDir(openTempBase()).removeRecursively();
    });
    connect(refreshTimer_, &QTimer::timeout, this, &MainContentWidget::onRefreshTimer);
    int refreshSec = root_->getSettingsUseCase().run().refreshIntervalSec;
    refreshTimer_->start((refreshSec >= 5 && refreshSec <= 3600 ? refreshSec : 60) * 1000);
    connect(cloudCheckTimer_, &QTimer::timeout, this, &MainContentWidget::onCloudCheckTimer);
    connect(internetCheckTimer_, &QTimer::timeout, this, &MainContentWidget::onInternetCheckTimer);
    internetCheckTimer_->start(kInternetCheckIntervalMs);
    ui_->treeLabel_->hide();
    ui_->quotaProgressBar_->setMinimum(0);
    ui_->quotaProgressBar_->setMaximum(100);
    ui_->connectionIndicator_->setStyleSheet(QStringLiteral("background-color: #9e9e9e; border-radius: 7px;"));
}

static void addWatchRecursive(QFileSystemWatcher* w, QSet<QString>* set, const QString& dirPath) {
    if (set->contains(dirPath)) return;
    w->addPath(dirPath);
    set->insert(dirPath);
    QDir dir(dirPath);
    const QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& name : subdirs)
        addWatchRecursive(w, set, dirPath + QLatin1Char('/') + name);
}

void MainContentWidget::setupSyncWatcher() {
    QString syncPath = QString::fromStdString(root_->getSettingsUseCase().run().syncPath).trimmed();
    if (syncPath.isEmpty() || syncPath == syncWatcherRoot_) return;
    if (!syncWatchedPaths_.isEmpty()) {
        syncWatcher_->removePaths(syncWatcher_->directories() + syncWatcher_->files());
        syncWatchedPaths_.clear();
    }
    syncWatcherRoot_ = syncPath;
    if (!QDir(syncPath).exists()) return;
    addWatchRecursive(syncWatcher_, &syncWatchedPaths_, syncPath);
}

static void addNewFilesUnderDir(sync::SyncIndex& idx, const QString& syncRoot, const QString& dirPath, const QString& baseRel) {
    QDir dir(dirPath);
    const QStringList names = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const QString& name : names) {
        QString childPath = dirPath + QLatin1Char('/') + name;
        QString childRel = baseRel.isEmpty() ? name : (baseRel + QLatin1Char('/') + name);
        QFileInfo cfi(childPath);
        if (cfi.isDir()) {
            addNewFilesUnderDir(idx, syncRoot, childPath, childRel);
        } else {
            if (idx.get(syncRoot, childRel).has_value()) continue;
            idx.upsertNew(syncRoot, childRel, cfi.lastModified().toSecsSinceEpoch(), cfi.size());
        }
    }
}

void MainContentWidget::onSyncPathChanged(const QString& path) {
    if (QFileInfo(path).isDir() && !syncWatchedPaths_.contains(path))
        addWatchRecursive(syncWatcher_, &syncWatchedPaths_, path);
    if (!syncWatcherRoot_.isEmpty() && path.startsWith(syncWatcherRoot_)) {
        sync::SyncIndex idx;
        if (idx.open(root_->getSyncIndexDbPath())) {
            QString syncRoot = sync::normalizeSyncRoot(QFileInfo(syncWatcherRoot_).absoluteFilePath());
            QString baseRel = path.mid(syncWatcherRoot_.size());
            while (baseRel.startsWith(QLatin1Char('/'))) baseRel = baseRel.mid(1);
            QFileInfo fi(path);
            if (fi.isFile() && !baseRel.isEmpty()) {
                if (!idx.get(syncRoot, baseRel).has_value())
                    idx.upsertNew(syncRoot, baseRel, fi.lastModified().toSecsSinceEpoch(), fi.size());
            } else if (fi.isDir()) {
                addNewFilesUnderDir(idx, syncRoot, path, baseRel);
            }
            idx.close();
        }
    }
    syncLocalDebounceTimer_->stop();
    syncLocalDebounceTimer_->start(kSyncLocalDebounceMs);
}

void MainContentWidget::onSyncLocalDebounce() {
    if (!online_ || syncStatus_ == sync::SyncStatus::Syncing) return;
    std::vector<std::string> paths = root_->getSelectedPaths();
    auto settings = root_->getSettingsUseCase().run();
    if (paths.empty() || settings.syncPath.empty()) return;
    auto token = root_->tokenProvider().getAccessToken();
    if (!token || token->empty()) return;
    root_->syncService().startSyncLocalToCloud(paths, settings.syncPath, root_->getSyncIndexDbPath(), settings.maxRetries);
}

MainContentWidget::~MainContentWidget() {
    delete ui_;
}

bool MainContentWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == ui_->treeView_ && event->type() == QEvent::Resize)
        updateTreeSizeColumnWidth();
    return QWidget::eventFilter(obj, event);
}

void MainContentWidget::updateTreeSizeColumnWidth() {
    int w = ui_->treeView_->width();
    ui_->treeView_->header()->resizeSection(1, qBound(30, w / 10, 120));
}

void MainContentWidget::appendChildrenToRow(QStandardItem* nameItem, const std::string& path) {
    root_->treeRepository().getChildrenAsync(path, [this, nameItem](std::vector<std::shared_ptr<disk_tree::Node>> children) {
        appendChildrenToRowFromData(nameItem, children);
    });
}

void MainContentWidget::appendChildrenToRowFromData(QStandardItem* nameItem,
    const std::vector<std::shared_ptr<disk_tree::Node>>& children) {
    std::vector<std::string> selectedPaths = root_->getSelectedPaths();
    QIcon dirIcon = themeIcon(this, {"folder", "folder-open", "inode-directory"}, QStyle::SP_DirIcon);
    for (const auto& c : children) {
        if (!c->isDir()) continue;
        QString name = utf8(c->name.empty() ? c->path : c->name);
        QStandardItem* nameCol = new QStandardItem(dirIcon, name);
        nameCol->setData(utf8(c->path), PathRole);
        nameCol->setData(true, IsDirRole);
        nameCol->setEditable(false);
        nameCol->setCheckable(true);
        treeModel_->blockSignals(true);
        nameCol->setCheckState(checkStateForPath(c->path, selectedPaths));
        treeModel_->blockSignals(false);
        nameCol->appendRow({new QStandardItem, new QStandardItem});
        nameCol->child(0, 0)->setData(true, IsPlaceholderRole);
        QStandardItem* sizeCol = new QStandardItem;
        sizeCol->setEditable(false);
        nameItem->appendRow({nameCol, sizeCol});
    }
}

static QJsonObject itemToJson(QStandardItem* nameItem) {
    QJsonObject o;
    o.insert(QStringLiteral("name"), nameItem->text());
    o.insert(QStringLiteral("path"), nameItem->data(PathRole).toString());
    o.insert(QStringLiteral("dir"), nameItem->data(IsDirRole).toBool());
    o.insert(QStringLiteral("checked"), nameItem->checkState() == Qt::Checked);
    QJsonArray arr;
    for (int i = 0; i < nameItem->rowCount(); ++i) {
        QStandardItem* child = nameItem->child(i, 0);
        if (!child || child->data(IsPlaceholderRole).toBool()) continue;
        arr.append(itemToJson(child));
    }
    if (!arr.isEmpty())
        o.insert(QStringLiteral("children"), arr);
    return o;
}

QByteArray MainContentWidget::treeAsJson() const {
    QJsonArray rootArr;
    QStandardItem* root = treeModel_->invisibleRootItem();
    if (root->rowCount() > 0) {
        QStandardItem* first = root->child(0, 0);
        if (first && !first->data(IsPlaceholderRole).toBool())
            rootArr.append(itemToJson(first));
    }
    return QJsonDocument(rootArr).toJson(QJsonDocument::Compact);
}

void MainContentWidget::setCheckStateRecursive(QStandardItem* nameItem, const std::vector<std::string>& paths) {
    if (!nameItem || nameItem->data(IsPlaceholderRole).toBool()) return;
    if (nameItem->isCheckable()) {
        QString path = nameItem->data(PathRole).toString();
        if (!path.isEmpty()) {
            std::string pathStr = ydisquette::normalizeCloudPath(path.toStdString());
            nameItem->setCheckState(checkStateForPath(pathStr, paths));
        }
    }
    for (int i = 0; i < nameItem->rowCount(); ++i) {
        QStandardItem* child = nameItem->child(i, 0);
        if (child)
            setCheckStateRecursive(child, paths);
    }
}

void MainContentWidget::refreshTreeCheckStatesFromStore() {
    std::vector<std::string> paths = root_->getSelectedPaths();
    treeModel_->blockSignals(true);
    QStandardItem* root = treeModel_->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); ++i) {
        QStandardItem* nameItem = root->child(i, 0);
        if (nameItem)
            setCheckStateRecursive(nameItem, paths);
    }
    treeModel_->blockSignals(false);
}

void MainContentWidget::applySelectionChangeSideEffects(const std::string& pathStr, bool nowChecked) {
    if (nowChecked) {
        auto settings = root_->getSettingsUseCase().run();
        auto token = root_->tokenProvider().getAccessToken();
        if (!settings.syncPath.empty() && token && !token->empty()) {
            root_->syncService().startSync({pathStr}, settings.syncPath, root_->getSyncIndexDbPath(), settings.maxRetries);
        }
    } else {
        std::string syncPath = root_->getSettingsUseCase().run().syncPath;
        QString indexPath = root_->getSyncIndexDbPath();
        if (!syncPath.empty() && !indexPath.isEmpty()) {
            QString syncPathQt = QString::fromStdString(syncPath).trimmed();
            QString syncRoot = sync::normalizeSyncRoot(QFileInfo(syncPathQt).absoluteFilePath());
            std::string pathForRel = pathStr;
            if (pathForRel.size() >= 5 && pathForRel.compare(0, 5, "disk:") == 0)
                pathForRel = pathForRel.substr(5);
            QString rel = QString::fromStdString(pathForRel);
            while (rel.startsWith(QLatin1Char('/')))
                rel = rel.mid(1);
            if (!rel.isEmpty()) {
                sync::SyncIndex idx;
                if (idx.open(QFileInfo(indexPath).absoluteFilePath()) && idx.beginTransaction()) {
                    idx.removePrefix(syncRoot, rel);
                    idx.commit();
                    idx.close();
                }
            }
        }
    }
    JsonConfig c = JsonConfig::load();
    c.selectedNodePaths.clear();
    for (const std::string& p : root_->getSelectedPaths())
        c.selectedNodePaths.append(QString::fromStdString(ydisquette::normalizeCloudPath(p)));
    JsonConfig::save(c);
}

void MainContentWidget::setPathChecked(const QString& path, bool checked) {
    if (path.isEmpty()) return;
    std::string pathStr = ydisquette::normalizeCloudPath(path.toStdString());
    std::vector<std::string> paths = root_->getSelectedPaths();
    bool inStore = std::find(paths.begin(), paths.end(), pathStr) != paths.end();
    if (checked == inStore) return;
    root_->toggleNodeSelectionUseCase().run(pathStr);
    refreshTreeCheckStatesFromStore();
    applySelectionChangeSideEffects(pathStr, checked);
    updateSyncIndicator();
}

static QJsonArray nodesToJsonArray(const std::vector<std::shared_ptr<disk_tree::Node>>& nodes,
                                   const std::vector<std::string>& selectedPaths) {
    QJsonArray arr;
    for (const auto& c : nodes) {
        if (!c->isDir()) continue;
        QJsonObject o;
        o.insert(QStringLiteral("name"), utf8(c->name.empty() ? c->path : c->name));
        o.insert(QStringLiteral("path"), utf8(c->path));
        o.insert(QStringLiteral("dir"), true);
        bool checked = std::find(selectedPaths.begin(), selectedPaths.end(), ydisquette::normalizeCloudPath(c->path)) != selectedPaths.end();
        o.insert(QStringLiteral("checked"), checked);
        arr.append(o);
    }
    return arr;
}

static QJsonArray contentsToJsonArray(const std::vector<std::shared_ptr<disk_tree::Node>>& nodes,
                                      const std::vector<std::string>& selectedPaths) {
    QJsonArray arr;
    for (const auto& c : nodes) {
        QJsonObject o;
        o.insert(QStringLiteral("name"), utf8(c->name.empty() ? c->path : c->name));
        o.insert(QStringLiteral("path"), utf8(c->path));
        o.insert(QStringLiteral("dir"), c->isDir());
        if (c->isDir()) {
            bool checked = std::find(selectedPaths.begin(), selectedPaths.end(), ydisquette::normalizeCloudPath(c->path)) != selectedPaths.end();
            o.insert(QStringLiteral("checked"), checked);
        } else {
            o.insert(QStringLiteral("size"), static_cast<qint64>(c->size));
            o.insert(QStringLiteral("modified"), utf8(c->modified));
        }
        arr.append(o);
    }
    return arr;
}

void MainContentWidget::requestChildrenForPath(const QString& path) {
    if (path.isEmpty()) return;
    std::string pathStr = path.toStdString();
    if (pathStr == "disk:/" || pathStr == "disk:") pathStr = "/";
    const std::string pathForApi = pathStr;
    root_->treeRepository().getChildrenAsync(pathForApi, [this, path](std::vector<std::shared_ptr<disk_tree::Node>> children) {
        std::vector<std::string> sel = root_->getSelectedPaths();
        QJsonArray arr = nodesToJsonArray(children, sel);
        QByteArray json = QJsonDocument(arr).toJson(QJsonDocument::Compact);
        emit childrenForPathLoaded(path, json);
    });
}

void MainContentWidget::requestContentsForPath(const QString& path) {
    if (path.isEmpty()) return;
    std::string pathStr = path.toStdString();
    if (pathStr == "disk:/" || pathStr == "disk:") pathStr = "/";
    const std::string pathForApi = pathStr;
    root_->treeRepository().getChildrenAsync(pathForApi, [this, path](std::vector<std::shared_ptr<disk_tree::Node>> children) {
        std::vector<std::string> sel = root_->getSelectedPaths();
        QJsonArray arr = contentsToJsonArray(children, sel);
        QByteArray json = QJsonDocument(arr).toJson(QJsonDocument::Compact);
        emit contentsForPathLoaded(path, json);
    });
}

void MainContentWidget::loadAndDisplay() {
    loadAndDisplay({});
}

void MainContentWidget::loadAndDisplay(const std::vector<std::string>& ensureSelectedPaths) {
    if (!root_) return;
    bool hasToken = root_->tokenProvider().getAccessToken().has_value();
    if (hasToken)
        ui_->loginHintLabel_->setText(tr("Вы вошли в аккаунт."));
    else
        ui_->loginHintLabel_->setText(tr("To sign in, please use the built-in login window (your browser will open automatically if needed)."));
    ui_->loginHintLabel_->setVisible(true);

    ++loadGeneration_;
    const quint64 gen = loadGeneration_;
    std::vector<std::string> ensureNormalized;
    for (const std::string& p : ensureSelectedPaths)
        ensureNormalized.push_back(ydisquette::normalizeCloudPath(p));
    auto rootNode = root_->loadTreeUseCase().run();
    treeModel_->removeRows(0, treeModel_->rowCount());
    if (rootNode) {
        QString rootName = rootNode->path == "/" ? tr("Disk") : utf8(rootNode->path);
        QIcon rootIcon = themeIcon(this, {"drive-harddisk", "drive-removable-media", "computer"}, QStyle::SP_DriveHDIcon);
        QStandardItem* nameCol = new QStandardItem(rootIcon, rootName);
        nameCol->setData(utf8(rootNode->path), PathRole);
        nameCol->setData(true, IsDirRole);
        nameCol->setEditable(false);
        nameCol->setCheckable(true);
        {
            std::vector<std::string> sel = root_->getSelectedPaths();
            treeModel_->blockSignals(true);
            nameCol->setCheckState(checkStateForPath(rootNode->path, sel));
            treeModel_->blockSignals(false);
        }
        QStandardItem* sizeCol = new QStandardItem;
        sizeCol->setEditable(false);
        nameCol->appendRow({new QStandardItem, new QStandardItem});
        nameCol->child(0, 0)->setData(true, IsPlaceholderRole);
        treeModel_->appendRow({nameCol, sizeCol});
        root_->treeRepository().getChildrenAsync(rootNode->path, [this, nameCol, gen, ensureNormalized](std::vector<std::shared_ptr<disk_tree::Node>> children) {
            if (loadGeneration_ != gen) return;
            nameCol->removeRow(0);
            appendChildrenToRowFromData(nameCol, children);
            if (!ensureNormalized.empty()) {
                QSet<QString> ensureSet;
                for (const std::string& p : ensureNormalized)
                    ensureSet.insert(QString::fromStdString(p));
                treeModel_->blockSignals(true);
                for (int i = 0; i < nameCol->rowCount(); ++i) {
                    QStandardItem* item = nameCol->child(i, 0);
                    if (!item || !item->isCheckable()) continue;
                    std::string pathNorm = ydisquette::normalizeCloudPath(item->data(PathRole).toString().toStdString());
                    if (ensureSet.contains(QString::fromStdString(pathNorm)))
                        item->setCheckState(Qt::Checked);
                }
                treeModel_->blockSignals(false);
            }
            ui_->treeView_->expandToDepth(0);
            emit treeRefreshed();
        });
    }
    root_->getQuotaUseCase().runAsync([this](disk_tree::Quota q) { updateStatusBar(q); });
    root_->syncService().startLoadIndexState(root_->getSyncIndexDbPath());
}

void MainContentWidget::onIndexStateLoaded(sync::IndexState) {
}

void MainContentWidget::onPathsCreatedInCloud(const std::vector<std::string>& cloudPaths) {
    if (cloudPaths.empty()) return;
    std::vector<std::string> sel = root_->getSelectedPaths();
    for (const std::string& p : cloudPaths) {
        std::string n = ydisquette::normalizeCloudPath(p);
        if (std::find(sel.begin(), sel.end(), n) == sel.end())
            sel.push_back(n);
    }
    root_->setSelectedPaths(std::move(sel));
    JsonConfig c = JsonConfig::load();
    c.selectedNodePaths.clear();
    for (const std::string& p : root_->getSelectedPaths())
        c.selectedNodePaths.append(QString::fromStdString(ydisquette::normalizeCloudPath(p)));
    JsonConfig::save(c);
    QTimer::singleShot(100, this, [this, cloudPaths]() {
        loadAndDisplay(cloudPaths);
        root_->syncService().startLoadIndexState(root_->getSyncIndexDbPath());
        skipNextIdleRefresh_ = true;
        std::vector<std::string> paths = root_->getSelectedPaths();
        auto settings = root_->getSettingsUseCase().run();
        if (!paths.empty() && !settings.syncPath.empty()) {
            auto token = root_->tokenProvider().getAccessToken();
            if (token && !token->empty())
                root_->syncService().startSyncLocalToCloud(paths, settings.syncPath, root_->getSyncIndexDbPath(), settings.maxRetries);
        }
    });
}

void MainContentWidget::onItemExpanded(const QModelIndex& index) {
    QStandardItem* nameItem = treeModel_->itemFromIndex(index.sibling(index.row(), 0));
    if (!nameItem || nameItem->rowCount() != 1) return;
    QStandardItem* firstChild = nameItem->child(0, 0);
    if (!firstChild || !firstChild->data(IsPlaceholderRole).toBool()) return;
    QString path = nameItem->data(PathRole).toString();
    if (path.isEmpty()) return;
    nameItem->removeRow(0);
    const quint64 gen = loadGeneration_;
    root_->treeRepository().getChildrenAsync(path.toStdString(), [this, nameItem, gen](std::vector<std::shared_ptr<disk_tree::Node>> children) {
        if (loadGeneration_ != gen) return;
        appendChildrenToRowFromData(nameItem, children);
    });
}

void MainContentWidget::onItemChanged(QStandardItem* item) {
    if (!item || item->column() != 0 || !item->isCheckable()) return;
    QString path = item->data(PathRole).toString();
    if (path.isEmpty()) return;
    std::string pathStr = ydisquette::normalizeCloudPath(path.toStdString());
    std::vector<std::string> paths = root_->getSelectedPaths();
    bool inStore = std::find(paths.begin(), paths.end(), pathStr) != paths.end();
    Qt::CheckState state = item->checkState();
    if (state == Qt::PartiallyChecked) {
        treeModel_->blockSignals(true);
        item->setCheckState(checkStateForPath(pathStr, paths));
        treeModel_->blockSignals(false);
    } else {
        bool checked = (state == Qt::Checked);
        if (inStore != checked) {
            root_->toggleNodeSelectionUseCase().run(pathStr);
            paths = root_->getSelectedPaths();
            if (checked) {
                auto settings = root_->getSettingsUseCase().run();
                auto token = root_->tokenProvider().getAccessToken();
                if (!settings.syncPath.empty() && token && !token->empty()) {
                    root_->syncService().startSync({pathStr}, settings.syncPath, root_->getSyncIndexDbPath(), settings.maxRetries);
                }
            } else {
                std::string syncPath = root_->getSettingsUseCase().run().syncPath;
                QString indexPath = root_->getSyncIndexDbPath();
                if (!syncPath.empty() && !indexPath.isEmpty()) {
                    QString syncPathQt = QString::fromStdString(syncPath).trimmed();
                    QString syncRoot = sync::normalizeSyncRoot(QFileInfo(syncPathQt).absoluteFilePath());
                    std::string pathForRel = pathStr;
                    if (pathForRel.size() >= 5 && pathForRel.compare(0, 5, "disk:") == 0)
                        pathForRel = pathForRel.substr(5);
                    QString rel = QString::fromStdString(pathForRel);
                    while (rel.startsWith(QLatin1Char('/')))
                        rel = rel.mid(1);
                    if (!rel.isEmpty()) {
                        sync::SyncIndex idx;
                        if (idx.open(QFileInfo(indexPath).absoluteFilePath()) && idx.beginTransaction()) {
                            idx.removePrefix(syncRoot, rel);
                            idx.commit();
                            idx.close();
                        }
                    }
                }
            }
            JsonConfig c = JsonConfig::load();
            c.selectedNodePaths.clear();
            for (const std::string& p : root_->getSelectedPaths())
                c.selectedNodePaths.append(QString::fromStdString(ydisquette::normalizeCloudPath(p)));
            JsonConfig::save(c);
        } else {
            paths = root_->getSelectedPaths();
        }
        treeModel_->blockSignals(true);
        item->setCheckState(checkStateForPath(pathStr, paths));
        treeModel_->blockSignals(false);
    }
    updateSyncIndicator();
}

void MainContentWidget::onStopSyncTriggered() {
    root_->syncService().stopSync();
}

void MainContentWidget::setStopSyncAction(QAction* action) {
    stopSyncAction_ = action;
    if (stopSyncAction_)
        stopSyncAction_->setEnabled(syncStatus_ == sync::SyncStatus::Syncing);
}

void MainContentWidget::setSyncAction(QAction* action) {
    syncAction_ = action;
    if (syncAction_)
        syncAction_->setEnabled(syncStatus_ != sync::SyncStatus::Syncing);
}

void MainContentWidget::openSettings() {
    SettingsDialog dlg(*root_, this);
    dlg.exec();
    auto s = root_->getSettingsUseCase().run();
    int refreshSec = (s.refreshIntervalSec >= 5 && s.refreshIntervalSec <= 3600) ? s.refreshIntervalSec : 60;
    int cloudSec = (s.cloudCheckIntervalSec >= 5 && s.cloudCheckIntervalSec <= 3600) ? s.cloudCheckIntervalSec : 30;
    refreshTimer_->setInterval(refreshSec * 1000);
    if (cloudCheckTimerStarted_)
        cloudCheckTimer_->setInterval(cloudSec * 1000);
    if (!s.syncPath.empty()) {
        loadAndDisplay();
        setupSyncWatcher();
        if (!cloudCheckTimerStarted_) {
            cloudCheckTimerStarted_ = true;
            QTimer::singleShot(kCloudCheckFirstRunDelayMs, this, &MainContentWidget::onCloudCheckTimer);
            cloudCheckTimer_->start((cloudSec >= 5 && cloudSec <= 3600 ? cloudSec : 30) * 1000);
        }
    }
}

static constexpr int kRefreshIntervalMin = 5;
static constexpr int kRefreshIntervalMax = 3600;

QString MainContentWidget::getSettingsJson() const {
    if (!root_) return QStringLiteral("{}");
    auto s = root_->getSettingsUseCase().run();
    QJsonObject o;
    o.insert(QStringLiteral("syncPath"), QString::fromStdString(s.syncPath));
    o.insert(QStringLiteral("maxRetries"), s.maxRetries);
    o.insert(QStringLiteral("cloudCheckIntervalSec"), s.cloudCheckIntervalSec);
    o.insert(QStringLiteral("refreshIntervalSec"), s.refreshIntervalSec);
    o.insert(QStringLiteral("hideToTray"), s.hideToTray);
    o.insert(QStringLiteral("closeToTray"), s.closeToTray);
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

bool MainContentWidget::saveSettingsFromJson(const QString& json) {
    if (!root_) return false;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (!doc.isObject()) return false;
    QJsonObject o = doc.object();
    settings::AppSettings s = root_->getSettingsUseCase().run();
    if (o.contains(QStringLiteral("syncPath")))
        s.syncPath = o.value(QStringLiteral("syncPath")).toString().trimmed().toStdString();
    if (o.contains(QStringLiteral("maxRetries")))
        s.maxRetries = o.value(QStringLiteral("maxRetries")).toInt(3);
    if (o.contains(QStringLiteral("cloudCheckIntervalSec")))
        s.cloudCheckIntervalSec = std::clamp(o.value(QStringLiteral("cloudCheckIntervalSec")).toInt(30), kRefreshIntervalMin, kRefreshIntervalMax);
    if (o.contains(QStringLiteral("refreshIntervalSec")))
        s.refreshIntervalSec = std::clamp(o.value(QStringLiteral("refreshIntervalSec")).toInt(60), kRefreshIntervalMin, kRefreshIntervalMax);
    if (o.contains(QStringLiteral("hideToTray")))
        s.hideToTray = o.value(QStringLiteral("hideToTray")).toBool(true);
    if (o.contains(QStringLiteral("closeToTray")))
        s.closeToTray = o.value(QStringLiteral("closeToTray")).toBool(true);
    root_->saveSettingsUseCase().run(s);
    JsonConfig c = JsonConfig::load();
    c.syncFolder = QString::fromStdString(s.syncPath);
    c.cloudCheckIntervalSec = s.cloudCheckIntervalSec;
    c.refreshIntervalSec = s.refreshIntervalSec;
    c.hideToTray = s.hideToTray;
    c.closeToTray = s.closeToTray;
    if (!JsonConfig::save(c)) return false;
    int refreshSec = (s.refreshIntervalSec >= kRefreshIntervalMin && s.refreshIntervalSec <= kRefreshIntervalMax) ? s.refreshIntervalSec : 60;
    int cloudSec = (s.cloudCheckIntervalSec >= kRefreshIntervalMin && s.cloudCheckIntervalSec <= kRefreshIntervalMax) ? s.cloudCheckIntervalSec : 30;
    refreshTimer_->setInterval(refreshSec * 1000);
    if (cloudCheckTimerStarted_)
        cloudCheckTimer_->setInterval(cloudSec * 1000);
    if (!s.syncPath.empty()) {
        loadAndDisplay();
        setupSyncWatcher();
        if (!cloudCheckTimerStarted_) {
            cloudCheckTimerStarted_ = true;
            QTimer::singleShot(kCloudCheckFirstRunDelayMs, this, &MainContentWidget::onCloudCheckTimer);
            cloudCheckTimer_->start((cloudSec >= kRefreshIntervalMin && cloudSec <= kRefreshIntervalMax ? cloudSec : 30) * 1000);
        }
    }
    return true;
}

QString MainContentWidget::getLayoutStateJson() const {
    JsonConfig c = JsonConfig::load();
    QJsonObject o;
    o.insert(QStringLiteral("sidebarCollapsed"), c.webExperimentSidebarCollapsed);
    o.insert(QStringLiteral("treeWidth"), c.webExperimentTreeWidth);
    if (!c.webExperimentTheme.isEmpty())
        o.insert(QStringLiteral("theme"), c.webExperimentTheme);
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

bool MainContentWidget::saveLayoutStateFromJson(const QString& json) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (!doc.isObject()) return false;
    QJsonObject o = doc.object();
    JsonConfig c = JsonConfig::load();
    if (o.contains(QStringLiteral("sidebarCollapsed")))
        c.webExperimentSidebarCollapsed = o.value(QStringLiteral("sidebarCollapsed")).toBool(false);
    if (o.contains(QStringLiteral("treeWidth"))) {
        int tw = o.value(QStringLiteral("treeWidth")).toInt(280);
        c.webExperimentTreeWidth = (tw >= 160 && tw <= 600) ? tw : 280;
    }
    if (o.contains(QStringLiteral("theme"))) {
        QString t = o.value(QStringLiteral("theme")).toString();
        if (t == QStringLiteral("light") || t == QStringLiteral("dark") || t == QStringLiteral("system"))
            c.webExperimentTheme = t;
    }
    return JsonConfig::save(c);
}

QString MainContentWidget::chooseSyncFolder(const QString& startPath) {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose sync folder"), startPath.isEmpty() ? QDir::homePath() : startPath);
    return dir.isEmpty() ? QString() : dir;
}

void MainContentWidget::onSyncClicked() {
    if (!online_) return;
    std::vector<std::string> paths = root_->getSelectedPaths();
    auto settings = root_->getSettingsUseCase().run();
    if (paths.empty()) {
        QMessageBox::information(this, tr("Synchronize"),
            tr("Select at least one folder to sync (check the box next to the folder)."));
        return;
    }
    if (settings.syncPath.empty()) {
        QMessageBox::information(this, tr("Synchronize"),
            tr("Choose a sync folder in Settings first."));
        return;
    }
    QString syncPathQt = QString::fromStdString(settings.syncPath).trimmed();
    if (!QFileInfo(syncPathQt).isAbsolute()) {
        QMessageBox::warning(this, tr("Synchronize"), tr("Sync folder path must be absolute."));
        return;
    }
    for (const std::string& p : paths) {
        if (!ydisquette::isValidCloudPath(p)) {
            QMessageBox::warning(this, tr("Synchronize"), tr("Invalid cloud path: %1").arg(QString::fromStdString(p)));
            return;
        }
    }
    auto token = root_->tokenProvider().getAccessToken();
    if (!token || token->empty()) {
        QMessageBox::warning(this, tr("Synchronize"),
            tr("Please sign in first (restart the app and sign in if needed)."));
        return;
    }
    root_->syncService().startSync(paths, settings.syncPath, root_->getSyncIndexDbPath(), settings.maxRetries);
}

void MainContentWidget::onSyncStatusChanged(sync::SyncStatus status) {
    bool wasSyncing = (syncStatus_ == sync::SyncStatus::Syncing);
    syncStatus_ = status;
    if (status == sync::SyncStatus::Idle)
        lastSyncSpeed_ = 0;
    if (status != sync::SyncStatus::Syncing)
        lastSyncProgressMessage_.clear();
    updateSyncIndicator();
    if (status == sync::SyncStatus::Syncing)
        lastSyncError_.clear();
    if (status == sync::SyncStatus::Idle && wasSyncing) {
        if (skipNextIdleRefresh_) {
            skipNextIdleRefresh_ = false;
        } else {
            QTimer::singleShot(100, this, [this]() {
                loadAndDisplay();
                root_->syncService().startLoadIndexState(root_->getSyncIndexDbPath());
            });
        }
    }
    if (status == sync::SyncStatus::Error) {
        QString text = tr("Sync failed. Token may have expired or the request was rejected — try signing in again.");
        if (!lastSyncError_.isEmpty())
            text += QStringLiteral("\n\n") + lastSyncError_;
        QMessageBox::warning(this, tr("Synchronize"), text);
    }
}

void MainContentWidget::onTokenExpired() {
    if (root_->refreshToken()) {
        std::vector<std::string> paths = root_->getSelectedPaths();
        auto settings = root_->getSettingsUseCase().run();
        if (!paths.empty() && !settings.syncPath.empty()) {
            root_->syncService().startSync(paths, settings.syncPath,
                root_->getSyncIndexDbPath(), settings.maxRetries);
        }
    } else {
        QMessageBox::warning(this, tr("Synchronize"),
            tr("Could not refresh token. Please sign in again (restart the app if needed)."));
    }
}

void MainContentWidget::onSyncError(QString message) {
    lastSyncError_ = message;
}

void MainContentWidget::onSyncProgressMessage(QString message) {
    lastSyncProgressMessage_ = message;
    updateSyncIndicator();
}

void MainContentWidget::onTreeSelectionChanged() {
    QModelIndexList sel = ui_->treeView_->selectionModel()->selectedRows(0);
    contentsModel_->removeRows(0, contentsModel_->rowCount());
    currentFolderPath_.clear();
    if (sel.isEmpty()) return;
    QModelIndex idx = sel.front();
    QStandardItem* nameItem = treeModel_->itemFromIndex(idx);
    if (!nameItem) return;
    QString path = nameItem->data(PathRole).toString();
    if (path.isEmpty() || !nameItem->data(IsDirRole).toBool()) return;
    currentFolderPath_ = path;
    root_->treeRepository().getChildrenAsync(path.toStdString(), [this](std::vector<std::shared_ptr<disk_tree::Node>> children) {
        setContentsFromNodes(children);
    });
}

void MainContentWidget::setContentsFromNodes(const std::vector<std::shared_ptr<disk_tree::Node>>& children) {
    contentsModel_->removeRows(0, contentsModel_->rowCount());
    QIcon dirIcon = themeIcon(this, {"folder", "folder-open", "inode-directory"}, QStyle::SP_DirIcon);
    QIcon fileIcon = themeIcon(this, {"document", "text-x-generic"}, QStyle::SP_FileIcon);
    for (const auto& c : children) {
        QString name = utf8(c->name.empty() ? c->path : c->name);
        QStandardItem* item = new QStandardItem(c->isDir() ? dirIcon : fileIcon, name);
        item->setEditable(false);
        item->setData(utf8(c->path), PathRole);
        item->setData(c->isDir(), IsDirRole);
        QString tip = c->isFile() ? tr("%1 · %2").arg(formatBytes(c->size)).arg(utf8(c->modified))
                                  : utf8(c->modified);
        if (!tip.isEmpty()) item->setToolTip(tip);
        contentsModel_->appendRow(item);
    }
}

void MainContentWidget::refreshContentsList() {
    if (currentFolderPath_.isEmpty()) return;
    root_->treeRepository().getChildrenAsync(currentFolderPath_.toStdString(), [this](std::vector<std::shared_ptr<disk_tree::Node>> children) {
        setContentsFromNodes(children);
    });
}

void MainContentWidget::refreshQuotaLabel() {
    root_->getQuotaUseCase().runAsync([this](disk_tree::Quota q) { updateStatusBar(q); });
}

void MainContentWidget::onRefreshTimer() {
    loadAndDisplay();
}

void MainContentWidget::onInternetCheckTimer() {
    if (internetCheckReply_) return;
    QNetworkRequest req(QUrl(QStringLiteral("https://ya.ru")));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    internetCheckReply_ = internetCheckNam_->get(req);
    connect(internetCheckReply_, &QNetworkReply::finished, this, &MainContentWidget::onInternetCheckFinished);
}

void MainContentWidget::onInternetCheckFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    bool ok = (reply->error() == QNetworkReply::NoError) || (statusCode > 0);
    bool wasOffline = !online_;
    online_ = ok;
    if (wasOffline && online_)
        tryResumeSyncAfterOnline();
    else if (!online_ && syncStatus_ == sync::SyncStatus::Syncing)
        root_->syncService().stopSync();
    internetCheckReply_.clear();
    reply->deleteLater();
    updateSyncIndicator();
    emitStatusBarUpdate();
}

void MainContentWidget::tryResumeSyncAfterOnline() {
    if (syncStatus_ == sync::SyncStatus::Syncing) return;
    std::vector<std::string> paths = root_->getSelectedPaths();
    auto settings = root_->getSettingsUseCase().run();
    if (paths.empty() || settings.syncPath.empty()) return;
    auto token = root_->tokenProvider().getAccessToken();
    if (!token || token->empty()) return;
    root_->syncService().startSyncLocalToCloud(paths, settings.syncPath, root_->getSyncIndexDbPath(), settings.maxRetries);
}

void MainContentWidget::onSyncThroughput(qint64 bytesPerSecond) {
    lastSyncSpeed_ = static_cast<double>(bytesPerSecond);
    emitStatusBarUpdate();
}

void MainContentWidget::onCloudCheckTimer() {
    if (!online_) return;
    setupSyncWatcher();
    if (syncStatus_ == sync::SyncStatus::Syncing)
        return;
    std::vector<std::string> paths = root_->getSelectedPaths();
    auto settings = root_->getSettingsUseCase().run();
    if (paths.empty() || settings.syncPath.empty())
        return;
    auto token = root_->tokenProvider().getAccessToken();
    if (!token || token->empty())
        return;
    root_->syncService().startSyncLocalToCloud(paths, settings.syncPath, root_->getSyncIndexDbPath(), settings.maxRetries);
}

void MainContentWidget::navigateTreeToPath(const QString& path) {
    if (path.isEmpty() || path == QStringLiteral("/")) {
        QModelIndex rootIdx = treeModel_->index(0, 0);
        ui_->treeView_->selectionModel()->select(rootIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ui_->treeView_->scrollTo(rootIdx);
        return;
    }
    QStringList segments = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QStandardItem* parent = treeModel_->item(0, 0);
    if (!parent) return;
    QModelIndex parentIdx = parent->index();
    for (const QString& segment : segments) {
        QString targetPath = parent->data(PathRole).toString();
        if (targetPath != QStringLiteral("/")) targetPath += QLatin1Char('/');
        targetPath += segment;
        if (!ui_->treeView_->isExpanded(parentIdx))
            ui_->treeView_->expand(parentIdx);
        QStandardItem* child = nullptr;
        for (int r = 0, n = parent->rowCount(); r < n; ++r) {
            QStandardItem* c = parent->child(r, 0);
            if (c && c->data(PathRole).toString() == targetPath) {
                child = c;
                break;
            }
        }
        if (!child) return;
        parent = child;
        parentIdx = parent->index();
    }
    ui_->treeView_->expand(parentIdx);
    ui_->treeView_->selectionModel()->select(parentIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ui_->treeView_->scrollTo(parentIdx);
}

void MainContentWidget::onContentsDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    QStandardItem* item = contentsModel_->itemFromIndex(index);
    if (!item) return;
    QString path = item->data(PathRole).toString();
    if (path.isEmpty()) return;
    bool isDir = item->data(IsDirRole).toBool();
    if (isDir) {
        currentFolderPath_ = path;
        refreshContentsList();
        navigateTreeToPath(path);
        return;
    }
    QString fileName = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
    QString base = openTempBase();
    if (!QDir().exists(base))
        QDir().mkpath(base);
    QString localPath = base + QLatin1Char('/') + fileName;
    root_->downloadFileUseCase().runAsync(path.toStdString(), localPath, [this, localPath](sync::DiskResourceResult r) {
        if (r.success)
            QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
        else
            QMessageBox::warning(this, tr("Open error"), r.errorMessage.isEmpty() ? tr("Failed to download file.") : r.errorMessage);
    });
}

void MainContentWidget::updateStatusBar(const disk_tree::Quota& q) {
    updateSyncIndicator();
    if (q.totalSpace > 0) {
        lastQuotaUsed_ = q.usedSpace;
        lastQuotaTotal_ = q.totalSpace;
        int pct = static_cast<int>((q.usedSpace * 100) / q.totalSpace);
        ui_->quotaProgressBar_->setValue(pct);
        ui_->quotaStatusLabel_->setText(tr("%1 / %2").arg(formatBytes(q.usedSpace)).arg(formatBytes(q.totalSpace)));
    } else {
        lastQuotaUsed_ = 0;
        lastQuotaTotal_ = 0;
        ui_->quotaProgressBar_->setValue(0);
        ui_->quotaStatusLabel_->setText(tr("— / —"));
    }
    emitStatusBarUpdate();
}

void MainContentWidget::updateSyncIndicator() {
    bool syncOn = !root_->getSelectedPaths().empty();
    bool syncing = (syncStatus_ == sync::SyncStatus::Syncing);
    bool error = (syncStatus_ == sync::SyncStatus::Error);
    QString color = !online_ ? QStringLiteral("#757575")
                 : error ? QStringLiteral("#f44336")
                 : syncing ? QStringLiteral("#ff9800")
                 : syncOn ? QStringLiteral("#4caf50")
                 : QStringLiteral("#9e9e9e");
    ui_->connectionIndicator_->setStyleSheet(
        QStringLiteral("background-color: %1; border-radius: 7px;").arg(color));
    QString tip = !online_ ? tr("No internet")
                 : error ? tr("Sync error")
                 : syncing ? tr("Syncing…")
                 : syncOn ? tr("Sync on")
                 : tr("Sync off");
    ui_->connectionIndicator_->setToolTip(tip);
    if (syncing && !lastSyncProgressMessage_.isEmpty())
        ui_->syncProgressLabel_->setText(lastSyncProgressMessage_);
    else
        ui_->syncProgressLabel_->clear();
    if (stopSyncAction_)
        stopSyncAction_->setEnabled(syncing);
    if (syncAction_)
        syncAction_->setEnabled(!syncing);
    emitStatusBarUpdate();
}

void MainContentWidget::requestStatusBarUpdate() {
    emitStatusBarUpdate();
}

void MainContentWidget::emitStatusBarUpdate() {
    QString status;
    if (syncStatus_ == sync::SyncStatus::Syncing) status = QStringLiteral("syncing");
    else if (syncStatus_ == sync::SyncStatus::Error) status = QStringLiteral("error");
    else if (!root_->getSelectedPaths().empty()) status = QStringLiteral("on");
    else status = QStringLiteral("off");
    QJsonObject o;
    o.insert(QStringLiteral("quotaUsed"), static_cast<qint64>(lastQuotaUsed_));
    o.insert(QStringLiteral("quotaTotal"), static_cast<qint64>(lastQuotaTotal_));
    o.insert(QStringLiteral("syncStatus"), status);
    o.insert(QStringLiteral("syncMessage"), lastSyncProgressMessage_);
    o.insert(QStringLiteral("online"), online_);
    o.insert(QStringLiteral("speed"), static_cast<double>(lastSyncSpeed_));
    emit statusBarUpdated(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
}

void MainContentWidget::onContentsContextMenuRequested(const QPoint& pos) {
    QModelIndex idx = ui_->contentsView_->indexAt(pos);
    if (!idx.isValid()) return;
    QStandardItem* item = contentsModel_->itemFromIndex(idx);
    if (!item) return;
    QString path = item->data(PathRole).toString();
    if (path.isEmpty()) return;
    bool isDir = item->data(IsDirRole).toBool();

    QMenu menu(this);
    QAction* downloadAction = menu.addAction(tr("Download"));
    downloadAction->setEnabled(!isDir);
    downloadAction->setData(path);
    QAction* deleteAction = menu.addAction(tr("Delete from Disk"));
    deleteAction->setData(path);

    QAction* chosen = menu.exec(ui_->contentsView_->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == downloadAction) {
        QString localPath;
        QString syncPath = QString::fromStdString(root_->getSettingsUseCase().run().syncPath);
        if (!syncPath.isEmpty()) {
            int lastSlash = path.lastIndexOf(QLatin1Char('/'));
            QString fileName = lastSlash >= 0 ? path.mid(lastSlash + 1) : path;
            localPath = syncPath + QLatin1Char('/') + fileName;
        }
        if (localPath.isEmpty())
            localPath = QFileDialog::getSaveFileName(this, tr("Save file"), path);
        if (localPath.isEmpty()) return;
        root_->downloadFileUseCase().runAsync(path.toStdString(), localPath, [this](sync::DiskResourceResult r) {
            if (!r.success)
                QMessageBox::warning(this, tr("Download error"), r.errorMessage.isEmpty() ? tr("Failed to download file.") : r.errorMessage);
        });
    } else if (chosen == deleteAction) {
        if (QMessageBox::question(this, tr("Delete from Disk"),
                tr("Delete «%1» from Yandex.Disk?").arg(path),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
        root_->deleteResourceUseCase().runAsync(path.toStdString(), [this](sync::DiskResourceResult r) {
            if (!r.success)
                QMessageBox::warning(this, tr("Delete error"), r.errorMessage.isEmpty() ? tr("Failed to delete.") : r.errorMessage);
            else {
                refreshContentsList();
                refreshQuotaLabel();
            }
        });
    }
}

void MainContentWidget::openFileFromCloud(const QString& cloudPath) {
    if (cloudPath.isEmpty() || !root_) {
        emit downloadFinished(false, tr("Invalid path."));
        return;
    }
    int lastSlash = cloudPath.lastIndexOf(QLatin1Char('/'));
    QString fileName = lastSlash >= 0 ? cloudPath.mid(lastSlash + 1) : cloudPath;
    QString base = openTempBase();
    if (!QDir().exists(base))
        QDir().mkpath(base);
    QString localPath = base + QLatin1Char('/') + fileName;
    root_->downloadFileUseCase().runAsync(cloudPath.toStdString(), localPath, [this, localPath](sync::DiskResourceResult r) {
        if (r.success) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
            emit downloadFinished(true, QString());
        } else {
            emit downloadFinished(false, r.errorMessage.isEmpty() ? tr("Failed to download file.") : r.errorMessage);
        }
    });
}

void MainContentWidget::downloadFile(const QString& cloudPath) {
    if (cloudPath.isEmpty() || !root_) {
        emit downloadFinished(false, tr("Invalid path."));
        return;
    }
    QString localPath;
    QString syncPath = QString::fromStdString(root_->getSettingsUseCase().run().syncPath).trimmed();
    if (!syncPath.isEmpty()) {
        int lastSlash = cloudPath.lastIndexOf(QLatin1Char('/'));
        QString fileName = lastSlash >= 0 ? cloudPath.mid(lastSlash + 1) : cloudPath;
        localPath = syncPath + QLatin1Char('/') + fileName;
    }
    if (localPath.isEmpty())
        localPath = QFileDialog::getSaveFileName(this, tr("Save file"), cloudPath);
    if (localPath.isEmpty()) {
        emit downloadFinished(false, QString());
        return;
    }
    root_->downloadFileUseCase().runAsync(cloudPath.toStdString(), localPath, [this](sync::DiskResourceResult r) {
        emit downloadFinished(r.success, r.errorMessage.isEmpty() ? tr("Failed to download file.") : r.errorMessage);
    });
}

void MainContentWidget::deleteFromDisk(const QString& cloudPath) {
    if (cloudPath.isEmpty() || !root_) {
        emit deleteFinished(false, tr("Invalid path."));
        return;
    }
    root_->deleteResourceUseCase().runAsync(cloudPath.toStdString(), [this](sync::DiskResourceResult r) {
        if (r.success) {
            refreshQuotaLabel();
        }
        emit deleteFinished(r.success, r.errorMessage.isEmpty() ? tr("Failed to delete.") : r.errorMessage);
    });
}

QByteArray MainContentWidget::saveState() const {
    return ui_->mainSplitter_->saveState();
}

void MainContentWidget::restoreState(const QByteArray& state) {
    if (!state.isEmpty())
        ui_->mainSplitter_->restoreState(state);
}

void MainContentWidget::ensureInitialLoad() {
    std::string syncPath = root_->getSettingsUseCase().run().syncPath;
    if (syncPath.empty()) {
        if (!syncFolderPromptShown_) {
            syncFolderPromptShown_ = true;
            QTimer::singleShot(400, this, [this]() { openSettings(); });
        }
        return;
    }
    QTimer::singleShot(0, this, [this]() { loadAndDisplay(); });
    QTimer::singleShot(500, this, [this]() { setupSyncWatcher(); });
    if (!cloudCheckTimerStarted_) {
        cloudCheckTimerStarted_ = true;
        QTimer::singleShot(kCloudCheckFirstRunDelayMs, this, &MainContentWidget::onCloudCheckTimer);
        int cloudSec = root_->getSettingsUseCase().run().cloudCheckIntervalSec;
        cloudCheckTimer_->start((cloudSec >= 5 && cloudSec <= 3600 ? cloudSec : 30) * 1000);
    }
}

void MainContentWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (property("skipFirstShow").toBool()) {
        setProperty("skipFirstShow", false);
        return;
    }
    ensureInitialLoad();
}

}  // namespace ydisquette
