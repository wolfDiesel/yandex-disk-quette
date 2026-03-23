#pragma once

#include "sync/domain/poll_run.hpp"
#include <QString>
#include <optional>

namespace ydisquette {
namespace sync {

struct IPollRunRepository {
    virtual ~IPollRunRepository() = default;
    virtual bool open(const QString& dbPath) = 0;
    virtual void close() = 0;
    virtual qint64 insert(const QString& syncRoot, qint64 startedAt) = 0;
    virtual bool updateRun(qint64 id, const QString& status, std::optional<qint64> finishedAt,
                          int changesCount, const QString& errorMessage) = 0;
    virtual bool pruneKeepingLast(int n) = 0;
    virtual std::optional<qint64> getLastFinishedAt(const QString& syncRoot) const = 0;
};

}  // namespace sync
}  // namespace ydisquette
