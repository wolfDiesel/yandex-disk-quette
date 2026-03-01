#include "sync/infrastructure/sync_worker.hpp"
#include "sync/domain/sync_file_status.hpp"
#include "auth/application/itoken_provider.hpp"
#include "auth/infrastructure/yandex_disk_api_client.hpp"
#include "disk_tree/infrastructure/api_tree_repository.hpp"
#include "sync/infrastructure/disk_resource_client.hpp"
#include "sync/infrastructure/sync_index.hpp"
#include "shared/app_log.hpp"
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ydisquette {
namespace sync {

struct TokenHolder : auth::ITokenProvider {
    std::optional<std::string> token;
    std::optional<std::string> getAccessToken() const override { return token; }
};

static QDateTime parseCloudModified(const std::string& modified) {
    QString s = QString::fromStdString(modified).trimmed();
    if (s.isEmpty()) return QDateTime();
    if (!s.endsWith(QLatin1Char('Z')) && s.indexOf(QLatin1Char('+'), 10) < 0
        && (s.length() <= 19 || (s.at(19) != QLatin1Char('+') && s.at(19) != QLatin1Char('-'))))
        s += QStringLiteral("Z");
    QDateTime dt = QDateTime::fromString(s, Qt::ISODate);
    if (dt.isValid())
        dt = dt.toUTC();
    return dt;
}

static bool cloudNewerThanLocal(const disk_tree::Node* node, const QString& localPath) {
    if (!node || node->modified.empty()) return false;
    QDateTime cloudDt = parseCloudModified(node->modified);
    if (!cloudDt.isValid()) return false;
    qint64 cloudSec = cloudDt.toSecsSinceEpoch();
    qint64 localSec = QFileInfo(localPath).lastModified().toSecsSinceEpoch();
    return cloudSec > localSec;
}

static bool localNewerThanCloud(const disk_tree::Node* node, const QString& localPath) {
    if (!node || node->modified.empty()) return true;
    QDateTime cloudDt = parseCloudModified(node->modified);
    if (!cloudDt.isValid()) return true;
    qint64 cloudSec = cloudDt.toSecsSinceEpoch();
    qint64 localSec = QFileInfo(localPath).lastModified().toSecsSinceEpoch();
    return localSec > cloudSec;
}

SyncWorker::SyncWorker(QObject* parent) : QObject(parent) {}

void SyncWorker::requestStop() {
    stopRequested_ = true;
}

void SyncWorker::doSync(const std::vector<std::string>& selectedPaths, const std::string& syncPath,
                        const std::string& accessToken, const QString& indexDbPath, int maxRetries) {
    stopRequested_ = false;
    ydisquette::log(ydisquette::LogLevel::Normal, QStringLiteral("[Sync] sync started paths=") + QString::number(selectedPaths.size())
        + QStringLiteral(" syncPath=") + (syncPath.empty() ? QStringLiteral("(empty)") : QString::fromStdString(syncPath)));

    emit statusChanged(SyncStatus::Syncing);
    QElapsedTimer throughputTimer;
    throughputTimer.start();
    qint64 throughputBytes = 0;

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

    TokenHolder holder;
    holder.token = accessToken;

    QNetworkAccessManager* nam = new QNetworkAccessManager(this);
    auth::YandexDiskApiClient* apiClient =
        new auth::YandexDiskApiClient(holder, nam, this);
    QUrlQuery probeQuery;
    probeQuery.addQueryItem(QStringLiteral("path"), QStringLiteral("/"));
    auth::ApiResponse probe = apiClient->get("/resources", probeQuery);
    if (probe.statusCode == 401) {
        emit tokenExpired();
        emit syncThroughput(0);
        emit statusChanged(SyncStatus::Error);
        return;
    }

    DiskResourceClient client(*apiClient);
    disk_tree::ApiTreeRepository treeRepo(*apiClient);

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
        }
    }
    if (!indexDbPath.isEmpty() && !useIndex)
        ydisquette::logToFile(QStringLiteral("[Sync] cloud→local index not used: open or beginTransaction failed ") + indexPath);

    auto flushIndex = [&]() {
        if (useIndex) { index.commit(); index.beginTransaction(); }
    };
    auto cloudPathToLocal = [](const std::string& cloudPath) -> QString {
        std::string p = cloudPath;
        if (p.size() >= 5 && p.compare(0, 5, "disk:") == 0) p = "/" + p.substr(5);
        else if (!p.empty() && p[0] != '/') p = "/" + p;
        return QString::fromStdString(p);
    };

    auto toRelativePath = [&syncRoot](const QString& localPath) -> QString {
        QString base = syncRoot + QLatin1Char('/');
        if (!localPath.startsWith(base)) return QString();
        return localPath.mid(base.size());
    };

    std::function<bool(const std::string&)> syncFolder = [&](const std::string& cloudPath) {
        if (stopRequested_) return false;
        emit syncProgressMessage(QString::fromStdString(cloudPath));
        std::vector<std::shared_ptr<disk_tree::Node>> children = treeRepo.getChildren(cloudPath);
        QString localDir = localRoot + cloudPathToLocal(cloudPath).mid(1);
        if (!QDir().mkpath(localDir)) {
            emit syncError(QStringLiteral("Failed to create local directory: ") + localDir);
            emit syncThroughput(0);
            emit statusChanged(SyncStatus::Error);
            return false;
        }
        for (const auto& node : children) {
            if (stopRequested_) return false;
            if (!node) continue;
            std::string remotePath = node->path;
            QString localPath = localRoot + cloudPathToLocal(remotePath).mid(1);
            if (node->isDir()) {
                DiskResourceResult cr = client.createFolder(remotePath);
                if (!cr.success && cr.httpStatus != 409) {
                    emit syncError(QStringLiteral("Create folder failed (HTTP %1). Yandex: %2")
                                       .arg(cr.httpStatus).arg(cr.errorMessage));
                    emit syncThroughput(0);
                    emit statusChanged(SyncStatus::Error);
                    return false;
                }
                QDir().mkpath(localPath);
                if (!syncFolder(remotePath)) return false;
            } else {
                QFileInfo fi(localPath);
                bool exists = fi.exists();
                bool empty = fi.size() == 0;
                bool needDownload = !exists
                    || empty
                    || cloudNewerThanLocal(node.get(), localPath);
                if (useIndex) {
                    QString rel = toRelativePath(localPath);
                    while (rel.startsWith(QLatin1Char('/'))) rel = rel.mid(1);
                    if (!rel.isEmpty()) {
                        auto entry = index.get(syncRoot, rel);
                        if (entry && FileStatus::needsDownload(entry->status)) needDownload = true;
                        else if (entry && entry->status == FileStatus::SYNCED && exists
                                 && fi.size() == static_cast<qint64>(node->size)
                                 && entry->size == static_cast<qint64>(node->size))
                            needDownload = false;
                    }
                }
                if (!needDownload) {
                } else {
                    if (useIndex) {
                        QString rel = toRelativePath(localPath);
                        while (rel.startsWith(QLatin1Char('/'))) rel = rel.mid(1);
                        if (!rel.isEmpty()) {
                            auto entry = index.get(syncRoot, rel);
                            if (!entry) index.upsertNew(syncRoot, rel, 0, 0);
                            index.setStatus(syncRoot, rel, QString::fromUtf8(FileStatus::DOWNLOADING), 0);
                            flushIndex();
                        }
                    }
                    emit syncProgressMessage(QStringLiteral("cloud→local ") + QString::fromStdString(remotePath));
                    DiskResourceResult dr = client.downloadFile(remotePath, localPath);
                    if (dr.success) {
                        throughputBytes += static_cast<qint64>(node->size);
                        qint64 ms = throughputTimer.elapsed();
                        emit syncThroughput(ms > 0 ? (throughputBytes * 1000 / ms) : 0);
                    }
                    if (!dr.success) {
                        if (useIndex) {
                            QString rel = toRelativePath(localPath);
                            while (rel.startsWith(QLatin1Char('/'))) rel = rel.mid(1);
                            if (!rel.isEmpty()) {
                                auto entry = index.get(syncRoot, rel);
                                int newRetries = (entry ? entry->retries : 0) + 1;
                                QString newStatus = (newRetries >= maxRetries)
                                    ? QString::fromUtf8(FileStatus::FAILED)
                                    : QString::fromUtf8(FileStatus::DOWNLOADING);
                                index.setStatus(syncRoot, rel, newStatus, 1);
                                ydisquette::logToFile(QStringLiteral("[Sync] download failed ") + rel
                                    + QStringLiteral(" retries=") + QString::number(newRetries));
                                flushIndex();
                            }
                        }
                        emit syncError(QStringLiteral("Download failed (HTTP %1). Yandex: %2")
                                           .arg(dr.httpStatus).arg(dr.errorMessage));
                        continue;
                    }
                }
                if (useIndex) {
                    QString rel = toRelativePath(localPath);
                    while (rel.startsWith(QLatin1Char('/')))
                        rel = rel.mid(1);
                    if (!rel.isEmpty()) {
                        QFileInfo fi2(localPath);
                        if (!index.set(syncRoot, rel, fi2.lastModified().toSecsSinceEpoch(), fi2.size()))
                            ydisquette::logToFile(QStringLiteral("[Sync] index set FAIL (cloud→local) ") + rel);
                        flushIndex();
                    }
                }
            }
        }
        return true;
    };

    for (const std::string& cloudPath : selectedPaths) {
        if (stopRequested_) break;
        emit syncProgressMessage(QString::fromStdString(cloudPath));
        if (!syncFolder(cloudPath)) {
            if (useIndex) { index.rollback(); index.close(); }
            emit syncThroughput(0);
            emit statusChanged(stopRequested_ ? SyncStatus::Idle : SyncStatus::Error);
            return;
        }
    }

    if (useIndex) {
        if (!index.commit()) {
            ydisquette::logToFile(QStringLiteral("[Sync] cloud→local index commit FAIL"));
        }
        index.close();
    }
    emit syncThroughput(0);
    emit statusChanged(SyncStatus::Idle);
}

void SyncWorker::doSyncLocalToCloud(const std::vector<std::string>& selectedPaths, const std::string& syncPath,
                                    const std::string& accessToken, const QString& indexDbPath, int maxRetries) {
    stopRequested_ = false;
    QElapsedTimer throughputTimer;
    throughputTimer.start();
    qint64 throughputBytes = 0;
    if (selectedPaths.empty() || syncPath.empty() || accessToken.empty()) {
        return;
    }
    TokenHolder holder;
    holder.token = accessToken;
    QNetworkAccessManager* nam = new QNetworkAccessManager(this);
    auth::YandexDiskApiClient* apiClient = new auth::YandexDiskApiClient(holder, nam, this);
    QUrlQuery probeQuery;
    probeQuery.addQueryItem(QStringLiteral("path"), QStringLiteral("/"));
    auth::ApiResponse probe = apiClient->get("/resources", probeQuery);
    if (probe.statusCode == 401) {
        emit tokenExpired();
        emit syncThroughput(0);
        emit statusChanged(SyncStatus::Error);
        return;
    }
    DiskResourceClient client(*apiClient);
    disk_tree::ApiTreeRepository treeRepo(*apiClient);

    QString syncPathQt = QString::fromStdString(syncPath).trimmed();
    QString syncRoot = syncPathQt.isEmpty() ? QString() : normalizeSyncRoot(QFileInfo(syncPathQt).absoluteFilePath());
    QString localRoot = syncRoot.isEmpty() ? QString() : QDir::cleanPath(syncRoot + QLatin1Char('/')) + QLatin1Char('/');

    auto normalizeCloudPath = [](std::string p) -> std::string {
        if (p.size() >= 5 && p.compare(0, 5, "disk:") == 0) return "/" + p.substr(5);
        if (!p.empty() && p[0] != '/') return "/" + p;
        return p;
    };
    auto cloudPathToLocal = [&normalizeCloudPath](const std::string& cloudPath) -> QString {
        return QString::fromStdString(normalizeCloudPath(cloudPath));
    };

    SyncIndex index;
    bool useIndex = !indexDbPath.isEmpty() && index.open(indexDbPath);
    if (!indexDbPath.isEmpty() && !useIndex)
        ydisquette::logToFile(QStringLiteral("[Sync] index open FAIL ") + indexDbPath);

    std::set<std::string> pathSet;
    for (const std::string& p : selectedPaths)
        pathSet.insert(normalizeCloudPath(p));
    if (useIndex && !syncRoot.isEmpty()) {
        for (const QString& top : index.getTopLevelRelativePaths(syncRoot)) {
            std::string cloud = "/" + top.toStdString();
            pathSet.insert(normalizeCloudPath(cloud));
        }
    }
    if (pathSet.find("/") == pathSet.end())
        pathSet.insert("/");

    std::set<std::string> selectedNormalized;
    for (const std::string& p : selectedPaths)
        selectedNormalized.insert(normalizeCloudPath(p));

    std::set<std::string> newTopLevelToCreate;
    for (const std::string& p : pathSet) {
        if (p.empty() || p == "/") continue;
        std::size_t firstSlash = p.find('/', 1);
        std::string topLevel = (firstSlash != std::string::npos) ? p.substr(0, firstSlash) : p;
        if (!selectedNormalized.count(topLevel))
            newTopLevelToCreate.insert(topLevel);
    }

    if (!newTopLevelToCreate.empty()) {
        std::vector<std::string> createdList;
        for (const std::string& cloudPath : newTopLevelToCreate) {
            QString localDir = QDir::cleanPath(localRoot + cloudPathToLocal(cloudPath).mid(1));
            if (!QDir(localDir).exists()) continue;
            DiskResourceResult cr = client.createFolder(cloudPath);
            if (!cr.success && cr.httpStatus != 409) {
                emit syncError(QStringLiteral("Create folder failed (local→cloud): ") + cr.errorMessage);
                continue;
            }
            if (cr.success && (cr.httpStatus == 200 || cr.httpStatus == 201))
                createdList.push_back(normalizeCloudPath(cloudPath));
        }
        if (!createdList.empty()) {
            emit pathsCreatedInCloud(createdList);
            emit syncThroughput(0);
            emit statusChanged(SyncStatus::Idle);
            return;
        }
    }

    if (useIndex && !index.beginTransaction()) {
        index.close();
        useIndex = false;
    }
    auto flushIndex = [&]() {
        if (useIndex) { index.commit(); index.beginTransaction(); }
    };

    auto toRelativePath = [&syncRoot](const QString& localPath) -> QString {
        QString base = syncRoot + QLatin1Char('/');
        if (!localPath.startsWith(base)) return QString();
        return localPath.mid(base.size());
    };

    std::set<std::string> createdFolders;
    std::function<bool(const QString&, const std::string&)> syncLocalToCloudFolder = [&](const QString& localDirPath, const std::string& cloudPath) -> bool {
        if (stopRequested_) return false;
        QDir dir(localDirPath);
        if (!dir.exists()) return true;
        if (!cloudPath.empty() && cloudPath != "/") {
            DiskResourceResult cr = client.createFolder(cloudPath);
            if (!cr.success && cr.httpStatus != 409) {
                emit syncError(QStringLiteral("Create folder failed (local→cloud): ") + cr.errorMessage);
                return false;
            }
            if (cr.success && (cr.httpStatus == 200 || cr.httpStatus == 201))
                createdFolders.insert(normalizeCloudPath(cloudPath));
        }
        const QStringList localNames = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        int localFileCount = 0;
        for (const QString& n : localNames)
            if (!QFileInfo(localDirPath + QLatin1Char('/') + n).isDir()) ++localFileCount;
        std::vector<std::shared_ptr<disk_tree::Node>> cloudChildren = treeRepo.getChildren(cloudPath);
        std::map<std::string, disk_tree::Node*> cloudByName;
        for (const auto& n : cloudChildren)
            if (n && !n->name.empty()) cloudByName[n->name] = n.get();

        for (const QString& name : localNames) {
            if (stopRequested_) return false;
            const QString localPath = localDirPath + QLatin1Char('/') + name;
            const std::string nameStr = name.toStdString();
            const std::string childCloudPath = std::string(cloudPath) + (cloudPath.empty() || cloudPath.back() == '/' ? "" : "/") + nameStr;
            if (QFileInfo(localPath).isDir()) {
                DiskResourceResult cr = client.createFolder(childCloudPath);
                if (!cr.success && cr.httpStatus != 409) {
                    emit syncError(QStringLiteral("Create folder failed (local→cloud): ") + cr.errorMessage);
                    continue;
                }
                if (cr.success && (cr.httpStatus == 200 || cr.httpStatus == 201))
                    createdFolders.insert(normalizeCloudPath(childCloudPath));
                if (!syncLocalToCloudFolder(localPath, childCloudPath)) continue;
            } else {
                auto it = cloudByName.find(nameStr);
                disk_tree::Node* cloudNode = (it != cloudByName.end()) ? it->second : nullptr;
                bool needUpload = false;
                bool skipReasonIndex = false;
                if (useIndex) {
                    QString rel = toRelativePath(localPath);
                    if (!rel.isEmpty()) {
                        auto entry = index.get(syncRoot, rel);
                        QFileInfo fi(localPath);
                        qint64 msec = fi.lastModified().toSecsSinceEpoch();
                        qint64 sz = fi.size();
                        needUpload = !entry || entry->mtime_sec != msec || entry->size != sz
                            || FileStatus::needsUpload(entry->status);
                        skipReasonIndex = !needUpload;
                    } else {
                        needUpload = true;
                    }
                } else {
                    needUpload = true;
                }
                if (needUpload && cloudNode && !localNewerThanCloud(cloudNode, localPath)) {
                    needUpload = false;
                    skipReasonIndex = false;
                }
                if (!needUpload) {
                    if (useIndex) {
                        QString rel = toRelativePath(localPath);
                        if (!rel.isEmpty()) {
                            auto entry = index.get(syncRoot, rel);
                            if (!entry || entry->status != FileStatus::FAILED) {
                                QFileInfo fi(localPath);
                                index.set(syncRoot, rel, fi.lastModified().toSecsSinceEpoch(), fi.size(),
                                        QString::fromUtf8(FileStatus::SYNCED), 0);
                                flushIndex();
                            }
                        }
                    }
                } else {
                    if (useIndex) {
                        QString rel = toRelativePath(localPath);
                        if (!rel.isEmpty()) {
                            auto entry = index.get(syncRoot, rel);
                            if (entry && entry->status == FileStatus::NEW) {
                                index.setStatus(syncRoot, rel, FileStatus::UPLOADING, 0);
                                flushIndex();
                            }
                        }
                    }
                    emit syncProgressMessage(QStringLiteral("local→cloud ") + QString::fromStdString(childCloudPath));
                    DiskResourceResult ur = client.uploadFile(childCloudPath, localPath);
                    if (!ur.success) {
                        if (useIndex) {
                            QString rel = toRelativePath(localPath);
                            if (!rel.isEmpty()) {
                                auto entry = index.get(syncRoot, rel);
                                int newRetries = (entry ? entry->retries : 0) + 1;
                                QString newStatus = (newRetries >= maxRetries)
                                    ? QString::fromUtf8(FileStatus::FAILED)
                                    : QString::fromUtf8(FileStatus::UPLOADING);
                                index.setStatus(syncRoot, rel, newStatus, 1);
                                ydisquette::logToFile(QStringLiteral("[Sync] upload failed ") + rel
                                    + QStringLiteral(" retries=") + QString::number(newRetries));
                                flushIndex();
                            }
                        }
                        emit syncError(QStringLiteral("Upload failed (local→cloud): ") + ur.errorMessage);
                        continue;
                    }
                    throughputBytes += QFileInfo(localPath).size();
                    qint64 ms = throughputTimer.elapsed();
                    emit syncThroughput(ms > 0 ? (throughputBytes * 1000 / ms) : 0);
                    if (useIndex) {
                        QString rel = toRelativePath(localPath);
                        if (!rel.isEmpty()) {
                            QFileInfo fi(localPath);
                            index.set(syncRoot, rel, fi.lastModified().toSecsSinceEpoch(), fi.size(),
                                    QString::fromUtf8(FileStatus::SYNCED), 0);
                            flushIndex();
                        }
                    }
                }
            }
        }
        if (localFileCount > 0) {
            for (const auto& node : cloudChildren) {
                if (!node) continue;
                if (!localNames.contains(QString::fromStdString(node->name))) {
                    DiskResourceResult dr = client.deleteResource(node->path);
                    if (!dr.success) {
                        if (useIndex) index.rollback();
                        index.close();
                        emit syncError(QStringLiteral("Delete in cloud failed: ") + dr.errorMessage);
                        return false;
                    }
                    if (useIndex) {
                        QString rel = cloudPathToLocal(node->path).mid(1);
                        if (!rel.isEmpty()) {
                            if (node->isDir())
                                index.removePrefix(syncRoot, rel);
                            else
                                index.remove(syncRoot, rel);
                        }
                    }
                }
            }
        }
        return true;
    };

    for (const std::string& cloudPath : pathSet) {
        if (stopRequested_) break;
        QString localDir = QDir::cleanPath(localRoot + cloudPathToLocal(cloudPath).mid(1));
        if (!QDir(localDir).exists()) continue;
        emit syncProgressMessage(QStringLiteral("local→cloud ") + QString::fromStdString(cloudPath));
        if (!syncLocalToCloudFolder(localDir, cloudPath)) {
            if (useIndex) index.rollback();
            index.close();
            emit syncThroughput(0);
            emit statusChanged(stopRequested_ ? SyncStatus::Idle : SyncStatus::Error);
            return;
        }
    }

    std::set<std::string> newTopLevelSet;
    for (const std::string& p : createdFolders) {
        if (p.empty() || p == "/") continue;
        std::string topLevel = "/";
        std::size_t firstSlash = p.find('/', 1);
        if (firstSlash != std::string::npos)
            topLevel = p.substr(0, firstSlash);
        else
            topLevel = p;
        if (selectedNormalized.count(topLevel)) continue;
        newTopLevelSet.insert(topLevel);
    }
    std::vector<std::string> newTopLevelPaths(newTopLevelSet.begin(), newTopLevelSet.end());
    if (useIndex) {
        if (!index.commit())
            ydisquette::logToFile(QStringLiteral("[Sync] local→cloud index commit FAIL"));
        index.close();
    }
    if (!newTopLevelPaths.empty())
        emit pathsCreatedInCloud(newTopLevelPaths);

    if (stopRequested_) {
        emit syncThroughput(0);
        emit statusChanged(SyncStatus::Idle);
    } else {
        emit localToCloudFinished(selectedPaths, syncPath, accessToken);
    }
}

void SyncWorker::loadIndexState(const QString& indexDbPath) {
    emit indexStateLoaded(readIndexState(indexDbPath));
}

}  // namespace sync
}  // namespace ydisquette
