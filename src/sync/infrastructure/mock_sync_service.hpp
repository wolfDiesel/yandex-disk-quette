#pragma once

#include "sync/application/isync_service.hpp"

namespace ydisquette {
namespace sync {

class MockSyncService : public ISyncService {
public:
    void setStatus(SyncStatus s) { status_ = s; }
    void startSync(const std::vector<std::string>&, const std::string&, const QString& = QString(), int = 3) override { status_ = SyncStatus::Syncing; }
    void stopSync() override { status_ = SyncStatus::Idle; }
    SyncStatus getStatus() const override { return status_; }

private:
    SyncStatus status_{SyncStatus::Idle};
};

}  // namespace sync
}  // namespace ydisquette
