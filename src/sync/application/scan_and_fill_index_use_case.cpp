#include "sync/application/scan_and_fill_index_use_case.hpp"
#include "shared/cloud_path_util.hpp"
#include "sync/domain/cloud_datetime.hpp"
#include "shared/app_log.hpp"
#include <QDateTime>

namespace ydisquette {
namespace sync {

ScanAndFillIndexUseCase::Result ScanAndFillIndexUseCase::run(
    disk_tree::ITreeRepository& treeRepo,
    SyncIndex& index,
    const QString& syncRoot,
    const std::vector<std::string>& selectedPaths,
    std::function<bool()> stopRequested) {
    std::function<bool(const std::string&)> scanFolder = [&](const std::string& cloudPath) -> bool {
        if (stopRequested && stopRequested()) return false;
        std::vector<std::shared_ptr<disk_tree::Node>> children = treeRepo.getChildren(cloudPath);
        for (const auto& node : children) {
            if (stopRequested && stopRequested()) return false;
            if (!node) continue;
            std::string childPath = node->path;
            if (node->isDir()) {
                if (!scanFolder(childPath)) return false;
            } else {
                QString rel = cloudPathToRelativeQString(childPath);
                if (rel.isEmpty()) continue;
                auto entry = index.get(syncRoot, rel);
                if (!entry) {
                    qint64 mtimeSec = 0;
                    if (!node->modified.empty()) {
                        QDateTime dt = parseCloudModified(node->modified);
                        if (dt.isValid()) mtimeSec = dt.toSecsSinceEpoch();
                    }
                    qint64 size = static_cast<qint64>(node->size);
                    index.set(syncRoot, rel, mtimeSec, size, QString::fromUtf8(FileStatus::TO_DOWNLOAD), 0);
                }
            }
        }
        if (!index.commit() || !index.beginTransaction()) {
            ydisquette::logToFile(QStringLiteral("[Sync] scan index flush failed"));
            return false;
        }
        return true;
    };
    for (const std::string& cloudPath : selectedPaths) {
        if (stopRequested && stopRequested()) return Result::Stopped;
        if (!scanFolder(cloudPath)) return Result::Stopped;
    }
    if (!index.commit())
        return Result::IndexError;
    return Result::Success;
}

}  // namespace sync
}  // namespace ydisquette
