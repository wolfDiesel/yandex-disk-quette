#pragma once

#include <QString>
#include <functional>
#include <string>

namespace ydisquette {
namespace auth {
class YandexDiskApiClient;
}
namespace sync {

struct DiskResourceResult {
    bool success{};
    int httpStatus{};
    QString errorMessage;
};

class DiskResourceClient {
public:
    explicit DiskResourceClient(auth::YandexDiskApiClient const& api);
    DiskResourceResult createFolder(const std::string& path);
    DiskResourceResult downloadFile(const std::string& remotePath, const QString& localPath);
    void downloadFileAsync(const std::string& remotePath, const QString& localPath,
                          std::function<void(DiskResourceResult)> cb);
    DiskResourceResult uploadFile(const std::string& remotePath, const QString& localPath);
    DiskResourceResult deleteResource(const std::string& path);
    void deleteResourceAsync(const std::string& path,
                             std::function<void(DiskResourceResult)> cb);

private:
    auth::YandexDiskApiClient const& api_;
};

}  // namespace sync
}  // namespace ydisquette
