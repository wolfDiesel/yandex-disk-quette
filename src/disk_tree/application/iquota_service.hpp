#pragma once

#include "disk_tree/domain/quota.hpp"
#include <functional>

namespace ydisquette {
namespace disk_tree {

struct IQuotaService {
    virtual ~IQuotaService() = default;
    virtual Quota getQuota() = 0;
    virtual void getQuotaAsync(std::function<void(Quota)> cb) = 0;
};

}  // namespace disk_tree
}  // namespace ydisquette
