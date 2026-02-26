#pragma once

#include "settings/application/isettings_store.hpp"

namespace ydisquette {
namespace settings {

class MockSettingsStore : public ISettingsStore {
public:
    AppSettings load() override { return settings_; }
    void save(const AppSettings& s) override { settings_ = s; }

private:
    AppSettings settings_;
};

}  // namespace settings
}  // namespace ydisquette
