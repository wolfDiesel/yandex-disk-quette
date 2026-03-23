#pragma once

#include <QString>

namespace ydisquette {
namespace sync {

struct TrashItem {
    QString originPath;
    qint64 deletedSec = 0;
    QString type;
};

}  // namespace sync
}  // namespace ydisquette
