#pragma once

#include "sync/domain/last_uploaded_item.hpp"
#include <QVector>
#include <string>

namespace ydisquette {
namespace sync {

QVector<LastUploadedItem> parseLastUploadedJson(const std::string& body);

}  // namespace sync
}  // namespace ydisquette
