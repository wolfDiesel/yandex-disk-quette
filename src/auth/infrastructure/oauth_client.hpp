#pragma once

#include "auth/infrastructure/token_store.hpp"
#include <QObject>
#include <QString>
#include <string>

class QNetworkAccessManager;

namespace ydisquette {
namespace auth {

class OAuthClient : public QObject {
    Q_OBJECT
public:
    explicit OAuthClient(TokenStore& store, QNetworkAccessManager* nam, QObject* parent = nullptr);
    bool exchangeCode(const QString& clientId, const QString& clientSecret,
                      const QString& redirectUri, const QString& code);
    bool refresh(const QString& clientId, const QString& clientSecret);

signals:
    void exchangeFailed(const QString& error);

private:
    TokenStore& store_;
    QNetworkAccessManager* nam_;
};

}  // namespace auth
}  // namespace ydisquette
