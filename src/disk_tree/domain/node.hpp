#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace ydisquette {
namespace disk_tree {

enum class NodeType { File, Dir };

struct Node {
    NodeType type{};
    std::string path;
    std::string name;
    int64_t size{};
    std::string modified;
    bool syncSelected{};

    std::vector<std::shared_ptr<Node>> children;

    static std::shared_ptr<Node> makeDir(std::string path, std::string name, std::string modified = {}) {
        auto n = std::make_shared<Node>();
        n->type = NodeType::Dir;
        n->path = std::move(path);
        n->name = std::move(name);
        n->modified = std::move(modified);
        return n;
    }

    static std::shared_ptr<Node> makeFile(std::string path, std::string name, int64_t size, std::string modified = {}) {
        auto n = std::make_shared<Node>();
        n->type = NodeType::File;
        n->path = std::move(path);
        n->name = std::move(name);
        n->size = size;
        n->modified = std::move(modified);
        return n;
    }

    bool isDir() const { return type == NodeType::Dir; }
    bool isFile() const { return type == NodeType::File; }
};

}  // namespace disk_tree
}  // namespace ydisquette
