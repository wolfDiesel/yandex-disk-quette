#pragma once

#include "auth/infrastructure/token_holder.hpp"
#include "auth/infrastructure/ssl_ignoring_network_access_manager.hpp"
#include "auth/infrastructure/yandex_disk_api_client.hpp"
#include "disk_tree/infrastructure/api_tree_repository.hpp"
#include "sync/infrastructure/disk_resource_client.hpp"
#include <memory>
#include <string>

namespace ydisquette {
namespace sync {

struct SyncInfrastructure {
    auth::TokenHolder tokenHolder;
    std::unique_ptr<auth::SslIgnoringNetworkAccessManager> nam;
    std::unique_ptr<auth::YandexDiskApiClient> apiClient;
    std::unique_ptr<DiskResourceClient> diskClient;
    std::unique_ptr<disk_tree::ApiTreeRepository> treeRepo;
};

struct SyncInfrastructureFactory {
    static void create(const std::string& accessToken, SyncInfrastructure& out);
};

}  // namespace sync
}  // namespace ydisquette
