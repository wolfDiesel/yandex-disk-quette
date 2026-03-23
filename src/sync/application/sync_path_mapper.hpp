#pragma once

#include <QString>

namespace ydisquette {
namespace sync {

QString localPathToRelative(const QString& localPath, const QString& syncRoot);

}  // namespace sync
}  // namespace ydisquette
