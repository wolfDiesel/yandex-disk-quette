#include "sync/infrastructure/poll_worker.hpp"
#include "sync/infrastructure/sync_index.hpp"
#include "sync/infrastructure/sqlite_poll_run_repository.hpp"
#include "sync/infrastructure/disk_resource_client.hpp"
#include "sync/infrastructure/last_uploaded_parser.hpp"
#include "sync/infrastructure/trash_parser.hpp"
#include "sync/domain/sync_file_status.hpp"
#include "shared/cloud_path_util.hpp"
#include "auth/infrastructure/token_holder.hpp"
#include "auth/infrastructure/yandex_disk_api_client.hpp"
#include "auth/infrastructure/ssl_ignoring_network_access_manager.hpp"
#include "shared/app_log.hpp"
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QUrlQuery>

namespace ydisquette {
namespace sync {

static bool isPathUnderSynced(const SyncIndex& index, const QString& syncRoot, const QString& rel) {
    QString path = rel.trimmed();
    while (path.startsWith(QLatin1Char('/'))) path = path.mid(1);
    if (path.isEmpty()) return false;
    while (!path.isEmpty()) {
        if (index.hasAnyWithPrefix(syncRoot, path))
            return true;
        int lastSlash = path.lastIndexOf(QLatin1Char('/'));
        if (lastSlash < 0) break;
        path = path.left(lastSlash);
    }
    return false;
}

static std::string relativeToApiPath(const QString& rel) {
    QString r = rel.trimmed();
    while (r.startsWith(QLatin1Char('/'))) r = r.mid(1);
    return "/" + r.toStdString();
}

PollWorker::PollWorker(QObject* parent) : QObject(parent) {}

void PollWorker::doPoll(const QString& syncRoot, const QString& indexDbPath,
                        const QString& accessToken, int pollTimeSec, int maxRetries) {
    stopRequested_ = false;
    std::string accessTokenStr = accessToken.trimmed().toStdString();
    if (syncRoot.isEmpty() || indexDbPath.isEmpty() || accessTokenStr.empty() || pollTimeSec < 60) {
        emit pollFailed(QStringLiteral("Invalid poll parameters"));
        return;
    }
    auth::TokenHolder holder;
    holder.token = accessTokenStr;
    QNetworkAccessManager* nam = new auth::SslIgnoringNetworkAccessManager(this);
    auth::YandexDiskApiClient* apiClient = new auth::YandexDiskApiClient(holder, nam, this);
    QUrlQuery probeQuery;
    probeQuery.addQueryItem(QStringLiteral("path"), QStringLiteral("/"));
    auth::ApiResponse probe = apiClient->get("/resources", probeQuery);
    if (probe.statusCode == 401) {
        emit tokenExpired();
        return;
    }
    SyncIndex index;
    if (!index.open(QFileInfo(indexDbPath).absoluteFilePath())) {
        emit pollFailed(QStringLiteral("Index open failed"));
        return;
    }
    SqlitePollRunRepository pollRepo;
    if (!pollRepo.open(indexDbPath)) {
        index.close();
        emit pollFailed(QStringLiteral("Poll run repository open failed"));
        return;
    }
    qint64 nowSec = QDateTime::currentSecsSinceEpoch();
    qint64 runId = pollRepo.insert(syncRoot, nowSec);
    if (runId <= 0) {
        pollRepo.close();
        index.close();
        emit pollFailed(QStringLiteral("Poll run insert failed"));
        return;
    }
    std::optional<qint64> lastFinished = pollRepo.getLastFinishedAt(syncRoot);
    qint64 sinceSec = lastFinished.has_value() ? *lastFinished : (nowSec - static_cast<qint64>(pollTimeSec));

    QUrlQuery lastQuery;
    lastQuery.addQueryItem(QStringLiteral("limit"), QString::number(300));
    auth::ApiResponse lastRes = apiClient->get("/resources/last-uploaded", lastQuery);
    if (!lastRes.ok()) {
        QString err = QString::fromStdString(lastRes.body);
        if (err.isEmpty()) err = QStringLiteral("HTTP %1").arg(lastRes.statusCode);
        pollRepo.updateRun(runId, QStringLiteral("failed"), nowSec, 0, err);
        pollRepo.close();
        index.close();
        emit pollFailed(err);
        return;
    }
    QVector<LastUploadedItem> items = parseLastUploadedJson(lastRes.body);
    DiskResourceClient client(*apiClient);
    QString localRoot = QDir::cleanPath(syncRoot + QLatin1Char('/')) + QLatin1Char('/');
    int changesCount = 0;
    if (!index.beginTransaction()) {
        pollRepo.updateRun(runId, QStringLiteral("failed"), nowSec, 0, QStringLiteral("Index transaction failed"));
        index.close();
        pollRepo.close();
        emit pollFailed(QStringLiteral("Index transaction failed"));
        return;
    }
    auto flushIndex = [&]() {
        index.commit();
        index.beginTransaction();
    };
    for (const LastUploadedItem& item : items) {
        if (stopRequested_) break;
        if (item.modifiedSec < sinceSec) continue;
        if (item.type != QLatin1String("file")) continue;
        if (!isPathUnderSynced(index, syncRoot, item.relativePath)) continue;
        QString localPath = localRoot + item.relativePath;
        std::string apiPath = relativeToApiPath(item.relativePath);
        auto entry = index.get(syncRoot, item.relativePath);
        if (entry && entry->status == QLatin1String(FileStatus::TO_DELETE)) continue;
        qint64 localMtime = 0;
        qint64 localSize = 0;
        QFileInfo fi(localPath);
        if (fi.exists() && fi.isFile()) {
            localMtime = fi.lastModified().toSecsSinceEpoch();
            localSize = fi.size();
        }
        bool cloudNewer = item.modifiedSec > localMtime;
        bool localNewer = localMtime > item.modifiedSec;
        bool alreadySynced = entry && entry->status == QLatin1String(FileStatus::SYNCED)
                            && entry->size == item.size;
        if (cloudNewer && !alreadySynced) {
            logToFile(QStringLiteral("[Poll] cloud newer: ") + item.relativePath + QStringLiteral(" — downloading"));
            if (!QDir().mkpath(fi.absolutePath())) continue;
            if (!entry) index.upsertNew(syncRoot, item.relativePath, 0, 0);
            index.setStatus(syncRoot, item.relativePath, QString::fromUtf8(FileStatus::DOWNLOADING), 0);
            flushIndex();
            DiskResourceResult dr = client.downloadFile(apiPath, localPath);
            if (dr.success) {
                QFileInfo fi2(localPath);
                index.set(syncRoot, item.relativePath, fi2.lastModified().toSecsSinceEpoch(), fi2.size(),
                         QString::fromUtf8(FileStatus::SYNCED), 0);
                flushIndex();
                ++changesCount;
            } else {
                int newRetries = (entry ? entry->retries : 0) + 1;
                QString newStatus = (newRetries >= maxRetries)
                    ? QString::fromUtf8(FileStatus::FAILED)
                    : QString::fromUtf8(FileStatus::TO_DOWNLOAD);
                index.setStatus(syncRoot, item.relativePath, newStatus, 1);
                flushIndex();
            }
        } else if (localNewer && fi.exists() && fi.isFile()) {
            logToFile(QStringLiteral("[Poll] local newer: ") + item.relativePath + QStringLiteral(" — uploading"));
            DiskResourceResult dr = client.uploadFile(apiPath, localPath);
            if (dr.success) {
                bool wrote = false;
                if (!entry) {
                    index.upsertNew(syncRoot, item.relativePath, localMtime, localSize);
                    wrote = true;
                } else if (entry->status != QLatin1String(FileStatus::SYNCED)
                           || entry->mtime_sec != localMtime || entry->size != localSize) {
                    index.set(syncRoot, item.relativePath, localMtime, localSize, QString::fromUtf8(FileStatus::SYNCED), 0);
                    wrote = true;
                }
                if (wrote) { flushIndex(); ++changesCount; }
            }
        } else if (item.modifiedSec == localMtime) {
            if (fi.exists() && fi.isFile() && localSize == item.size) {
            } else if (!fi.exists() || !fi.isFile()) {
                logToFile(QStringLiteral("[Poll] missing locally: ") + item.relativePath + QStringLiteral(" — downloading"));
                if (!QDir().mkpath(fi.absolutePath())) continue;
                if (!entry) index.upsertNew(syncRoot, item.relativePath, 0, 0);
                index.setStatus(syncRoot, item.relativePath, QString::fromUtf8(FileStatus::DOWNLOADING), 0);
                flushIndex();
                DiskResourceResult dr = client.downloadFile(apiPath, localPath);
                if (dr.success) {
                    QFileInfo fi2(localPath);
                    index.set(syncRoot, item.relativePath, fi2.lastModified().toSecsSinceEpoch(), fi2.size(),
                             QString::fromUtf8(FileStatus::SYNCED), 0);
                    flushIndex();
                    ++changesCount;
                }
            }
        }
    }

    const int trashLimit = 300;
    const QString trashFields = QStringLiteral("path,origin_path,type,deleted,_embedded.items.path,_embedded.items.origin_path,_embedded.items.type,_embedded.items.deleted");
    int trashOffset = 0;
    for (;;) {
        if (stopRequested_) break;
        QUrlQuery trashQuery;
        trashQuery.addQueryItem(QStringLiteral("path"), QStringLiteral("/"));
        trashQuery.addQueryItem(QStringLiteral("limit"), QString::number(trashLimit));
        trashQuery.addQueryItem(QStringLiteral("offset"), QString::number(trashOffset));
        trashQuery.addQueryItem(QStringLiteral("sort"), QStringLiteral("-deleted"));
        trashQuery.addQueryItem(QStringLiteral("fields"), trashFields);
        auth::ApiResponse trashRes = apiClient->get("/trash/resources", trashQuery);
        QString trashUrl = QStringLiteral("/trash/resources");
        if (!trashQuery.isEmpty())
            trashUrl += QChar('?') + trashQuery.toString(QUrl::FullyEncoded);
        emit pollLog(QStringLiteral("[Poll] trash request: GET ") + trashUrl
                     + QStringLiteral(" → HTTP ") + QString::number(trashRes.statusCode));
        emit pollLog(QStringLiteral("[Poll] trash body: ") + QString::fromStdString(trashRes.body));
        if (!trashRes.ok()) break;
        QVector<TrashItem> trashItems = parseTrashJson(trashRes.body);
        if (trashItems.isEmpty()) break;
        bool pastSince = false;
        for (const TrashItem& ti : trashItems) {
            if (ti.deletedSec < sinceSec) {
                pastSince = true;
                break;
            }
            QString rel = ydisquette::cloudPathToRelativeQString(ti.originPath.toStdString());
            if (rel.isEmpty()) continue;
            if (!isPathUnderSynced(index, syncRoot, rel)) continue;
            if (ti.type == QLatin1String("dir")) {
                index.upsertNew(syncRoot, rel, 0, 0);
                index.setStatusPrefix(syncRoot, rel, QString::fromUtf8(FileStatus::CLOUD_DELETED));
            } else {
                index.setStatus(syncRoot, rel, QString::fromUtf8(FileStatus::CLOUD_DELETED), 0);
            }
            flushIndex();
            ++changesCount;
            logToFile(QStringLiteral("[Poll] cloud deleted: ") + rel);
        }
        if (pastSince || trashItems.size() < trashLimit) break;
        trashOffset += trashLimit;
    }

    index.commit();
    index.close();
    qint64 finishedAt = QDateTime::currentSecsSinceEpoch();
    pollRepo.updateRun(runId, QStringLiteral("completed"), finishedAt, changesCount, QString());
    pollRepo.close();
    emit pollCompleted(changesCount);
}

}  // namespace sync
}  // namespace ydisquette
