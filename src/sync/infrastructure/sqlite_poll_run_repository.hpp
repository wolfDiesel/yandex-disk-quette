#pragma once

#include "sync/application/ipoll_run_repository.hpp"
#include <QString>
#include <optional>

namespace ydisquette {
namespace sync {

class SqlitePollRunRepository : public IPollRunRepository {
public:
    SqlitePollRunRepository() = default;
    ~SqlitePollRunRepository() override;

    bool open(const QString& dbPath) override;
    void close() override;
    qint64 insert(const QString& syncRoot, qint64 startedAt) override;
    bool updateRun(qint64 id, const QString& status, std::optional<qint64> finishedAt,
                   int changesCount, const QString& errorMessage) override;
    bool pruneKeepingLast(int n) override;
    std::optional<qint64> getLastFinishedAt(const QString& syncRoot) const override;

private:
    QString connectionName_;
};

}  // namespace sync
}  // namespace ydisquette
