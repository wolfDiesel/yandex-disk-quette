#pragma once

#include <disk_tree/domain/node.hpp>
#include <disk_tree/domain/quota.hpp>
#include <sync/domain/sync_status.hpp>
#include <sync/infrastructure/sync_index.hpp>
#include <QByteArray>
#include <QEvent>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QFileSystemWatcher>
#include <QSet>
#include <QTimer>
#include <QAction>
#include <QWidget>
#include <cstdint>
#include <memory>
#include <vector>

class QStandardItem;

namespace Ui {
class MainContentWidget;
}

namespace ydisquette {

struct CompositionRoot;

class MainContentWidget : public QWidget {
    Q_OBJECT
public:
    explicit MainContentWidget(CompositionRoot& root, QWidget* parent = nullptr);
    ~MainContentWidget() override;

    QByteArray saveState() const;
    void restoreState(const QByteArray& state);

public slots:
    void openSettings();
    void onSyncClicked();
    void onStopSyncTriggered();
    void setStopSyncAction(QAction* action);
    void setSyncAction(QAction* action);

protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onItemExpanded(const QModelIndex& index);
    void onItemChanged(QStandardItem* item);
    void onSyncStatusChanged(sync::SyncStatus status);
    void onTokenExpired();
    void onSyncError(QString message);
    void onSyncProgressMessage(QString message);
    void onTreeSelectionChanged();
    void onContentsContextMenuRequested(const QPoint& pos);
    void onContentsDoubleClicked(const QModelIndex& index);
    void onRefreshTimer();
    void onCloudCheckTimer();
    void onSyncPathChanged(const QString& path);
    void onSyncLocalDebounce();
    void onIndexStateLoaded(sync::IndexState state);
    void onPathsCreatedInCloud(const std::vector<std::string>& cloudPaths);

private:
    void setupSyncWatcher();
    void loadAndDisplay();
    void loadAndDisplay(const std::vector<std::string>& ensureSelectedPaths);
    void appendChildrenToRow(QStandardItem* nameItem, const std::string& path);
    void appendChildrenToRowFromData(QStandardItem* nameItem,
                                     const std::vector<std::shared_ptr<disk_tree::Node>>& children);
    void setContentsFromNodes(const std::vector<std::shared_ptr<disk_tree::Node>>& children);
    void refreshContentsList();
    void refreshQuotaLabel();
    void updateStatusBar(const disk_tree::Quota& q);
    void updateSyncIndicator();
    void updateTreeSizeColumnWidth();
    void navigateTreeToPath(const QString& path);
    static QString formatBytes(int64_t bytes);

    CompositionRoot* root_;
    Ui::MainContentWidget* ui_;
    QStandardItemModel* treeModel_;
    QStandardItemModel* contentsModel_;
    QString currentFolderPath_;
    QTimer* refreshTimer_;
    QTimer* cloudCheckTimer_;
    bool cloudCheckTimerStarted_ = false;
    QFileSystemWatcher* syncWatcher_ = nullptr;
    QTimer* syncLocalDebounceTimer_ = nullptr;
    QSet<QString> syncWatchedPaths_;
    QString syncWatcherRoot_;
    static const int kSyncLocalDebounceMs = 2000;
    sync::SyncStatus syncStatus_{sync::SyncStatus::Idle};
    QString lastSyncError_;
    QString lastSyncProgressMessage_;
    quint64 loadGeneration_ = 0;
    bool skipNextIdleRefresh_ = false;
    QAction* stopSyncAction_ = nullptr;
    QAction* syncAction_ = nullptr;
};

}  // namespace ydisquette
