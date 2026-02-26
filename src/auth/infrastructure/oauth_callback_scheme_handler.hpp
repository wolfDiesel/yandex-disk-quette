#pragma once

#include <QObject>
#include <QWebEngineUrlSchemeHandler>

namespace ydisquette {
namespace auth {

class OAuthCallbackSchemeHandler : public QWebEngineUrlSchemeHandler {
    Q_OBJECT
public:
    explicit OAuthCallbackSchemeHandler(QObject* parent = nullptr);
    void requestStarted(QWebEngineUrlRequestJob* request) override;

signals:
    void codeReceived(const QString& code);
};

}  // namespace auth
}  // namespace ydisquette
