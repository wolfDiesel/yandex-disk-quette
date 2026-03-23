#pragma once

#include "sync/domain/sync_file_status.hpp"
#include "sync/infrastructure/disk_resource_client.hpp"
#include "sync/infrastructure/sync_index.hpp"
#include <disk_tree/application/itree_repository.hpp>
#include <functional>
#include <QString>
#include <string>
#include <vector>

namespace ydisquette {
namespace sync {

struct SyncLocalToCloudCallbacks {
    std::function<void(QString)> onProgressMessage;
    std::function<void(QString)> onError;
    std::function<void(qint64)> onThroughput;
    std::function<void(const std::vector<std::string>&)> onPathsCreatedInCloud;
};

class SyncLocalToCloudUseCase {
public:
    enum class Result { Success, Stopped, Error };

    static Result run(disk_tree::ITreeRepository& treeRepo,
                     DiskResourceClient& diskClient,
                     SyncIndex* index,
                     const QString& syncRoot,
                     const QString& localRoot,
                     const std::vector<std::string>& selectedPaths,
                     int maxRetries,
                     std::function<bool()> stopRequested,
                     const SyncLocalToCloudCallbacks& callbacks);
};

}  // namespace sync
}  // namespace ydisquette
