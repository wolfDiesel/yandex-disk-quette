#include "auth/infrastructure/oauth_callback_scheme_handler.hpp"
#include <QBuffer>
#include <QUrlQuery>
#include <QWebEngineUrlRequestJob>

namespace ydisquette {
namespace auth {

OAuthCallbackSchemeHandler::OAuthCallbackSchemeHandler(QObject* parent)
    : QWebEngineUrlSchemeHandler(parent) {}

void OAuthCallbackSchemeHandler::requestStarted(QWebEngineUrlRequestJob* request) {
    const QUrl url = request->requestUrl();
    if (url.scheme() != QLatin1String("ydisquette") || url.host() != QLatin1String("callback")) {
        request->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }
    QUrlQuery query(url.query());
    QString code = query.queryItemValue(QStringLiteral("code"));
    if (code.isEmpty()) {
        request->fail(QWebEngineUrlRequestJob::UrlInvalid);
        return;
    }
    emit codeReceived(code);
    const QByteArray html = QByteArrayLiteral(
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\"></head><body>"
        "<p>Успешно. Получение токена…</p></body></html>");
    auto* device = new QBuffer;
    device->setData(html);
    device->open(QIODevice::ReadOnly);
    connect(request, &QObject::destroyed, device, &QObject::deleteLater);
    request->reply(QByteArrayLiteral("text/html; charset=utf-8"), device);
}

}  // namespace auth
}  // namespace ydisquette
