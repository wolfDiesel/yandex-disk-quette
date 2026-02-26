#include "auth/ui/login_widget.hpp"
#include "auth/infrastructure/oauth_client.hpp"
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QLabel>
#include <QWebEngineView>
#include <QWebEnginePage>

namespace ydisquette {
namespace auth {

struct LoginWidget::Private {
    OAuthClient& oauthClient;
    QString clientId;
    QString clientSecret;
    QString redirectUri;
    QWebEngineView* webView = nullptr;
    QLabel* statusLabel = nullptr;
    bool exchangeDone = false;

    Private(OAuthClient& oauth, const QString& id, const QString& secret, const QString& redirect)
        : oauthClient(oauth), clientId(id), clientSecret(secret), redirectUri(redirect) {}
};

LoginWidget::~LoginWidget() = default;

LoginWidget::LoginWidget(OAuthClient& oauthClient,
                         const QString& clientId,
                         const QString& clientSecret,
                         const QString& redirectUri,
                         QWidget* parent)
    : QWidget(parent), d_(std::make_unique<Private>(oauthClient, clientId, clientSecret, redirectUri)) {
    setWindowTitle(tr("Y.Disquette — Sign in"));
    auto* layout = new QVBoxLayout(this);
    d_->statusLabel = new QLabel(this);
    d_->statusLabel->hide();
    layout->addWidget(d_->statusLabel);
    d_->webView = new QWebEngineView(this);
    d_->webView->setMinimumHeight(400);
    layout->addWidget(d_->webView, 1);
    connect(d_->webView->page(), &QWebEnginePage::loadFinished, this, &LoginWidget::onLoadFinished);
}

void LoginWidget::startLogin() {
    d_->exchangeDone = false;
    QUrl url(QStringLiteral("https://oauth.yandex.ru/authorize"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    q.addQueryItem(QStringLiteral("client_id"), d_->clientId);
    q.addQueryItem(QStringLiteral("redirect_uri"), d_->redirectUri);
    q.addQueryItem(QStringLiteral("scope"), QStringLiteral("cloud_api:disk.read cloud_api:disk.write cloud_api:disk.info"));
    url.setQuery(q);
    d_->statusLabel->setText(tr("Откройте страницу входа в браузере ниже."));
    d_->webView->setUrl(url);
}

void LoginWidget::onCodeReceived(const QString& code) {
    if (d_->exchangeDone) return;
    d_->exchangeDone = true;
    bool ok = d_->oauthClient.exchangeCode(d_->clientId, d_->clientSecret, d_->redirectUri, code);
    if (ok)
        emit loggedIn();
    else
        emit loginFailed(tr("Failed to get token."));
}

void LoginWidget::onLoadFinished(bool ok) {
    (void)ok;
}

}  // namespace auth
}  // namespace ydisquette
