#pragma once

#include "sync/domain/cloud_datetime.hpp"
#include <disk_tree/domain/node.hpp>
#include <QFileInfo>
#include <QString>

namespace ydisquette {
namespace sync {

bool cloudNewerThanLocal(const disk_tree::Node* node, const QString& localPath);
bool localNewerThanCloud(const disk_tree::Node* node, const QString& localPath);

}  // namespace sync
}  // namespace ydisquette
