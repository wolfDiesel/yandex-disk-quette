#pragma once

#include "sync/domain/trash_item.hpp"
#include <QVector>
#include <string>

namespace ydisquette {
namespace sync {

QVector<TrashItem> parseTrashJson(const std::string& body);

}  // namespace sync
}  // namespace ydisquette
