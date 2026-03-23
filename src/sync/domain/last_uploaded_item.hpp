#pragma once

#include <QString>

namespace ydisquette {
namespace sync {

struct LastUploadedItem {
    QString relativePath;
    qint64 modifiedSec = 0;
    qint64 size = 0;
    QString type;
};

}  // namespace sync
}  // namespace ydisquette
