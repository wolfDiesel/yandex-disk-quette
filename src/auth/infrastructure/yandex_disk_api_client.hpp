#pragma once

#include "auth/application/itoken_provider.hpp"
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrlQuery>
#include <functional>
#include <optional>
#include <string>

class QNetworkAccessManager;

namespace ydisquette {
namespace auth {

struct ApiResponse {
    int statusCode{};
    std::string body;
    bool ok() const { return statusCode >= 200 && statusCode < 300; }
};

class YandexDiskApiClient : public QObject {
    Q_OBJECT
public:
    explicit YandexDiskApiClient(ITokenProvider const& tokenProvider,
                                QNetworkAccessManager* nam,
                                QObject* parent = nullptr);
    ApiResponse get(const std::string& path, const QUrlQuery& query = QUrlQuery()) const;
    void getAsync(const std::string& path, const QUrlQuery& query,
                  std::function<void(ApiResponse)> cb) const;
    ApiResponse getByFullUrl(const QString& fullUrlWithQuery) const;
    void getByFullUrlAsync(const QString& fullUrlWithQuery,
                           std::function<void(ApiResponse)> cb) const;
    ApiResponse getAbsoluteUrl(const QString& absoluteUrl) const;
    void getAbsoluteUrlAsync(const QString& absoluteUrl,
                            std::function<void(ApiResponse)> cb) const;
    ApiResponse put(const std::string& path, const QByteArray& body = QByteArray()) const;
    ApiResponse putNoBody(const std::string& pathWithQuery) const;
    ApiResponse putToAbsoluteUrl(const QString& absoluteUrl, const QByteArray& body) const;
    ApiResponse deleteResource(const std::string& path) const;
    void deleteResourceAsync(const std::string& path,
                             std::function<void(ApiResponse)> cb) const;

    static const char kBaseUrl[];

private:
    ITokenProvider const& tokenProvider_;
    QNetworkAccessManager* nam_;
};

}  // namespace auth
}  // namespace ydisquette
