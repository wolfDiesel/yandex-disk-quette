#pragma once

#include <string>

namespace ydisquette {
namespace settings {

struct AppSettings {
    std::string syncPath;
    int maxRetries = 3;
    int cloudCheckIntervalSec = 30;
    int refreshIntervalSec = 60;
    bool hideToTray = true;
    bool closeToTray = true;
};

}  // namespace settings
}  // namespace ydisquette
