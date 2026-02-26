#pragma once

#include "iquota_service.hpp"
#include <functional>

namespace ydisquette {
namespace disk_tree {

class GetQuotaUseCase {
public:
    explicit GetQuotaUseCase(IQuotaService& service) : service_(service) {}
    Quota run() { return service_.getQuota(); }
    void runAsync(std::function<void(Quota)> cb) { service_.getQuotaAsync(std::move(cb)); }

private:
    IQuotaService& service_;
};

}  // namespace disk_tree
}  // namespace ydisquette
