#pragma once

#include "iselected_nodes_store.hpp"

namespace ydisquette {
namespace settings {

class ToggleNodeSelectionUseCase {
public:
    explicit ToggleNodeSelectionUseCase(ISelectedNodesStore& store) : store_(store) {}
    void run(const std::string& path) { store_.togglePath(path); }

private:
    ISelectedNodesStore& store_;
};

}  // namespace settings
}  // namespace ydisquette
