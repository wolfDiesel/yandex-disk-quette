#pragma once

#include "disk_tree/domain/node.hpp"
#include "disk_tree/domain/quota.hpp"
#include <string>
#include <vector>
#include <memory>

namespace ydisquette {
namespace disk_tree {

Quota parseDiskJson(const std::string& body);
std::vector<std::shared_ptr<Node>> parseResourcesJson(const std::string& body);

}  // namespace disk_tree
}  // namespace ydisquette
