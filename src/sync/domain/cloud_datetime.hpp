#pragma once

#include <QDateTime>
#include <QtGlobal>
#include <string>

namespace ydisquette {
namespace sync {

QDateTime parseCloudModified(const std::string& modified);

inline qint64 parseCloudModifiedToSec(const std::string& modified) {
    QDateTime dt = parseCloudModified(modified);
    return dt.isValid() ? dt.toSecsSinceEpoch() : 0;
}

}  // namespace sync
}  // namespace ydisquette
