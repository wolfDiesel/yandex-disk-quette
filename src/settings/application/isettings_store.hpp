#pragma once

#include "settings/domain/app_settings.hpp"

namespace ydisquette {
namespace settings {

struct ISettingsStore {
    virtual ~ISettingsStore() = default;
    virtual AppSettings load() = 0;
    virtual void save(const AppSettings&) = 0;
};

}  // namespace settings
}  // namespace ydisquette
