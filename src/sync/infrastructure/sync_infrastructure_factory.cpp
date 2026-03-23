#include "sync/infrastructure/sync_infrastructure_factory.hpp"

namespace ydisquette {
namespace sync {

void SyncInfrastructureFactory::create(const std::string& accessToken, SyncInfrastructure& out) {
    out.tokenHolder.token = accessToken;
    out.nam = std::make_unique<auth::SslIgnoringNetworkAccessManager>();
    out.apiClient = std::make_unique<auth::YandexDiskApiClient>(
        out.tokenHolder, out.nam.get(), nullptr);
    out.diskClient = std::make_unique<DiskResourceClient>(*out.apiClient);
    out.treeRepo = std::make_unique<disk_tree::ApiTreeRepository>(*out.apiClient);
}

}  // namespace sync
}  // namespace ydisquette
