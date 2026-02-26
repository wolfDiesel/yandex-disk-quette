#include "sync/infrastructure/sync_index.hpp"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>

namespace ydisquette {
namespace sync {

IndexState readIndexState(const QString& dbPath) {
    IndexState out;
    if (dbPath.isEmpty()) return out;
    QFileInfo fi(dbPath);
    if (!fi.exists()) {
        out.summary = QStringLiteral("(no file)");
        return out;
    }
    QString connName = QStringLiteral("sync_index_read_") + QString::number(QDateTime::currentMSecsSinceEpoch());
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            out.summary = QStringLiteral("(open failed)");
            QSqlDatabase::removeDatabase(connName);
            return out;
        }
        QSqlQuery q(db);
        if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM sync_state"))) {
            out.summary = QStringLiteral("(query failed)");
            QSqlDatabase::removeDatabase(connName);
            return out;
        }
        if (q.next())
            out.totalEntries = q.value(0).toInt();
        QStringList parts;
        if (q.exec(QStringLiteral("SELECT sync_root, COUNT(*) FROM sync_state GROUP BY sync_root"))) {
            while (q.next())
                parts.append(q.value(0).toString() + QLatin1Char(':') + q.value(1).toString());
        }
        out.summary = parts.isEmpty() ? QString::number(out.totalEntries) : parts.join(QStringLiteral("; "));
    }
    QSqlDatabase::removeDatabase(connName);
    return out;
}

QString normalizeSyncRoot(const QString& syncPath) {
    QString s = syncPath.trimmed();
    if (s.isEmpty()) return s;
    s = QDir::cleanPath(QDir(s).absolutePath());
    while (s.endsWith(QLatin1Char('/')) && s.size() > 1)
        s.chop(1);
    return s;
}

static QString normalizeRelativePath(QString path) {
    path = path.trimmed();
    while (path.startsWith(QLatin1Char('/')))
        path = path.mid(1);
    return path;
}

SyncIndex::~SyncIndex() {
    close();
}

static bool ensureStatusColumns(QSqlQuery& q) {
    if (!q.exec(QStringLiteral("PRAGMA table_info(sync_state)"))) return false;
    bool hasStatus = false;
    bool hasRetries = false;
    while (q.next()) {
        QString name = q.value(1).toString();
        if (name == QLatin1String("status")) hasStatus = true;
        if (name == QLatin1String("retries")) hasRetries = true;
    }
    if (!hasStatus && !q.exec(QStringLiteral("ALTER TABLE sync_state ADD COLUMN status TEXT NOT NULL DEFAULT 'SYNCED'")))
        return false;
    if (!hasRetries && !q.exec(QStringLiteral("ALTER TABLE sync_state ADD COLUMN retries INTEGER NOT NULL DEFAULT 0")))
        return false;
    return true;
}

bool SyncIndex::open(const QString& dbPath) {
    if (!connectionName_.isEmpty())
        return true;
    QFileInfo fi(dbPath);
    QDir().mkpath(fi.absolutePath());
    connectionName_ = QStringLiteral("sync_index_") + QString::number(reinterpret_cast<quintptr>(this));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
        db.setDatabaseName(dbPath);
        if (!db.open())
            return false;
    }
    QSqlQuery q(queryDb());
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS sync_state ("
            "sync_root TEXT NOT NULL, relative_path TEXT NOT NULL,"
            "mtime_sec INTEGER NOT NULL, size INTEGER NOT NULL, updated_at INTEGER,"
            "status TEXT NOT NULL DEFAULT 'SYNCED', retries INTEGER NOT NULL DEFAULT 0,"
            "PRIMARY KEY (sync_root, relative_path))")))
        return false;
    if (!ensureStatusColumns(q)) return false;
    return true;
}

void SyncIndex::close() {
    if (connectionName_.isEmpty()) return;
    QSqlDatabase::removeDatabase(connectionName_);
    connectionName_.clear();
}

QSqlDatabase SyncIndex::queryDb() const {
    return QSqlDatabase::database(connectionName_);
}

std::optional<SyncIndexEntry> SyncIndex::get(const QString& syncRoot, const QString& relativePath) const {
    if (connectionName_.isEmpty()) return std::nullopt;
    QString rel = normalizeRelativePath(relativePath);
    QSqlQuery q(queryDb());
    q.prepare(QStringLiteral("SELECT mtime_sec, size, status, retries, updated_at FROM sync_state WHERE sync_root = ? AND relative_path = ?"));
    q.addBindValue(syncRoot);
    q.addBindValue(rel);
    if (!q.exec() || !q.next())
        return std::nullopt;
    SyncIndexEntry e;
    e.mtime_sec = q.value(0).toLongLong();
    e.size = q.value(1).toLongLong();
    e.status = q.value(2).toString();
    if (e.status.isEmpty()) e.status = QStringLiteral("SYNCED");
    e.retries = q.value(3).toInt();
    e.updated_at_sec = q.value(4).toLongLong();
    return e;
}

bool SyncIndex::hasAnyWithPrefix(const QString& syncRoot, const QString& relativePathPrefix) const {
    if (connectionName_.isEmpty()) return false;
    QString prefix = normalizeRelativePath(relativePathPrefix);
    if (prefix.isEmpty()) return true;
    QSqlQuery q(queryDb());
    q.prepare(QStringLiteral("SELECT 1 FROM sync_state WHERE sync_root = ? AND (relative_path = ? OR relative_path LIKE ?) LIMIT 1"));
    q.addBindValue(syncRoot);
    q.addBindValue(prefix);
    q.addBindValue(prefix + QLatin1Char('/') + QLatin1Char('%'));
    return q.exec() && q.next();
}

bool SyncIndex::set(const QString& syncRoot, const QString& relativePath, qint64 mtimeSec, qint64 size,
                    const QString& status, int retries) {
    if (connectionName_.isEmpty()) return false;
    QString rel = normalizeRelativePath(relativePath);
    qint64 now = QDateTime::currentSecsSinceEpoch();
    QString st = status.isEmpty() ? QStringLiteral("SYNCED") : status;
    int r = (retries >= 0) ? retries : 0;
    QSqlQuery q(queryDb());
    q.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO sync_state (sync_root, relative_path, mtime_sec, size, updated_at, status, retries) VALUES (?, ?, ?, ?, ?, ?, ?)"));
    q.addBindValue(syncRoot);
    q.addBindValue(rel);
    q.addBindValue(mtimeSec);
    q.addBindValue(size);
    q.addBindValue(now);
    q.addBindValue(st);
    q.addBindValue(r);
    return q.exec();
}

bool SyncIndex::setStatus(const QString& syncRoot, const QString& relativePath, const QString& status,
                          int retriesDelta) {
    if (connectionName_.isEmpty()) return false;
    QString rel = normalizeRelativePath(relativePath);
    qint64 now = QDateTime::currentSecsSinceEpoch();
    QSqlQuery q(queryDb());
    if (retriesDelta != 0) {
        q.prepare(QStringLiteral(
                "UPDATE sync_state SET status = ?, retries = retries + ?, updated_at = ? WHERE sync_root = ? AND relative_path = ?"));
        q.addBindValue(status);
        q.addBindValue(retriesDelta);
        q.addBindValue(now);
        q.addBindValue(syncRoot);
        q.addBindValue(rel);
    } else {
        q.prepare(QStringLiteral(
                "UPDATE sync_state SET status = ?, updated_at = ? WHERE sync_root = ? AND relative_path = ?"));
        q.addBindValue(status);
        q.addBindValue(now);
        q.addBindValue(syncRoot);
        q.addBindValue(rel);
    }
    return q.exec();
}

bool SyncIndex::upsertNew(const QString& syncRoot, const QString& relativePath, qint64 mtimeSec, qint64 size) {
    return set(syncRoot, relativePath, mtimeSec, size, QStringLiteral("NEW"), 0);
}

bool SyncIndex::remove(const QString& syncRoot, const QString& relativePath) {
    if (connectionName_.isEmpty()) return false;
    QString rel = normalizeRelativePath(relativePath);
    QSqlQuery q(queryDb());
    q.prepare(QStringLiteral("DELETE FROM sync_state WHERE sync_root = ? AND relative_path = ?"));
    q.addBindValue(syncRoot);
    q.addBindValue(rel);
    return q.exec();
}

bool SyncIndex::removePrefix(const QString& syncRoot, const QString& relativePathPrefix) {
    if (connectionName_.isEmpty()) return false;
    QString prefix = normalizeRelativePath(relativePathPrefix);
    QString likePrefix = prefix.isEmpty() || prefix.endsWith(QLatin1Char('/')) ? prefix : prefix + QLatin1Char('/');
    QSqlQuery q(queryDb());
    q.prepare(QStringLiteral("DELETE FROM sync_state WHERE sync_root = ? AND (relative_path = ? OR relative_path LIKE ?)"));
    q.addBindValue(syncRoot);
    q.addBindValue(prefix);
    q.addBindValue(likePrefix + QStringLiteral("%"));
    return q.exec();
}

QStringList SyncIndex::getTopLevelRelativePaths(const QString& syncRoot) const {
    QStringList out;
    if (connectionName_.isEmpty()) return out;
    QSqlQuery q(queryDb());
    q.prepare(QStringLiteral(
            "SELECT DISTINCT CASE WHEN instr(relative_path, '/') > 0 THEN substr(relative_path, 1, instr(relative_path, '/') - 1) ELSE relative_path END FROM sync_state WHERE sync_root = ?"));
    q.addBindValue(syncRoot);
    if (!q.exec()) return out;
    while (q.next()) {
        QString part = q.value(0).toString().trimmed();
        if (!part.isEmpty()) out.append(part);
    }
    return out;
}

bool SyncIndex::beginTransaction() {
    if (connectionName_.isEmpty()) return false;
    return queryDb().transaction();
}

bool SyncIndex::commit() {
    if (connectionName_.isEmpty()) return false;
    return queryDb().commit();
}

bool SyncIndex::rollback() {
    if (connectionName_.isEmpty()) return false;
    return queryDb().rollback();
}

}  // namespace sync
}  // namespace ydisquette
