#include "sync/application/sync_cloud_to_local_use_case.hpp"
#include "shared/cloud_path_util.hpp"
#include "sync/domain/cloud_local_compare.hpp"
#include "sync/domain/sync_file_status.hpp"
#include "sync/application/sync_path_mapper.hpp"
#include "shared/app_log.hpp"
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <algorithm>

namespace ydisquette {
namespace sync {

static QString normRel(QString rel) {
    while (rel.startsWith(QLatin1Char('/'))) rel = rel.mid(1);
    return rel;
}

SyncCloudToLocalUseCase::Result SyncCloudToLocalUseCase::run(
    disk_tree::ITreeRepository& treeRepo,
    DiskResourceClient& diskClient,
    SyncIndex* index,
    const QString& syncRoot,
    const QString& localRoot,
    const std::vector<std::string>& selectedPaths,
    int maxRetries,
    std::function<bool()> stopRequested,
    const SyncCloudToLocalCallbacks& callbacks) {
    const bool useIndex = index != nullptr;
    qint64 throughputBytes = 0;

    auto flushIndex = [useIndex, index]() {
        if (useIndex && index && index->commit()) index->beginTransaction();
    };

    auto toRelativePath = [&syncRoot](const QString& localPath) -> QString {
        return localPathToRelative(localPath, syncRoot);
    };

    if (useIndex && index) {
        QStringList cloudDeleted = index->getRelativePathsWithStatus(syncRoot, QString::fromUtf8(FileStatus::CLOUD_DELETED));
        std::sort(cloudDeleted.begin(), cloudDeleted.end(), [](const QString& a, const QString& b) {
            return a.length() > b.length() || (a.length() == b.length() && a > b);
        });
        for (const QString& rel : cloudDeleted) {
            if (stopRequested && stopRequested()) return Result::Stopped;
            QString localPath = localRoot + normRel(rel);
            if (callbacks.onProgressMessage)
                callbacks.onProgressMessage(QStringLiteral("cloud deleted: ") + rel);
            QFileInfo fi(localPath);
            if (fi.isDir()) {
                index->removePrefix(syncRoot, rel);
                flushIndex();
                if (QDir(localPath).exists() && !QDir(localPath).removeRecursively() && callbacks.onError)
                    callbacks.onError(QStringLiteral("Failed to remove local directory: ") + localPath);
            } else {
                index->remove(syncRoot, rel);
                flushIndex();
                if (QFileInfo::exists(localPath) && !QFile::remove(localPath) && callbacks.onError)
                    callbacks.onError(QStringLiteral("Failed to remove local file: ") + localPath);
            }
        }
        QStringList toDownload = index->getRelativePathsWithStatus(syncRoot, QString::fromUtf8(FileStatus::TO_DOWNLOAD));
        toDownload.append(index->getRelativePathsWithStatus(syncRoot, QString::fromUtf8(FileStatus::DOWNLOADING)));
        toDownload.removeDuplicates();
        if (toDownload.isEmpty()) {
            if (!index->commit())
                ydisquette::logToFile(QStringLiteral("[Sync] cloud→local index commit FAIL"));
            return Result::Success;
        }
    }

    std::function<bool(const std::string&)> syncFolder = [&](const std::string& cloudPath) -> bool {
        if (stopRequested && stopRequested()) return false;
        if (callbacks.onProgressMessage)
            callbacks.onProgressMessage(QString::fromStdString(cloudPath));
        std::vector<std::shared_ptr<disk_tree::Node>> children = treeRepo.getChildren(cloudPath);
        QString localDir = localRoot + cloudPathToRelativeQString(cloudPath);
        for (const auto& node : children) {
            if (stopRequested && stopRequested()) return false;
            if (!node) continue;
            std::string remotePath = node->path;
            QString localPath = localRoot + cloudPathToRelativeQString(remotePath);
            if (node->isDir()) {
                if (!syncFolder(remotePath)) return false;
            } else {
                QFileInfo fi(localPath);
                bool exists = fi.exists();
                bool empty = fi.size() == 0;
                bool needDownload = !exists || empty || cloudNewerThanLocal(node.get(), localPath);
                if (useIndex && index) {
                    QString rel = normRel(toRelativePath(localPath));
                    if (!rel.isEmpty()) {
                        auto entry = index->get(syncRoot, rel);
                        if (entry && entry->status == QLatin1String(FileStatus::TO_DELETE))
                            needDownload = false;
                        else if (entry && FileStatus::needsDownload(entry->status))
                            needDownload = true;
                        else if (entry && entry->status == FileStatus::SYNCED && exists
                                 && fi.size() == static_cast<qint64>(node->size)
                                 && entry->size == static_cast<qint64>(node->size))
                            needDownload = false;
                    }
                }
                if (!needDownload) {
                } else {
                    if (useIndex && index) {
                        QString rel = normRel(toRelativePath(localPath));
                        if (!rel.isEmpty()) {
                            auto entry = index->get(syncRoot, rel);
                            if (entry && entry->status == QLatin1String(FileStatus::TO_DELETE))
                                continue;
                            if (!entry) index->upsertNew(syncRoot, rel, 0, 0);
                            index->setStatus(syncRoot, rel, QString::fromUtf8(FileStatus::DOWNLOADING), 0);
                            flushIndex();
                        }
                    }
                    if (callbacks.onProgressMessage)
                        callbacks.onProgressMessage(QStringLiteral("cloud→local ") + QString::fromStdString(remotePath));
                    QString parentDir = QFileInfo(localPath).absolutePath();
                    if (!QDir().mkpath(parentDir)) {
                        if (callbacks.onError)
                            callbacks.onError(QStringLiteral("Failed to create directory: ") + parentDir);
                        continue;
                    }
                    ydisquette::logToFile(QStringLiteral("[Sync] download start ") + QString::fromStdString(remotePath)
                        + QStringLiteral(" size=") + QString::number(node ? static_cast<qint64>(node->size) : 0));
                    QElapsedTimer fileTimer;
                    fileTimer.start();
                    DiskResourceResult dr = diskClient.downloadFile(remotePath, localPath);
                    if (dr.success) {
                        ydisquette::logToFile(QStringLiteral("[Sync] download OK ") + QString::fromStdString(remotePath));
                        if (callbacks.onProgressMessage)
                            callbacks.onProgressMessage(QStringLiteral("cloud→local OK ") + QString::fromStdString(remotePath));
                        throughputBytes += static_cast<qint64>(node->size);
                        qint64 fileMs = qMax(qint64(1), fileTimer.elapsed());
                        qint64 fileSize = static_cast<qint64>(node->size);
                        if (callbacks.onThroughput)
                            callbacks.onThroughput(fileSize * 1000 / fileMs);
                    }
                    if (!dr.success) {
                        if (useIndex && index) {
                            QString rel = normRel(toRelativePath(localPath));
                            if (!rel.isEmpty()) {
                                auto entry = index->get(syncRoot, rel);
                                int newRetries = (entry ? entry->retries : 0) + 1;
                                QString newStatus = (newRetries >= maxRetries)
                                    ? QString::fromUtf8(FileStatus::FAILED)
                                    : QString::fromUtf8(FileStatus::TO_DOWNLOAD);
                                index->setStatus(syncRoot, rel, newStatus, 1);
                                ydisquette::logToFile(QStringLiteral("[Sync] download failed ") + rel
                                    + QStringLiteral(" retries=") + QString::number(newRetries));
                                flushIndex();
                            }
                        }
                        if (callbacks.onError)
                            callbacks.onError(QStringLiteral("Download failed (HTTP %1). Yandex: %2")
                                                  .arg(dr.httpStatus).arg(dr.errorMessage));
                        continue;
                    }
                }
                if (useIndex && index) {
                    QString rel = normRel(toRelativePath(localPath));
                    if (rel.isEmpty()) continue;
                    auto entry = index->get(syncRoot, rel);
                    if (entry && entry->status == QLatin1String(FileStatus::TO_DELETE)) continue;
                    QFileInfo fi2(localPath);
                    if (entry && entry->status == QLatin1String(FileStatus::SYNCED)
                        && entry->size == fi2.size()
                        && entry->mtime_sec == fi2.lastModified().toSecsSinceEpoch())
                        continue;
                    if (!index->set(syncRoot, rel, fi2.lastModified().toSecsSinceEpoch(), fi2.size(),
                                    QString::fromUtf8(FileStatus::SYNCED), 0))
                        ydisquette::logToFile(QStringLiteral("[Sync] index set FAIL (cloud→local) ") + rel);
                    flushIndex();
                }
            }
        }
        QSet<QString> cloudNames;
        for (const auto& node : children)
            if (node && !node->name.empty())
                cloudNames.insert(QString::fromStdString(node->name));
        if (!QDir(localDir).exists())
            return true;
        QDir dir(localDir);
        const QStringList localNames = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        for (const QString& name : localNames) {
            if (stopRequested && stopRequested()) return false;
            if (cloudNames.contains(name)) continue;
            QString localPath = localDir + QLatin1Char('/') + name;
            QString rel = normRel(toRelativePath(localPath));
            if (rel.isEmpty()) continue;
            if (useIndex && index) {
                if (index->hasAnyUnderPrefixWithStatus(syncRoot, rel,
                    {QString::fromUtf8(FileStatus::NEW), QString::fromUtf8(FileStatus::UPLOADING)}))
                    continue;
            }
            QFileInfo fi(localPath);
            if (fi.isDir()) {
                if (useIndex && index) {
                    index->removePrefix(syncRoot, rel);
                    flushIndex();
                }
                if (callbacks.onProgressMessage)
                    callbacks.onProgressMessage(QStringLiteral("cloud deleted: ") + rel);
                if (!QDir(localPath).removeRecursively() && callbacks.onError)
                    callbacks.onError(QStringLiteral("Failed to remove local directory: ") + localPath);
            } else {
                if (useIndex && index) {
                    index->remove(syncRoot, rel);
                    flushIndex();
                }
                if (callbacks.onProgressMessage)
                    callbacks.onProgressMessage(QStringLiteral("cloud deleted: ") + rel);
                if (!QFile::remove(localPath) && callbacks.onError)
                    callbacks.onError(QStringLiteral("Failed to remove local file: ") + localPath);
            }
        }
        return true;
    };

    std::function<bool(const std::string&)> downloadToDownloadOnly = [&](const std::string& cloudPath) -> bool {
        if (stopRequested && stopRequested()) return false;
        std::vector<std::shared_ptr<disk_tree::Node>> children = treeRepo.getChildren(cloudPath);
        QString localDir = localRoot + cloudPathToRelativeQString(cloudPath);
        for (const auto& node : children) {
            if (stopRequested && stopRequested()) return false;
            if (!node) continue;
            std::string remotePath = node->path;
            QString localPath = localRoot + cloudPathToRelativeQString(remotePath);
            if (node->isDir()) {
                if (!downloadToDownloadOnly(remotePath)) return false;
            } else if (useIndex && index) {
                QString rel = normRel(toRelativePath(localPath));
                if (rel.isEmpty()) continue;
                auto entry = index->get(syncRoot, rel);
                if (!entry || !FileStatus::needsDownload(entry->status)) continue;
                index->setStatus(syncRoot, rel, QString::fromUtf8(FileStatus::DOWNLOADING), 0);
                flushIndex();
                if (callbacks.onProgressMessage)
                    callbacks.onProgressMessage(QStringLiteral("cloud→local ") + QString::fromStdString(remotePath));
                if (!QDir().mkpath(QFileInfo(localPath).absolutePath())) continue;
                DiskResourceResult dr = diskClient.downloadFile(remotePath, localPath);
                if (dr.success) {
                    throughputBytes += static_cast<qint64>(node->size);
                    QFileInfo fi2(localPath);
                    index->set(syncRoot, rel, fi2.lastModified().toSecsSinceEpoch(), fi2.size(),
                               QString::fromUtf8(FileStatus::SYNCED), 0);
                    flushIndex();
                } else {
                    int newRetries = (entry ? entry->retries : 0) + 1;
                    QString newStatus = (newRetries >= maxRetries)
                        ? QString::fromUtf8(FileStatus::FAILED)
                        : QString::fromUtf8(FileStatus::TO_DOWNLOAD);
                    index->setStatus(syncRoot, rel, newStatus, 1);
                    flushIndex();
                }
            }
        }
        return true;
    };

    if (useIndex && index) {
        for (const std::string& cloudPath : selectedPaths) {
            if (stopRequested && stopRequested()) return Result::Stopped;
            if (!downloadToDownloadOnly(cloudPath))
                return (stopRequested && stopRequested()) ? Result::Stopped : Result::Error;
        }
    } else {
        for (const std::string& cloudPath : selectedPaths) {
            if (stopRequested && stopRequested()) return Result::Stopped;
            if (callbacks.onProgressMessage)
                callbacks.onProgressMessage(QString::fromStdString(cloudPath));
            if (!syncFolder(cloudPath))
                return (stopRequested && stopRequested()) ? Result::Stopped : Result::Error;
        }
    }

    if (useIndex && index && !index->commit())
        ydisquette::logToFile(QStringLiteral("[Sync] cloud→local index commit FAIL"));
    return Result::Success;
}

}  // namespace sync
}  // namespace ydisquette
