#pragma once

#include "sync/infrastructure/disk_resource_client.hpp"
#include <QString>
#include <functional>
#include <string>

namespace ydisquette {
namespace sync {

class DownloadFileUseCase {
public:
    explicit DownloadFileUseCase(DiskResourceClient& client) : client_(client) {}
    DiskResourceResult run(const std::string& remotePath, const QString& localPath) {
        return client_.downloadFile(remotePath, localPath);
    }
    void runAsync(const std::string& remotePath, const QString& localPath,
                  std::function<void(DiskResourceResult)> cb) {
        client_.downloadFileAsync(remotePath, localPath, std::move(cb));
    }

private:
    DiskResourceClient& client_;
};

}  // namespace sync
}  // namespace ydisquette
