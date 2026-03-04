#pragma once

#include <QNetworkAccessManager>

namespace ydisquette {
namespace auth {

class SslIgnoringNetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    explicit SslIgnoringNetworkAccessManager(QObject* parent = nullptr);

protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request,
                                 QIODevice* outgoingData) override;
};

}  // namespace auth
}  // namespace ydisquette
