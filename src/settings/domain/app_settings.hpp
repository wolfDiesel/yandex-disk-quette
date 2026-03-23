#pragma once

#include <string>

namespace ydisquette {
namespace settings {

struct AppSettings {
    std::string syncPath;
    int maxRetries = 3;
    int refreshIntervalSec = 60;
    int pollTimeSec = 120;
    bool hideToTray = true;
    bool closeToTray = true;
};

}  // namespace settings
}  // namespace ydisquette
