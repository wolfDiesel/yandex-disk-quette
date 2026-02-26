#pragma once

#include "disk_tree/application/itree_repository.hpp"
#include "disk_tree/domain/node.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ydisquette {
namespace auth {
class YandexDiskApiClient;
}
namespace disk_tree {

class ApiTreeRepository : public ITreeRepository {
public:
    explicit ApiTreeRepository(auth::YandexDiskApiClient const& api);
    std::shared_ptr<Node> getRoot() override;
    std::vector<std::shared_ptr<Node>> getChildren(const std::string& path) override;
    void getChildrenAsync(const std::string& path,
                          std::function<void(std::vector<std::shared_ptr<Node>>)> cb) override;

private:
    auth::YandexDiskApiClient const& api_;
};

}  // namespace disk_tree
}  // namespace ydisquette
