#pragma once

#include "disk_tree/application/iquota_service.hpp"

namespace ydisquette {
namespace disk_tree {

class MockQuotaService : public IQuotaService {
public:
    void setQuota(Quota q) { quota_ = q; }
    Quota getQuota() override { return quota_; }
    void getQuotaAsync(std::function<void(Quota)> cb) override { cb(quota_); }

private:
    Quota quota_;
};

}  // namespace disk_tree
}  // namespace ydisquette
