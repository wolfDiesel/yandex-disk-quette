#pragma once

#include <QStringList>

namespace ydisquette {
namespace sync {

struct ToDeleteBatches {
    QStringList rootFiles;
    QStringList minimalFolders;
};

ToDeleteBatches computeToDeleteBatches(const QStringList& toDeletePaths);

}  // namespace sync
}  // namespace ydisquette
