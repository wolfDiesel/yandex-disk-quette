#pragma once

#include "disk_tree/application/iquota_service.hpp"
#include "disk_tree/domain/quota.hpp"
#include <functional>

namespace ydisquette {
namespace auth {
class YandexDiskApiClient;
}
namespace disk_tree {

class ApiQuotaService : public IQuotaService {
public:
    explicit ApiQuotaService(auth::YandexDiskApiClient const& api);
    Quota getQuota() override;
    void getQuotaAsync(std::function<void(Quota)> cb) override;

private:
    auth::YandexDiskApiClient const& api_;
};

}  // namespace disk_tree
}  // namespace ydisquette
