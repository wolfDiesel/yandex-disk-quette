#include "disk_tree/infrastructure/mock_tree_repository.hpp"

namespace ydisquette {
namespace disk_tree {

std::vector<std::shared_ptr<Node>> MockTreeRepository::getChildren(const std::string&) {
    if (!root_) return {};
    return root_->children;
}

void MockTreeRepository::getChildrenAsync(const std::string& path,
                                          std::function<void(std::vector<std::shared_ptr<Node>>)> cb) {
    cb(getChildren(path));
}

}  // namespace disk_tree
}  // namespace ydisquette
