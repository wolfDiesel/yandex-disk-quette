#pragma once

#include "isettings_store.hpp"

namespace ydisquette {
namespace settings {

class GetSettingsUseCase {
public:
    explicit GetSettingsUseCase(ISettingsStore& store) : store_(store) {}
    AppSettings run() { return store_.load(); }

private:
    ISettingsStore& store_;
};

}  // namespace settings
}  // namespace ydisquette
