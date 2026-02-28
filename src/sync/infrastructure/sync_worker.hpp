#pragma once

#include "sync/domain/sync_status.hpp"
#include "sync/infrastructure/sync_index.hpp"
#include <QObject>
#include <atomic>
#include <string>
#include <vector>

namespace ydisquette {
namespace sync {

class SyncWorker : public QObject {
    Q_OBJECT
public:
    explicit SyncWorker(QObject* parent = nullptr);

public slots:
    void doSync(const std::vector<std::string>& selectedPaths, const std::string& syncPath,
               const std::string& accessToken, const QString& indexDbPath = QString(), int maxRetries = 3);
    void doSyncLocalToCloud(const std::vector<std::string>& selectedPaths, const std::string& syncPath,
                            const std::string& accessToken, const QString& indexDbPath, int maxRetries = 3);
    void loadIndexState(const QString& indexDbPath);
    void requestStop();

signals:
    void statusChanged(SyncStatus status);
    void tokenExpired();
    void syncError(QString message);
    void syncProgressMessage(QString message);
    void syncThroughput(qint64 bytesPerSecond);
    void localToCloudFinished(const std::vector<std::string>& selectedPaths,
                              const std::string& syncPath,
                              const std::string& accessToken);
    void pathsCreatedInCloud(const std::vector<std::string>& cloudPaths);
    void indexStateLoaded(IndexState state);

private:
    std::atomic<bool> stopRequested_{false};
};

}  // namespace sync
}  // namespace ydisquette
