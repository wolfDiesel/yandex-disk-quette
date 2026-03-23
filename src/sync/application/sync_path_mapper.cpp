#include "sync/application/sync_path_mapper.hpp"

namespace ydisquette {
namespace sync {

QString localPathToRelative(const QString& localPath, const QString& syncRoot) {
    QString base = syncRoot + QLatin1Char('/');
    if (!localPath.startsWith(base)) return QString();
    return localPath.mid(base.size());
}

}  // namespace sync
}  // namespace ydisquette
