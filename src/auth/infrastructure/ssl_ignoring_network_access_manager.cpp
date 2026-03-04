#include "auth/infrastructure/ssl_ignoring_network_access_manager.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>

namespace ydisquette {
namespace auth {

SslIgnoringNetworkAccessManager::SslIgnoringNetworkAccessManager(QObject* parent)
    : QNetworkAccessManager(parent) {}

QNetworkReply* SslIgnoringNetworkAccessManager::createRequest(Operation op,
                                                              const QNetworkRequest& request,
                                                              QIODevice* outgoingData) {
    QNetworkReply* reply = QNetworkAccessManager::createRequest(op, request, outgoingData);
    connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError>&) {
        reply->ignoreSslErrors();
    });
    return reply;
}

}  // namespace auth
}  // namespace ydisquette
