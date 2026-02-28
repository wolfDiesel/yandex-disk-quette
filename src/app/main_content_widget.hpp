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
#include <QNetworkAccessManager>
#include <QPointer>
class QNetworkReply;
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

    QByteArray treeAsJson() const;
    void setPathChecked(const QString& path, bool checked);
    void requestChildrenForPath(const QString& path);
    void requestContentsForPath(const QString& path);
    QString getSettingsJson() const;
    bool saveSettingsFromJson(const QString& json);
    QString getLayoutStateJson() const;
    bool saveLayoutStateFromJson(const QString& json);
    QString chooseSyncFolder(const QString& startPath);

signals:
    void childrenForPathLoaded(const QString& path, const QByteArray& json);
    void contentsForPathLoaded(const QString& path, const QByteArray& json);
    void statusBarUpdated(const QString& json);
    void treeRefreshed();
    void downloadFinished(bool success, const QString& errorMessage);
    void deleteFinished(bool success, const QString& errorMessage);

public slots:
    void requestStatusBarUpdate();
    void ensureInitialLoad();
    void openSettings();
    void onSyncClicked();
    void onStopSyncTriggered();
    void downloadFile(const QString& cloudPath);
    void openFileFromCloud(const QString& cloudPath);
    void deleteFromDisk(const QString& cloudPath);
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
    void onInternetCheckTimer();
    void onInternetCheckFinished();
    void onSyncThroughput(qint64 bytesPerSecond);
    void tryResumeSyncAfterOnline();
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
    void emitStatusBarUpdate();
    void updateTreeSizeColumnWidth();
    void navigateTreeToPath(const QString& path);
    void refreshTreeCheckStatesFromStore();
    void setCheckStateRecursive(QStandardItem* nameItem, const std::vector<std::string>& paths);
    void applySelectionChangeSideEffects(const std::string& pathStr, bool nowChecked);
    static QString formatBytes(int64_t bytes);

    CompositionRoot* root_;
    Ui::MainContentWidget* ui_;
    QStandardItemModel* treeModel_;
    QStandardItemModel* contentsModel_;
    QString currentFolderPath_;
    QTimer* refreshTimer_;
    QTimer* cloudCheckTimer_;
    bool cloudCheckTimerStarted_ = false;
    QTimer* internetCheckTimer_ = nullptr;
    QNetworkAccessManager* internetCheckNam_ = nullptr;
    QPointer<QNetworkReply> internetCheckReply_;
    bool online_ = true;
    double lastSyncSpeed_ = 0;
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
    bool syncFolderPromptShown_ = false;
    int64_t lastQuotaUsed_ = 0;
    int64_t lastQuotaTotal_ = 0;
};

}  // namespace ydisquette
