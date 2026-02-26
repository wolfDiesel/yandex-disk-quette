#pragma once

#include "settings/application/iselected_nodes_store.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace ydisquette {
namespace settings {

class MockSelectedNodesStore : public ISelectedNodesStore {
public:
    std::vector<std::string> getSelectedPaths() override { return paths_; }
    void setSelectedPaths(std::vector<std::string> paths) override { paths_ = std::move(paths); }
    void togglePath(const std::string& path) override {
        auto it = std::find(paths_.begin(), paths_.end(), path);
        if (it == paths_.end())
            paths_.push_back(path);
        else
            paths_.erase(it);
    }

private:
    std::vector<std::string> paths_;
};

}  // namespace settings
}  // namespace ydisquette
