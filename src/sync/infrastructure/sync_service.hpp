#pragma once

#include "sync/application/isync_service.hpp"
#include "sync/infrastructure/sync_index.hpp"
#include <QObject>
#include <QThread>

Q_DECLARE_METATYPE(std::string)
Q_DECLARE_METATYPE(std::vector<std::string>)
Q_DECLARE_METATYPE(ydisquette::sync::IndexState)

namespace ydisquette {
namespace auth {
class ITokenProvider;
}
namespace sync {

class SyncWorker;

class SyncService : public QObject, public ISyncService {
    Q_OBJECT
public:
    explicit SyncService(auth::ITokenProvider const& tokenProvider, QObject* parent = nullptr);
    ~SyncService() override;

    void startSync(const std::vector<std::string>& selectedPaths,
                   const std::string& syncPath,
                   const QString& indexDbPath = QString(),
                   int maxRetries = 3) override;
    void startSyncLocalToCloud(const std::vector<std::string>& selectedPaths,
                               const std::string& syncPath,
                               const QString& indexDbPath,
                               int maxRetries = 3);
    void stopSync() override;
    SyncStatus getStatus() const override;

    void startLoadIndexState(const QString& indexDbPath);

signals:
    void loadIndexStateRequested(const QString& indexDbPath);
    void startSyncRequested(const std::vector<std::string>& selectedPaths,
                            const std::string& syncPath,
                            const std::string& accessToken,
                            const QString& indexDbPath,
                            int maxRetries);
    void startSyncLocalToCloudRequested(const std::vector<std::string>& selectedPaths,
                                        const std::string& syncPath,
                                        const std::string& accessToken,
                                        const QString& indexDbPath,
                                        int maxRetries);
    void     statusChanged(SyncStatus status);
    void tokenExpired();
    void syncError(QString message);
    void syncProgressMessage(QString message);
    void indexStateLoaded(IndexState state);
    void pathsCreatedInCloud(const std::vector<std::string>& cloudPaths);
    void stopRequested();

private slots:
    void onWorkerStatusChanged(SyncStatus status);
    void onWorkerTokenExpired();
    void onWorkerSyncError(QString message);
    void onWorkerSyncProgressMessage(QString message);
    void onLocalToCloudFinished(const std::vector<std::string>& selectedPaths,
                                const std::string& syncPath,
                                const std::string& accessToken);

private:
    auth::ITokenProvider const& tokenProvider_;
    QString lastIndexDbPath_;
    int lastMaxRetries_ = 3;
    QThread* thread_ = nullptr;
    SyncWorker* worker_ = nullptr;
    SyncStatus status_{SyncStatus::Idle};
    QString lastSyncError_;
};

}  // namespace sync
}  // namespace ydisquette
