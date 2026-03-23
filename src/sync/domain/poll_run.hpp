#pragma once

#include <QString>
#include <optional>

namespace ydisquette {
namespace sync {

struct PollRun {
    qint64 id = 0;
    qint64 started_at = 0;
    QString status;
    std::optional<qint64> finished_at;
    int changes_count = 0;
    QString sync_root;
    QString error_message;
};

}  // namespace sync
}  // namespace ydisquette
