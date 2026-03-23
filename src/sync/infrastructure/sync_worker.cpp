#include "sync/infrastructure/sync_worker.hpp"
#include "sync/application/scan_and_fill_index_use_case.hpp"
#include "sync/application/sync_cloud_to_local_use_case.hpp"
#include "sync/application/sync_local_to_cloud_use_case.hpp"
#include "auth/infrastructure/yandex_disk_api_client.hpp"
#include "sync/infrastructure/sync_infrastructure_factory.hpp"
#include "sync/infrastructure/sync_index.hpp"
#include "shared/app_log.hpp"
#include <QDir>
#include <QFileInfo>
#include <QUrlQuery>
#include <functional>
#include <string>
#include <vector>

namespace ydisquette {
namespace sync {

SyncWorker::SyncWorker(QObject* parent) : QObject(parent) {}

void SyncWorker::requestStop() {
    stopRequested_ = true;
}

void SyncWorker::doScanPathAndFillIndex(const std::vector<std::string>& selectedPaths, const std::string& syncPath,
                                        const std::string& accessToken, const QString& indexDbPath) {
    stopRequested_ = false;
    if (selectedPaths.empty() || syncPath.empty() || indexDbPath.isEmpty() || accessToken.empty()) {
        emit scanCompleted();
        return;
    }
    SyncInfrastructure infra;
    SyncInfrastructureFactory::create(accessToken, infra);
    QUrlQuery probeQuery;
    probeQuery.addQueryItem(QStringLiteral("path"), QStringLiteral("/"));
    auth::ApiResponse probe = infra.apiClient->get("/resources", probeQuery);
    if (probe.statusCode == 401) {
        emit tokenExpired();
        emit scanCompleted();
        return;
    }
    QString syncPathQt = QString::fromStdString(syncPath).trimmed();
    QString syncRoot = normalizeSyncRoot(QFileInfo(syncPathQt).absoluteFilePath());
    SyncIndex index;
    QString indexPath = QFileInfo(indexDbPath).absoluteFilePath();
    if (!index.open(indexPath) || !index.beginTransaction()) {
        index.close();
        emit scanCompleted();
        return;
    }
    auto result = ScanAndFillIndexUseCase::run(
        *infra.treeRepo, index, syncRoot, selectedPaths,
        [this]() { return stopRequested_.load(); });
    index.close();
    if (result == ScanAndFillIndexUseCase::Result::IndexError)
        ydisquette::logToFile(QStringLiteral("[Sync] scan index commit failed"));
    emit scanCompleted();
}

void SyncWorker::doSync(const std::vector<std::string>& selectedPaths, const std::string& syncPath,
                        const std::string& accessToken, const QString& indexDbPath, int maxRetries) {
    stopRequested_ = false;
    ydisquette::log(ydisquette::LogLevel::Normal, QStringLiteral("[Sync] sync started paths=") + QString::number(selectedPaths.size())
        + QStringLiteral(" syncPath=") + (syncPath.empty() ? QStringLiteral("(empty)") : QString::fromStdString(syncPath)));

    emit statusChanged(SyncStatus::Syncing);
    emit syncProgressMessage(QStringLiteral("sync started paths=") + QString::number(selectedPaths.size()));

    if (selectedPaths.empty() || syncPath.empty()) {
        emit syncThroughput(0);
        emit statusChanged(SyncStatus::Idle);
        return;
    }
    if (accessToken.empty()) {
        emit syncError(QStringLiteral("No token received by worker (empty)."));
        emit syncThroughput(0);
        emit statusChanged(SyncStatus::Error);
        return;
    }

    QString syncPathQt = QString::fromStdString(syncPath).trimmed();
    QString syncRoot = syncPathQt.isEmpty() ? QString() : normalizeSyncRoot(QFileInfo(syncPathQt).absoluteFilePath());
    QString localRoot = syncRoot.isEmpty() ? QString() : QDir::cleanPath(syncRoot + QLatin1Char('/')) + QLatin1Char('/');

    SyncIndex index;
    QString indexPath = indexDbPath.isEmpty() ? QString() : QFileInfo(indexDbPath).absoluteFilePath();
    bool useIndex = !indexPath.isEmpty() && index.open(indexPath);
    if (useIndex) {
        if (!index.beginTransaction()) {
            useIndex = false;
            index.close();
        } else {
            QStringList toDownload = index.getRelativePathsWithStatus(syncRoot, QString::fromUtf8(FileStatus::TO_DOWNLOAD));
            toDownload.append(index.getRelativePathsWithStatus(syncRoot, QString::fromUtf8(FileStatus::DOWNLOADING)));
            toDownload.removeDuplicates();
            QStringList cloudDeleted = index.getRelativePathsWithStatus(syncRoot, QString::fromUtf8(FileStatus::CLOUD_DELETED));
            if (toDownload.isEmpty() && cloudDeleted.isEmpty()) {
                index.rollback();
                index.close();
                emit syncThroughput(0);
                emit statusChanged(SyncStatus::Idle);
                return;
            }
        }
    }
    if (!indexDbPath.isEmpty() && !useIndex)
        ydisquette::logToFile(QStringLiteral("[Sync] cloud→local index not used: open or beginTransaction failed ") + indexPath);

    SyncInfrastructure infra;
    SyncInfrastructureFactory::create(accessToken, infra);
    QUrlQuery probeQuery;
    probeQuery.addQueryItem(QStringLiteral("path"), QStringLiteral("/"));
    auth::ApiResponse probe = infra.apiClient->get("/resources", probeQuery);
    if (probe.statusCode == 401) {
        if (useIndex) { index.rollback(); index.close(); }
        emit tokenExpired();
        emit syncThroughput(0);
        emit statusChanged(SyncStatus::Error);
        return;
    }

    SyncCloudToLocalCallbacks callbacks;
    callbacks.onProgressMessage = [this](const QString& msg) { emit syncProgressMessage(msg); };
    callbacks.onError = [this](const QString& msg) { emit syncError(msg); };
    callbacks.onThroughput = [this](qint64 bytesPerSec) { emit syncThroughput(bytesPerSec); };

    auto result = SyncCloudToLocalUseCase::run(
        *infra.treeRepo,
        *infra.diskClient,
        useIndex ? &index : nullptr,
        syncRoot,
        localRoot,
        selectedPaths,
        maxRetries,
        [this]() { return stopRequested_.load(); },
        callbacks);

    if (useIndex) {
        if (result != SyncCloudToLocalUseCase::Result::Success)
            index.rollback();
        index.close();
    }
    emit syncThroughput(0);
    if (result == SyncCloudToLocalUseCase::Result::Error)
        emit statusChanged(SyncStatus::Error);
    else
        emit statusChanged(SyncStatus::Idle);
}

void SyncWorker::doSyncLocalToCloud(const std::vector<std::string>& selectedPaths, const std::string& syncPath,
                                    const std::string& accessToken, const QString& indexDbPath, int maxRetries) {
    stopRequested_ = false;
    if (selectedPaths.empty() || syncPath.empty() || accessToken.empty()) {
        return;
    }
    SyncInfrastructure infra;
    SyncInfrastructureFactory::create(accessToken, infra);
    QUrlQuery probeQuery;
    probeQuery.addQueryItem(QStringLiteral("path"), QStringLiteral("/"));
    auth::ApiResponse probe = infra.apiClient->get("/resources", probeQuery);
    if (probe.statusCode == 401) {
        emit tokenExpired();
        emit syncThroughput(0);
        emit statusChanged(SyncStatus::Error);
        return;
    }

    QString syncPathQt = QString::fromStdString(syncPath).trimmed();
    QString syncRoot = syncPathQt.isEmpty() ? QString() : normalizeSyncRoot(QFileInfo(syncPathQt).absoluteFilePath());
    QString localRoot = syncRoot.isEmpty() ? QString() : QDir::cleanPath(syncRoot + QLatin1Char('/')) + QLatin1Char('/');

    SyncIndex index;
    bool useIndex = !indexDbPath.isEmpty() && index.open(indexDbPath);
    if (!indexDbPath.isEmpty() && !useIndex)
        ydisquette::logToFile(QStringLiteral("[Sync] index open FAIL ") + indexDbPath);

    if (useIndex && !index.beginTransaction()) {
        index.close();
        useIndex = false;
    }

    SyncLocalToCloudCallbacks callbacks;
    callbacks.onProgressMessage = [this](const QString& msg) { emit syncProgressMessage(msg); };
    callbacks.onError = [this](const QString& msg) { emit syncError(msg); };
    callbacks.onThroughput = [this](qint64 bytesPerSec) { emit syncThroughput(bytesPerSec); };
    callbacks.onPathsCreatedInCloud = [this](const std::vector<std::string>& paths) { emit pathsCreatedInCloud(paths); };

    auto result = SyncLocalToCloudUseCase::run(
        *infra.treeRepo,
        *infra.diskClient,
        useIndex ? &index : nullptr,
        syncRoot,
        localRoot,
        selectedPaths,
        maxRetries,
        [this]() { return stopRequested_.load(); },
        callbacks);

    if (useIndex) {
        if (result == SyncLocalToCloudUseCase::Result::Success)
            index.commit();
        else
            index.rollback();
        index.close();
    }
    emit syncThroughput(0);
    if (result == SyncLocalToCloudUseCase::Result::Error)
        emit statusChanged(SyncStatus::Error);
    else
        emit statusChanged(SyncStatus::Idle);
    if (result == SyncLocalToCloudUseCase::Result::Success && !stopRequested_)
        emit localToCloudFinished(selectedPaths, syncPath, accessToken);
}

void SyncWorker::loadIndexState(const QString& indexDbPath, const QString& syncRoot) {
    emit indexStateLoaded(readIndexState(indexDbPath, syncRoot));
}

}  // namespace sync
}  // namespace ydisquette
