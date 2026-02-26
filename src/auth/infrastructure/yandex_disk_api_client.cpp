#include "auth/infrastructure/yandex_disk_api_client.hpp"
#include <QEventLoop>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <functional>

namespace ydisquette {
namespace auth {

const char YandexDiskApiClient::kBaseUrl[] = "https://cloud-api.yandex.net/v1/disk";

YandexDiskApiClient::YandexDiskApiClient(ITokenProvider const& tokenProvider,
                                         QNetworkAccessManager* nam,
                                         QObject* parent)
    : QObject(parent), tokenProvider_(tokenProvider), nam_(nam) {}

ApiResponse YandexDiskApiClient::get(const std::string& path, const QUrlQuery& query) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        return {401, ""};
    }

    QUrl url(QString::fromStdString(std::string(kBaseUrl) + path));
    if (!query.isEmpty()) url.setQuery(query);

    QNetworkRequest req(url);
    req.setTransferTimeout(30000);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());

    QNetworkReply* reply = nam_->get(req);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    std::string body = reply->readAll().toStdString();
    if (reply->error() != QNetworkReply::NoError && body.empty())
        body = reply->errorString().toStdString();
    reply->deleteLater();
    return {status, body};
}

ApiResponse YandexDiskApiClient::getByFullUrl(const QString& fullUrlWithQuery) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        return {401, ""};
    }
    QUrl url(fullUrlWithQuery);

    QNetworkRequest req(url);
    req.setTransferTimeout(30000);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());
    QNetworkReply* reply = nam_->get(req);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    std::string body = reply->readAll().toStdString();
    if (reply->error() != QNetworkReply::NoError && body.empty())
        body = reply->errorString().toStdString();
    reply->deleteLater();
    return {status, body};
}

void YandexDiskApiClient::getByFullUrlAsync(const QString& fullUrlWithQuery,
                                            std::function<void(ApiResponse)> cb) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        if (cb) cb({401, ""});
        return;
    }
    QUrl url(fullUrlWithQuery);
    QNetworkRequest req(url);
    req.setTransferTimeout(30000);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());
    QNetworkReply* reply = nam_->get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        std::string body = reply->readAll().toStdString();
        if (reply->error() != QNetworkReply::NoError && body.empty())
            body = reply->errorString().toStdString();
        reply->deleteLater();
        if (cb) cb({status, body});
    });
}

void YandexDiskApiClient::getAsync(const std::string& path, const QUrlQuery& query,
                                    std::function<void(ApiResponse)> cb) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        if (cb) cb({401, ""});
        return;
    }
    QUrl url(QString::fromStdString(std::string(kBaseUrl) + path));
    if (!query.isEmpty()) url.setQuery(query);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());
    QNetworkReply* reply = nam_->get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const std::string body = reply->readAll().toStdString();
        reply->deleteLater();
        if (cb) cb({status, body});
    });
}

ApiResponse YandexDiskApiClient::put(const std::string& path, const QByteArray& body) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        return {401, ""};
    }

    QUrl url(QString::fromStdString(std::string(kBaseUrl) + path));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());

    QNetworkReply* reply = nam_->put(req, body);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const std::string replyBody = reply->readAll().toStdString();
    reply->deleteLater();
    return {status, replyBody};
}

ApiResponse YandexDiskApiClient::putNoBody(const std::string& pathWithQuery) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        return {401, ""};
    }
    QUrl url(QString::fromStdString(std::string(kBaseUrl) + pathWithQuery));
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());

    QNetworkReply* reply = nam_->put(req, QByteArray());
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    std::string replyBody = reply->readAll().toStdString();
    if (reply->error() != QNetworkReply::NoError && replyBody.empty())
        replyBody = reply->errorString().toStdString();
    reply->deleteLater();
    return {status, replyBody};
}

ApiResponse YandexDiskApiClient::getAbsoluteUrl(const QString& absoluteUrl) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        return {401, ""};
    }
    QUrl u(absoluteUrl);
    QNetworkRequest req(u);
    req.setRawHeader("Accept", "application/octet-stream");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());

    QNetworkReply* reply = nam_->get(req);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const std::string body = reply->readAll().toStdString();
    reply->deleteLater();
    return {status, body};
}

void YandexDiskApiClient::getAbsoluteUrlAsync(const QString& absoluteUrl,
                                               std::function<void(ApiResponse)> cb) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        if (cb) cb({401, ""});
        return;
    }
    QUrl u(absoluteUrl);
    QNetworkRequest req(u);
    req.setRawHeader("Accept", "application/octet-stream");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());
    QNetworkReply* reply = nam_->get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const std::string body = reply->readAll().toStdString();
        reply->deleteLater();
        if (cb) cb({status, body});
    });
}

ApiResponse YandexDiskApiClient::putToAbsoluteUrl(const QString& absoluteUrl, const QByteArray& body) const {
    QUrl u(absoluteUrl);
    QNetworkRequest req(u);
    req.setTransferTimeout(900000);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/octet-stream"));

    QNetworkReply* reply = nam_->put(req, body);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    std::string replyBody = reply->readAll().toStdString();
    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        if (err.isEmpty()) err = QStringLiteral("Network error (%1)").arg(reply->error());
        if (replyBody.empty() || status == 0) replyBody = err.toStdString();
    }
    reply->deleteLater();
    return {status, replyBody};
}

ApiResponse YandexDiskApiClient::deleteResource(const std::string& path) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        return {401, ""};
    }

    QUrl url(QString::fromStdString(std::string(kBaseUrl) + path));
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());

    QNetworkReply* reply = nam_->deleteResource(req);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const std::string body = reply->readAll().toStdString();
    reply->deleteLater();
    return {status, body};
}

void YandexDiskApiClient::deleteResourceAsync(const std::string& path,
                                               std::function<void(ApiResponse)> cb) const {
    auto token = tokenProvider_.getAccessToken();
    if (!token || token->empty()) {
        if (cb) cb({401, ""});
        return;
    }
    QUrl url(QString::fromStdString(std::string(kBaseUrl) + path));
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("Authorization", ("OAuth " + *token).c_str());
    QNetworkReply* reply = nam_->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const std::string body = reply->readAll().toStdString();
        reply->deleteLater();
        if (cb) cb({status, body});
    });
}

}  // namespace auth
}  // namespace ydisquette
