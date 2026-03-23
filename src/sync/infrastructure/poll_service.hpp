#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <atomic>
#include <memory>

namespace ydisquette {
namespace auth {
class ITokenProvider;
}
namespace sync {

class PollWorker;

enum class PollStatus { Idle, Polling };

class PollService : public QObject {
    Q_OBJECT
public:
    explicit PollService(auth::ITokenProvider const& tokenProvider, QObject* parent = nullptr);
    ~PollService() override;

    void startPoll(const QString& syncRoot, const QString& indexDbPath, int pollTimeSec, int maxRetries);
    PollStatus getStatus() const;

signals:
    void startPollRequested(const QString& syncRoot, const QString& indexDbPath,
                            const QString& accessToken, int pollTimeSec, int maxRetries);
    void pollCompleted(int changesCount);
    void pollFailed(QString errorMessage);
    void pollLog(QString message);
    void tokenExpired();

private slots:
    void onPollCompleted(int changesCount);
    void onPollFailed(QString errorMessage);
    void onPollLog(QString message);
    void onTokenExpired();

private:
    auth::ITokenProvider const& tokenProvider_;
    QThread* thread_ = nullptr;
    PollWorker* worker_ = nullptr;
    std::atomic<PollStatus> status_{PollStatus::Idle};
};

}  // namespace sync
}  // namespace ydisquette
