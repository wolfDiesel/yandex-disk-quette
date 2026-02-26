#pragma once

#include "disk_tree/application/itree_repository.hpp"
#include "disk_tree/domain/node.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ydisquette {
namespace disk_tree {

class MockTreeRepository : public ITreeRepository {
public:
    void setRoot(std::shared_ptr<Node> root) { root_ = std::move(root); }
    std::shared_ptr<Node> getRoot() override { return root_; }
    std::vector<std::shared_ptr<Node>> getChildren(const std::string& path) override;
    void getChildrenAsync(const std::string& path,
                         std::function<void(std::vector<std::shared_ptr<Node>>)> cb) override;

private:
    std::shared_ptr<Node> root_;
};

}  // namespace disk_tree
}  // namespace ydisquette
