#pragma once

#include "sync/domain/sync_file_status.hpp"
#include "sync/infrastructure/sync_index.hpp"
#include <disk_tree/application/itree_repository.hpp>
#include <functional>
#include <QString>
#include <string>
#include <vector>

namespace ydisquette {
namespace sync {

class ScanAndFillIndexUseCase {
public:
    enum class Result { Success, Stopped, IndexError };

    static Result run(disk_tree::ITreeRepository& treeRepo,
                      SyncIndex& index,
                      const QString& syncRoot,
                      const std::vector<std::string>& selectedPaths,
                      std::function<bool()> stopRequested);
};

}  // namespace sync
}  // namespace ydisquette
