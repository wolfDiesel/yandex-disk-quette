#include "sync/infrastructure/sqlite_poll_run_repository.hpp"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <optional>

namespace ydisquette {
namespace sync {

SqlitePollRunRepository::~SqlitePollRunRepository() {
    close();
}

bool SqlitePollRunRepository::open(const QString& dbPath) {
    if (!connectionName_.isEmpty())
        return true;
    connectionName_ = QStringLiteral("poll_run_") + QString::number(reinterpret_cast<quintptr>(this));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db.setDatabaseName(dbPath);
    return db.open();
}

void SqlitePollRunRepository::close() {
    if (connectionName_.isEmpty()) return;
    QSqlDatabase::removeDatabase(connectionName_);
    connectionName_.clear();
}

qint64 SqlitePollRunRepository::insert(const QString& syncRoot, qint64 startedAt) {
    if (connectionName_.isEmpty()) return 0;
    QSqlQuery q(QSqlDatabase::database(connectionName_));
    q.prepare(QStringLiteral(
        "INSERT INTO poll_run (started_at, status, changes_count, sync_root) VALUES (?, 'running', 0, ?)"));
    q.addBindValue(startedAt);
    q.addBindValue(syncRoot);
    if (!q.exec()) return 0;
    qint64 id = q.lastInsertId().toLongLong();
    if (id > 0)
        pruneKeepingLast(3000);
    return id > 0 ? id : 0;
}

bool SqlitePollRunRepository::updateRun(qint64 id, const QString& status,
                                        std::optional<qint64> finishedAt,
                                        int changesCount, const QString& errorMessage) {
    if (connectionName_.isEmpty()) return false;
    QSqlQuery q(QSqlDatabase::database(connectionName_));
    q.prepare(QStringLiteral(
        "UPDATE poll_run SET status = ?, finished_at = ?, changes_count = ?, error_message = ? WHERE id = ?"));
    q.addBindValue(status);
    q.addBindValue(finishedAt.has_value() ? QVariant::fromValue(*finishedAt) : QVariant());
    q.addBindValue(changesCount);
    q.addBindValue(errorMessage.isEmpty() ? QVariant() : QVariant(errorMessage));
    q.addBindValue(id);
    return q.exec();
}

bool SqlitePollRunRepository::pruneKeepingLast(int n) {
    if (connectionName_.isEmpty() || n <= 0) return true;
    QSqlQuery q(QSqlDatabase::database(connectionName_));
    q.prepare(QStringLiteral(
        "DELETE FROM poll_run WHERE id IN (SELECT id FROM poll_run ORDER BY id DESC LIMIT -1 OFFSET ?)"));
    q.addBindValue(n);
    return q.exec();
}

std::optional<qint64> SqlitePollRunRepository::getLastFinishedAt(const QString& syncRoot) const {
    if (connectionName_.isEmpty()) return std::nullopt;
    QSqlQuery q(QSqlDatabase::database(connectionName_));
    q.prepare(QStringLiteral(
        "SELECT finished_at FROM poll_run WHERE sync_root = ? AND finished_at IS NOT NULL ORDER BY id DESC LIMIT 1"));
    q.addBindValue(syncRoot);
    if (!q.exec() || !q.next()) return std::nullopt;
    return q.value(0).toLongLong();
}

}  // namespace sync
}  // namespace ydisquette
