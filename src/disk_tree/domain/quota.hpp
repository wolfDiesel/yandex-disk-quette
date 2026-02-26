#pragma once

#include <cstdint>

namespace ydisquette {
namespace disk_tree {

struct Quota {
    int64_t usedSpace{};
    int64_t totalSpace{};

    int64_t freeSpace() const {
        if (totalSpace <= 0) return 0;
        int64_t free = totalSpace - usedSpace;
        return free > 0 ? free : 0;
    }
};

}  // namespace disk_tree
}  // namespace ydisquette
