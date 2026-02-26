#pragma once

#include <QString>

namespace ydisquette {
namespace sync {

namespace FileStatus {
constexpr const char* NEW = "NEW";
constexpr const char* SYNCED = "SYNCED";
constexpr const char* DOWNLOADING = "DOWNLOADING";
constexpr const char* UPLOADING = "UPLOADING";
constexpr const char* FAILED = "FAILED";
inline bool isTerminal(const QString& s) {
    return s == QLatin1String(SYNCED) || s == QLatin1String(FAILED);
}
inline bool needsUpload(const QString& s) {
    return s == QLatin1String(NEW) || s == QLatin1String(UPLOADING);
}
inline bool needsDownload(const QString& s) {
    return s == QLatin1String(DOWNLOADING);
}
}  // namespace FileStatus

}  // namespace sync
}  // namespace ydisquette
