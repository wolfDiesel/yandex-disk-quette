#pragma once

#include "sync/infrastructure/disk_resource_client.hpp"
#include <functional>
#include <string>

namespace ydisquette {
namespace sync {

class DeleteResourceUseCase {
public:
    explicit DeleteResourceUseCase(DiskResourceClient& client) : client_(client) {}
    DiskResourceResult run(const std::string& path) { return client_.deleteResource(path); }
    void runAsync(const std::string& path, std::function<void(DiskResourceResult)> cb) {
        client_.deleteResourceAsync(path, std::move(cb));
    }

private:
    DiskResourceClient& client_;
};

}  // namespace sync
}  // namespace ydisquette
