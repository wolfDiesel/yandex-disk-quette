#pragma once

#include <QObject>
#include <QString>
#include <atomic>
#include <string>

namespace ydisquette {
namespace sync {

class PollWorker : public QObject {
    Q_OBJECT
public:
    explicit PollWorker(QObject* parent = nullptr);

public slots:
    void doPoll(const QString& syncRoot, const QString& indexDbPath,
                const QString& accessToken, int pollTimeSec, int maxRetries);

signals:
    void pollCompleted(int changesCount);
    void pollFailed(QString errorMessage);
    void pollLog(QString message);
    void tokenExpired();

private:
    std::atomic<bool> stopRequested_{false};
};

}  // namespace sync
}  // namespace ydisquette
