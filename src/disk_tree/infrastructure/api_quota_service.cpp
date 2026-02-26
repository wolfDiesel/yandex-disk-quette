#include "disk_tree/infrastructure/api_quota_service.hpp"
#include "disk_tree/infrastructure/api_parse.hpp"
#include "auth/infrastructure/yandex_disk_api_client.hpp"
#include <QUrlQuery>

namespace ydisquette {
namespace disk_tree {

ApiQuotaService::ApiQuotaService(auth::YandexDiskApiClient const& api) : api_(api) {}

Quota ApiQuotaService::getQuota() {
    auth::ApiResponse res = api_.get("");
    if (!res.ok()) return {};
    return parseDiskJson(res.body);
}

void ApiQuotaService::getQuotaAsync(std::function<void(Quota)> cb) {
    api_.getAsync("", QUrlQuery(), [cb](auth::ApiResponse res) {
        cb(res.ok() ? parseDiskJson(res.body) : Quota{});
    });
}

}  // namespace disk_tree
}  // namespace ydisquette
