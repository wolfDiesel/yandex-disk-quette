#pragma once

#include "itree_repository.hpp"
#include <memory>

namespace ydisquette {
namespace disk_tree {

class LoadTreeUseCase {
public:
    explicit LoadTreeUseCase(ITreeRepository& repo) : repo_(repo) {}
    std::shared_ptr<Node> run() { return repo_.getRoot(); }

private:
    ITreeRepository& repo_;
};

}  // namespace disk_tree
}  // namespace ydisquette
