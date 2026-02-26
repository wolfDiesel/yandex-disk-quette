#pragma once

#include <QMetaType>

namespace ydisquette {
namespace sync {

enum class SyncStatus { Idle, Syncing, Error };

}  // namespace sync
}  // namespace ydisquette

Q_DECLARE_METATYPE(ydisquette::sync::SyncStatus)
