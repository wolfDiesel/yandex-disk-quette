#pragma once

#include "sync/domain/sync_status.hpp"
#include <QString>
#include <string>
#include <vector>

namespace ydisquette {
namespace sync {

struct ISyncService {
    virtual ~ISyncService() = default;
    virtual void startSync(const std::vector<std::string>& selectedPaths,
                          const std::string& syncPath,
                          const QString& indexDbPath = QString(),
                          int maxRetries = 3) = 0;
    virtual void stopSync() = 0;
    virtual SyncStatus getStatus() const = 0;
};

}  // namespace sync
}  // namespace ydisquette
