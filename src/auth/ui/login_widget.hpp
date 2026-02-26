#pragma once

#include <QString>
#include <QWidget>
#include <memory>

namespace ydisquette {
namespace auth {

class OAuthClient;

class LoginWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoginWidget(OAuthClient& oauthClient,
                        const QString& clientId,
                        const QString& clientSecret,
                        const QString& redirectUri,
                        QWidget* parent = nullptr);
    ~LoginWidget() override;
    void startLogin();

public slots:
    void onCodeReceived(const QString& code);

signals:
    void loggedIn();
    void loginFailed(const QString& error);

private:
    void onLoadFinished(bool ok);

    class Private;
    std::unique_ptr<Private> d_;
};

}  // namespace auth
}  // namespace ydisquette
