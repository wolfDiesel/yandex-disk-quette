#pragma once

#include "isettings_store.hpp"

namespace ydisquette {
namespace settings {

class SaveSettingsUseCase {
public:
    explicit SaveSettingsUseCase(ISettingsStore& store) : store_(store) {}
    void run(const AppSettings& s) { store_.save(s); }

private:
    ISettingsStore& store_;
};

}  // namespace settings
}  // namespace ydisquette
