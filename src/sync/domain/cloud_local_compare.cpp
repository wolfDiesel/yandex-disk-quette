#include "sync/domain/cloud_local_compare.hpp"

namespace ydisquette {
namespace sync {

bool cloudNewerThanLocal(const disk_tree::Node* node, const QString& localPath) {
    if (!node || node->modified.empty()) return false;
    QDateTime cloudDt = parseCloudModified(node->modified);
    if (!cloudDt.isValid()) return false;
    qint64 cloudSec = cloudDt.toSecsSinceEpoch();
    qint64 localSec = QFileInfo(localPath).lastModified().toSecsSinceEpoch();
    return cloudSec > localSec;
}

bool localNewerThanCloud(const disk_tree::Node* node, const QString& localPath) {
    if (!node || node->modified.empty()) return true;
    QDateTime cloudDt = parseCloudModified(node->modified);
    if (!cloudDt.isValid()) return true;
    qint64 cloudSec = cloudDt.toSecsSinceEpoch();
    qint64 localSec = QFileInfo(localPath).lastModified().toSecsSinceEpoch();
    return localSec > cloudSec;
}

}  // namespace sync
}  // namespace ydisquette
