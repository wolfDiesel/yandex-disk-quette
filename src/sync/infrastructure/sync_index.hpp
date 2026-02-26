#pragma once

#include "sync/domain/sync_file_status.hpp"
#include <QString>
#include <QSqlDatabase>
#include <optional>

namespace ydisquette {
namespace sync {

struct SyncIndexEntry {
    qint64 mtime_sec = 0;
    qint64 size = 0;
    QString status = FileStatus::SYNCED;
    int retries = 0;
    qint64 updated_at_sec = 0;
};

struct IndexState {
    int totalEntries = 0;
    QString summary;
};

QString normalizeSyncRoot(const QString& syncPath);
IndexState readIndexState(const QString& dbPath);

class SyncIndex {
public:
    SyncIndex() = default;
    ~SyncIndex();

    bool open(const QString& dbPath);
    void close();

    std::optional<SyncIndexEntry> get(const QString& syncRoot, const QString& relativePath) const;
    bool hasAnyWithPrefix(const QString& syncRoot, const QString& relativePathPrefix) const;
    bool set(const QString& syncRoot, const QString& relativePath, qint64 mtimeSec, qint64 size,
             const QString& status = QString(), int retries = -1);
    bool setStatus(const QString& syncRoot, const QString& relativePath, const QString& status,
                   int retriesDelta = 0);
    bool upsertNew(const QString& syncRoot, const QString& relativePath, qint64 mtimeSec, qint64 size);
    bool remove(const QString& syncRoot, const QString& relativePath);
    bool removePrefix(const QString& syncRoot, const QString& relativePathPrefix);

    QStringList getTopLevelRelativePaths(const QString& syncRoot) const;

    bool beginTransaction();
    bool commit();
    bool rollback();

private:
    QSqlDatabase queryDb() const;

    QString connectionName_;
};

}  // namespace sync
}  // namespace ydisquette
