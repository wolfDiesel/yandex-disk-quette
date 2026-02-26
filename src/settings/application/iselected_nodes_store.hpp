#pragma once

#include <string>
#include <vector>

namespace ydisquette {
namespace settings {

struct ISelectedNodesStore {
    virtual ~ISelectedNodesStore() = default;
    virtual std::vector<std::string> getSelectedPaths() = 0;
    virtual void setSelectedPaths(std::vector<std::string> paths) = 0;
    virtual void togglePath(const std::string& path) = 0;
};

}  // namespace settings
}  // namespace ydisquette
