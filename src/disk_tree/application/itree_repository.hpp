#pragma once

#include "disk_tree/domain/node.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ydisquette {
namespace disk_tree {

struct ITreeRepository {
    virtual ~ITreeRepository() = default;
    virtual std::shared_ptr<Node> getRoot() = 0;
    virtual std::vector<std::shared_ptr<Node>> getChildren(const std::string& path) = 0;
    virtual void getChildrenAsync(const std::string& path,
                                  std::function<void(std::vector<std::shared_ptr<Node>>)> cb) = 0;
};

}  // namespace disk_tree
}  // namespace ydisquette
