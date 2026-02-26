#include "auth/infrastructure/oauth_client.hpp"
#include <QByteArray>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

namespace ydisquette {
namespace auth {

static const char kTokenUrl[] = "https://oauth.yandex.ru/token";

OAuthClient::OAuthClient(TokenStore& store, QNetworkAccessManager* nam, QObject* parent)
    : QObject(parent), store_(store), nam_(nam) {}

bool OAuthClient::exchangeCode(const QString& clientId, const QString& clientSecret,
                               const QString& redirectUri, const QString& code) {
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    query.addQueryItem(QStringLiteral("code"), code);
    query.addQueryItem(QStringLiteral("client_id"), clientId);
    query.addQueryItem(QStringLiteral("client_secret"), clientSecret);
    query.addQueryItem(QStringLiteral("redirect_uri"), redirectUri);

    QUrl url(kTokenUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));

    QNetworkReply* reply = nam_->post(req, query.toString(QUrl::FullyEncoded).toUtf8());
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    reply->deleteLater();

    if (status != 200) {
        emit exchangeFailed(QString::fromUtf8(body));
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        emit exchangeFailed(QStringLiteral("Invalid JSON"));
        return false;
    }
    QJsonObject obj = doc.object();
    QString access = obj.value(QStringLiteral("access_token")).toString();
    QString refresh = obj.value(QStringLiteral("refresh_token")).toString();
    if (access.isEmpty()) {
        emit exchangeFailed(QStringLiteral("No access_token in response"));
        return false;
    }
    store_.setTokens(access.toStdString(), refresh.toStdString());
    return true;
}

bool OAuthClient::refresh(const QString& clientId, const QString& clientSecret) {
    auto ref = store_.getRefreshToken();
    if (!ref || ref->empty()) return false;

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    query.addQueryItem(QStringLiteral("refresh_token"), QString::fromStdString(*ref));
    query.addQueryItem(QStringLiteral("client_id"), clientId);
    query.addQueryItem(QStringLiteral("client_secret"), clientSecret);

    QUrl url(kTokenUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));

    QNetworkReply* reply = nam_->post(req, query.toString(QUrl::FullyEncoded).toUtf8());
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    reply->deleteLater();

    if (status != 200) return false;

    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) return false;
    QJsonObject obj = doc.object();
    QString access = obj.value(QStringLiteral("access_token")).toString();
    QString newRefresh = obj.value(QStringLiteral("refresh_token")).toString();
    if (access.isEmpty()) return false;
    store_.setTokens(access.toStdString(), newRefresh.isEmpty() ? *ref : newRefresh.toStdString());
    return true;
}

}  // namespace auth
}  // namespace ydisquette
