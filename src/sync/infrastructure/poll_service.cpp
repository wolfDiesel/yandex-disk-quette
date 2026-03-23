#include "sync/infrastructure/poll_service.hpp"
#include "sync/infrastructure/poll_worker.hpp"
#include "auth/application/itoken_provider.hpp"
#include <QMetaType>

namespace ydisquette {
namespace sync {

PollService::PollService(auth::ITokenProvider const& tokenProvider, QObject* parent)
    : QObject(parent), tokenProvider_(tokenProvider) {
    qRegisterMetaType<int>("int");
    thread_ = new QThread(this);
    worker_ = new PollWorker(nullptr);
    worker_->moveToThread(thread_);
    connect(this, &PollService::startPollRequested, worker_, &PollWorker::doPoll, Qt::QueuedConnection);
    connect(worker_, &PollWorker::pollCompleted, this, &PollService::onPollCompleted, Qt::QueuedConnection);
    connect(worker_, &PollWorker::pollFailed, this, &PollService::onPollFailed, Qt::QueuedConnection);
    connect(worker_, &PollWorker::pollLog, this, &PollService::onPollLog, Qt::QueuedConnection);
    connect(worker_, &PollWorker::tokenExpired, this, &PollService::onTokenExpired, Qt::QueuedConnection);
    thread_->start();
}

PollService::~PollService() {
    if (thread_ && thread_->isRunning()) {
        thread_->quit();
        thread_->wait(2000);
    }
}

void PollService::startPoll(const QString& syncRoot, const QString& indexDbPath, int pollTimeSec, int maxRetries) {
    if (status_ == PollStatus::Polling) return;
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) return;
    status_ = PollStatus::Polling;
    emit startPollRequested(syncRoot, indexDbPath, QString::fromStdString(*token), pollTimeSec, maxRetries);
}

PollStatus PollService::getStatus() const {
    return status_;
}

void PollService::onPollCompleted(int changesCount) {
    status_ = PollStatus::Idle;
    emit pollCompleted(changesCount);
}

void PollService::onPollFailed(QString errorMessage) {
    status_ = PollStatus::Idle;
    emit pollFailed(errorMessage);
}

void PollService::onPollLog(QString message) {
    emit pollLog(message);
}

void PollService::onTokenExpired() {
    status_ = PollStatus::Idle;
    emit tokenExpired();
}

}  // namespace sync
}  // namespace ydisquette
