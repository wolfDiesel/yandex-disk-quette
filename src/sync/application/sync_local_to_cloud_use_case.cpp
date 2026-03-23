#include "sync/application/sync_local_to_cloud_use_case.hpp"
#include "sync/application/to_delete_batches.hpp"
#include "sync/application/sync_path_mapper.hpp"
#include "shared/cloud_path_util.hpp"
#include "sync/domain/cloud_local_compare.hpp"
#include "shared/app_log.hpp"
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <map>
#include <set>

namespace ydisquette {
namespace sync {

static QString normRel(QString rel) {
    while (rel.startsWith(QLatin1Char('/'))) rel = rel.mid(1);
    return rel;
}

SyncLocalToCloudUseCase::Result SyncLocalToCloudUseCase::run(
    disk_tree::ITreeRepository& treeRepo,
    DiskResourceClient& diskClient,
    SyncIndex* index,
    const QString& syncRoot,
    const QString& localRoot,
    const std::vector<std::string>& selectedPaths,
    int maxRetries,
    std::function<bool()> stopRequested,
    const SyncLocalToCloudCallbacks& callbacks) {
    const bool useIndex = index != nullptr;

    std::set<std::string> pathSet;
    for (const std::string& p : selectedPaths)
        pathSet.insert(normalizeCloudPath(p));
    if (useIndex && index && !syncRoot.isEmpty()) {
        for (const QString& top : index->getTopLevelRelativePaths(syncRoot)) {
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
            if (stopRequested && stopRequested()) return Result::Stopped;
            QString localDir = QDir::cleanPath(localRoot + cloudPathToRelativeQString(cloudPath));
            if (!QDir(localDir).exists()) continue;
            DiskResourceResult cr = diskClient.createFolder(cloudPath);
            if (!cr.success && cr.httpStatus != 409) {
                if (callbacks.onError)
                    callbacks.onError(QStringLiteral("Create folder failed (local→cloud): ") + cr.errorMessage);
                continue;
            }
            if (cr.success && (cr.httpStatus == 200 || cr.httpStatus == 201))
                createdList.push_back(normalizeCloudPath(cloudPath));
        }
        if (!createdList.empty() && callbacks.onPathsCreatedInCloud) {
            callbacks.onPathsCreatedInCloud(createdList);
            return Result::Success;
        }
    }

    auto flushIndex = [useIndex, index]() {
        if (useIndex && index && index->commit()) index->beginTransaction();
    };

    if (useIndex && index && !syncRoot.isEmpty()) {
        QStringList toDeletePaths = index->getRelativePathsWithStatus(syncRoot, QString::fromUtf8(FileStatus::TO_DELETE));
        if (!toDeletePaths.isEmpty()) {
            ToDeleteBatches batches = computeToDeleteBatches(toDeletePaths);
            for (const QString& rel : batches.rootFiles) {
                if (stopRequested && stopRequested()) {
                    if (index) { index->rollback(); index->close(); }
                    return Result::Stopped;
                }
                std::string cp = normalizeCloudPath("/" + rel.toStdString());
                if (callbacks.onProgressMessage)
                    callbacks.onProgressMessage(QStringLiteral("cloud delete ") + QString::fromStdString(cp));
                DiskResourceResult dr = diskClient.deleteResource(cp);
                if (!dr.success) {
                    if (index) index->rollback();
                    if (callbacks.onError)
                        callbacks.onError(QStringLiteral("Delete in cloud failed (TO_DELETE): ") + dr.errorMessage);
                    return Result::Error;
                }
                index->remove(syncRoot, rel);
                flushIndex();
            }
            for (const QString& prefix : batches.minimalFolders) {
                if (stopRequested && stopRequested()) {
                    if (index) index->rollback();
                    return Result::Stopped;
                }
                std::string cp = normalizeCloudPath("/" + prefix.toStdString());
                if (callbacks.onProgressMessage)
                    callbacks.onProgressMessage(QStringLiteral("cloud delete ") + QString::fromStdString(cp));
                DiskResourceResult dr = diskClient.deleteResource(cp);
                if (!dr.success) {
                    if (index) index->rollback();
                    if (callbacks.onError)
                        callbacks.onError(QStringLiteral("Delete folder in cloud failed (TO_DELETE): ") + dr.errorMessage);
                    return Result::Error;
                }
                index->removePrefix(syncRoot, prefix);
                flushIndex();
            }
        }
    }

    auto toRelativePath = [&syncRoot](const QString& localPath) -> QString {
        return normRel(localPathToRelative(localPath, syncRoot));
    };

    std::set<std::string> createdFolders;
    std::function<bool(const QString&, const std::string&)> syncLocalToCloudFolder = [&](const QString& localDirPath, const std::string& cloudPath) -> bool {
        if (stopRequested && stopRequested()) return false;
        QDir dir(localDirPath);
        if (!dir.exists()) return true;
        if (!cloudPath.empty() && cloudPath != "/") {
            DiskResourceResult cr = diskClient.createFolder(cloudPath);
            if (!cr.success && cr.httpStatus != 409) {
                if (callbacks.onError)
                    callbacks.onError(QStringLiteral("Create folder failed (local→cloud): ") + cr.errorMessage);
                return false;
            }
            if (cr.success && (cr.httpStatus == 200 || cr.httpStatus == 201))
                createdFolders.insert(normalizeCloudPath(cloudPath));
        }
        const QStringList localNames = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        std::vector<std::shared_ptr<disk_tree::Node>> cloudChildren = treeRepo.getChildren(cloudPath);
        std::map<std::string, disk_tree::Node*> cloudByName;
        for (const auto& n : cloudChildren)
            if (n && !n->name.empty()) cloudByName[n->name] = n.get();

        for (const QString& name : localNames) {
            if (stopRequested && stopRequested()) return false;
            const QString localPath = localDirPath + QLatin1Char('/') + name;
            const std::string nameStr = name.toStdString();
            const std::string childCloudPath = std::string(cloudPath) + (cloudPath.empty() || cloudPath.back() == '/' ? "" : "/") + nameStr;
            if (QFileInfo(localPath).isDir()) {
                DiskResourceResult cr = diskClient.createFolder(childCloudPath);
                if (!cr.success && cr.httpStatus != 409) {
                    if (callbacks.onError)
                        callbacks.onError(QStringLiteral("Create folder failed (local→cloud): ") + cr.errorMessage);
                    continue;
                }
                if (cr.success && (cr.httpStatus == 200 || cr.httpStatus == 201))
                    createdFolders.insert(normalizeCloudPath(childCloudPath));
                if (!syncLocalToCloudFolder(localPath, childCloudPath)) continue;
            } else {
                auto it = cloudByName.find(nameStr);
                disk_tree::Node* cloudNode = (it != cloudByName.end()) ? it->second : nullptr;
                bool needUpload = false;
                if (useIndex && index) {
                    QString rel = toRelativePath(localPath);
                    if (!rel.isEmpty()) {
                        auto entry = index->get(syncRoot, rel);
                        QFileInfo fi(localPath);
                        qint64 msec = fi.lastModified().toSecsSinceEpoch();
                        qint64 sz = fi.size();
                        needUpload = !entry || entry->mtime_sec != msec || entry->size != sz
                            || FileStatus::needsUpload(entry->status);
                    } else {
                        needUpload = true;
                    }
                } else {
                    needUpload = true;
                }
                if (needUpload && cloudNode && !localNewerThanCloud(cloudNode, localPath))
                    needUpload = false;
                if (!needUpload) {
                    if (useIndex && index) {
                        QString rel = toRelativePath(localPath);
                        if (!rel.isEmpty()) {
                            auto entry = index->get(syncRoot, rel);
                            if (!entry || entry->status == FileStatus::FAILED) continue;
                            QFileInfo fi(localPath);
                            qint64 msec = fi.lastModified().toSecsSinceEpoch();
                            qint64 sz = fi.size();
                            if (entry->status == QLatin1String(FileStatus::SYNCED)
                                && entry->mtime_sec == msec && entry->size == sz)
                                continue;
                            index->set(syncRoot, rel, msec, sz, QString::fromUtf8(FileStatus::SYNCED), 0);
                            flushIndex();
                        }
                    }
                } else {
                    if (useIndex && index) {
                        QString rel = toRelativePath(localPath);
                        if (!rel.isEmpty()) {
                            auto entry = index->get(syncRoot, rel);
                            if (entry && entry->status == FileStatus::NEW) {
                                index->setStatus(syncRoot, rel, FileStatus::UPLOADING, 0);
                                flushIndex();
                            }
                        }
                    }
                    std::string originalCloudPath = childCloudPath;
                    std::string tempCloudPath = originalCloudPath + ".tmp-upload";
                    if (callbacks.onProgressMessage)
                        callbacks.onProgressMessage(QStringLiteral("local→cloud ") + QString::fromStdString(tempCloudPath));
                    qint64 fileSize = QFileInfo(localPath).size();
                    ydisquette::logToFile(QStringLiteral("[Sync] upload start ") + QString::fromStdString(tempCloudPath)
                        + QStringLiteral(" size=") + QString::number(fileSize));
                    QElapsedTimer fileTimer;
                    fileTimer.start();
                    DiskResourceResult ur = diskClient.uploadFile(tempCloudPath, localPath, [&callbacks](qint64 bytesPerSec) {
                        if (callbacks.onThroughput) callbacks.onThroughput(bytesPerSec);
                    });
                    if (!ur.success) {
                        if (useIndex && index) {
                            QString rel = toRelativePath(localPath);
                            if (!rel.isEmpty()) {
                                auto entry = index->get(syncRoot, rel);
                                int newRetries = (entry ? entry->retries : 0) + 1;
                                QString newStatus = (newRetries >= maxRetries)
                                    ? QString::fromUtf8(FileStatus::FAILED)
                                    : QString::fromUtf8(FileStatus::UPLOADING);
                                index->setStatus(syncRoot, rel, newStatus, 1);
                                ydisquette::logToFile(QStringLiteral("[Sync] upload failed ") + rel
                                    + QStringLiteral(" retries=") + QString::number(newRetries));
                                flushIndex();
                            }
                        }
                        if (callbacks.onError)
                            callbacks.onError(QStringLiteral("Upload failed (local→cloud): ") + ur.errorMessage);
                        continue;
                    }
                    DiskResourceResult delExisting = diskClient.deleteResource(originalCloudPath);
                    Q_UNUSED(delExisting);
                    DiskResourceResult mr = diskClient.moveResource(tempCloudPath, originalCloudPath);
                    if (!mr.success) {
                        if (callbacks.onError)
                            callbacks.onError(QStringLiteral("Move failed (local→cloud): ") + mr.errorMessage);
                        continue;
                    }
                    QString relOk = toRelativePath(localPath);
                    if (!relOk.isEmpty())
                        ydisquette::logToFile(QStringLiteral("[Sync] upload OK ") + relOk);
                    if (callbacks.onProgressMessage)
                        callbacks.onProgressMessage(QStringLiteral("local→cloud OK ") + QString::fromStdString(originalCloudPath));
                    if (callbacks.onThroughput) {
                        qint64 fileMs = qMax(qint64(1), fileTimer.elapsed());
                        callbacks.onThroughput(fileSize * 1000 / fileMs);
                    }
                    if (useIndex && index) {
                        QString rel = toRelativePath(localPath);
                        if (!rel.isEmpty()) {
                            QFileInfo fi(localPath);
                            index->set(syncRoot, rel, fi.lastModified().toSecsSinceEpoch(), fi.size(),
                                       QString::fromUtf8(FileStatus::SYNCED), 0);
                            flushIndex();
                        }
                    }
                }
            }
        }
        for (const auto& node : cloudChildren) {
            if (!node) continue;
            if (!localNames.contains(QString::fromStdString(node->name))) {
                if (useIndex && index) {
                    QString rel = cloudPathToRelativeQString(node->path).trimmed();
                    if (!rel.isEmpty()) {
                        if (node->isDir())
                            index->setStatusPrefix(syncRoot, rel, QString::fromUtf8(FileStatus::TO_DELETE));
                        else
                            index->setStatus(syncRoot, rel, QString::fromUtf8(FileStatus::TO_DELETE), 0);
                        flushIndex();
                    }
                }
            }
        }
        return true;
    };

    for (const std::string& cloudPath : pathSet) {
        if (stopRequested && stopRequested()) {
            if (useIndex && index) index->rollback();
            return Result::Stopped;
        }
        if (cloudPath.empty() || cloudPath == "/") continue;
        QString localDir = QDir::cleanPath(localRoot + cloudPathToRelativeQString(cloudPath));
        if (!QDir(localDir).exists()) {
            if (useIndex && index) {
                QString prefix = cloudPathToRelativeQString(cloudPath).trimmed();
                if (!prefix.isEmpty()) {
                    index->setStatusPrefix(syncRoot, prefix, QString::fromUtf8(FileStatus::TO_DELETE));
                    flushIndex();
                }
            }
            continue;
        }
        if (callbacks.onProgressMessage)
            callbacks.onProgressMessage(QStringLiteral("local→cloud ") + QString::fromStdString(cloudPath));
        if (!syncLocalToCloudFolder(localDir, cloudPath)) {
            if (useIndex && index) index->rollback();
            return (stopRequested && stopRequested()) ? Result::Stopped : Result::Error;
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
    if (!newTopLevelPaths.empty() && callbacks.onPathsCreatedInCloud)
        callbacks.onPathsCreatedInCloud(newTopLevelPaths);

    return Result::Success;
}

}  // namespace sync
}  // namespace ydisquette
