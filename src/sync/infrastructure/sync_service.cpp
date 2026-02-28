#include "sync/infrastructure/sync_service.hpp"
#include "sync/infrastructure/sync_worker.hpp"
#include "auth/application/itoken_provider.hpp"
#include <QMetaType>
#include <QMetaObject>
#include <Qt>

namespace ydisquette {
namespace sync {

SyncService::SyncService(auth::ITokenProvider const& tokenProvider, QObject* parent)
    : QObject(parent), tokenProvider_(tokenProvider) {
    qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<SyncStatus>("ydisquette::sync::SyncStatus");
    qRegisterMetaType<IndexState>("ydisquette::sync::IndexState");
    thread_ = new QThread(this);
    worker_ = new SyncWorker(nullptr);
    worker_->moveToThread(thread_);
    connect(this, &SyncService::startSyncRequested, worker_, &SyncWorker::doSync, Qt::QueuedConnection);
    connect(this, &SyncService::startSyncLocalToCloudRequested, worker_, &SyncWorker::doSyncLocalToCloud, Qt::QueuedConnection);
    connect(this, &SyncService::loadIndexStateRequested, worker_, &SyncWorker::loadIndexState, Qt::QueuedConnection);
    connect(worker_, &SyncWorker::indexStateLoaded, this, &SyncService::indexStateLoaded, Qt::QueuedConnection);
    connect(worker_, &SyncWorker::pathsCreatedInCloud, this, &SyncService::pathsCreatedInCloud, Qt::QueuedConnection);
    connect(worker_, &SyncWorker::localToCloudFinished, this, &SyncService::onLocalToCloudFinished, Qt::QueuedConnection);
    connect(worker_, &SyncWorker::statusChanged, this, &SyncService::onWorkerStatusChanged, Qt::QueuedConnection);
    connect(worker_, &SyncWorker::tokenExpired, this, &SyncService::onWorkerTokenExpired, Qt::QueuedConnection);
    connect(worker_, &SyncWorker::syncError, this, &SyncService::onWorkerSyncError, Qt::QueuedConnection);
    connect(worker_, &SyncWorker::syncProgressMessage, this, &SyncService::onWorkerSyncProgressMessage, Qt::QueuedConnection);
    connect(worker_, &SyncWorker::syncThroughput, this, &SyncService::onWorkerSyncThroughput, Qt::QueuedConnection);
    connect(this, &SyncService::stopRequested, worker_, &SyncWorker::requestStop, Qt::QueuedConnection);
    thread_->start();
}

SyncService::~SyncService() {
    if (thread_ && thread_->isRunning()) {
        thread_->quit();
        thread_->wait(2000);
    }
}

void SyncService::startSync(const std::vector<std::string>& selectedPaths,
                            const std::string& syncPath,
                            const QString& indexDbPath,
                            int maxRetries) {
    if (status_ == SyncStatus::Syncing) return;
    lastMaxRetries_ = maxRetries;
    status_ = SyncStatus::Syncing;
    emit statusChanged(SyncStatus::Syncing);
    auto token = tokenProvider_.getAccessToken();
    std::string tokenStr = (token && !token->empty()) ? *token : std::string();
    emit startSyncRequested(selectedPaths, syncPath, tokenStr, indexDbPath, maxRetries);
}

void SyncService::startSyncLocalToCloud(const std::vector<std::string>& selectedPaths,
                                         const std::string& syncPath,
                                         const QString& indexDbPath,
                                         int maxRetries) {
    if (selectedPaths.empty() || syncPath.empty()) return;
    if (status_ == SyncStatus::Syncing) return;
    lastIndexDbPath_ = indexDbPath;
    lastMaxRetries_ = maxRetries;
    status_ = SyncStatus::Syncing;
    emit statusChanged(SyncStatus::Syncing);
    auto token = tokenProvider_.getAccessToken();
    std::string tokenStr = (token && !token->empty()) ? *token : std::string();
    if (tokenStr.empty()) {
        status_ = SyncStatus::Idle;
        emit statusChanged(SyncStatus::Idle);
        return;
    }
    emit startSyncLocalToCloudRequested(selectedPaths, syncPath, tokenStr, indexDbPath, maxRetries);
}

void SyncService::startLoadIndexState(const QString& indexDbPath) {
    emit loadIndexStateRequested(indexDbPath);
}

void SyncService::stopSync() {
    if (status_ == SyncStatus::Syncing)
        emit stopRequested();
}

SyncStatus SyncService::getStatus() const {
    return status_;
}

void SyncService::onLocalToCloudFinished(const std::vector<std::string>& selectedPaths,
                                         const std::string& syncPath,
                                         const std::string& accessToken) {
    emit startSyncRequested(selectedPaths, syncPath, accessToken, lastIndexDbPath_, lastMaxRetries_);
}

void SyncService::onWorkerStatusChanged(SyncStatus status) {
    status_ = status;
    emit statusChanged(status);
}

void SyncService::onWorkerTokenExpired() {
    emit tokenExpired();
}

void SyncService::onWorkerSyncError(QString message) {
    lastSyncError_ = message;
    emit syncError(message);
}

void SyncService::onWorkerSyncProgressMessage(QString message) {
    emit syncProgressMessage(message);
}

void SyncService::onWorkerSyncThroughput(qint64 bytesPerSecond) {
    emit syncThroughput(bytesPerSecond);
}

}  // namespace sync
}  // namespace ydisquette
