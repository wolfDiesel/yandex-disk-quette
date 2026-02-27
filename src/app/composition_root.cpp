#include "composition_root.hpp"
#include "oauth_credentials.hpp"
#include "json_config.hpp"
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QByteArray>
#include <QString>

namespace ydisquette {

CompositionRoot::CompositionRoot() {
    nam_ = std::make_unique<QNetworkAccessManager>();
    tokenStore_ = std::make_unique<auth::TokenStore>();
    oauthClient_ = std::make_unique<auth::OAuthClient>(*tokenStore_, nam_.get(), nullptr);
    apiClient_ = std::make_unique<auth::YandexDiskApiClient>(*tokenStore_, nam_.get(), nullptr);
    treeRepo_ = std::make_unique<disk_tree::ApiTreeRepository>(*apiClient_);
    quotaService_ = std::make_unique<disk_tree::ApiQuotaService>(*apiClient_);
    settingsStore_ = std::make_unique<settings::MockSettingsStore>();
    selectedStore_ = std::make_unique<settings::MockSelectedNodesStore>();
    diskResourceClient_ = std::make_unique<sync::DiskResourceClient>(*apiClient_);

    loadTree_ = std::make_unique<disk_tree::LoadTreeUseCase>(*treeRepo_);
    getQuota_ = std::make_unique<disk_tree::GetQuotaUseCase>(*quotaService_);
    getSettings_ = std::make_unique<settings::GetSettingsUseCase>(*settingsStore_);
    saveSettings_ = std::make_unique<settings::SaveSettingsUseCase>(*settingsStore_);
    toggleNode_ = std::make_unique<settings::ToggleNodeSelectionUseCase>(*selectedStore_);
    downloadFile_ = std::make_unique<sync::DownloadFileUseCase>(*diskResourceClient_);
    deleteResource_ = std::make_unique<sync::DeleteResourceUseCase>(*diskResourceClient_);
    syncService_ = std::make_unique<sync::SyncService>(*tokenStore_);
}

QString CompositionRoot::getSyncIndexDbPath() const {
    return JsonConfig::syncIndexDbPath();
}

bool CompositionRoot::refreshToken() {
    QString clientId, clientSecret, redirectUri;
    getBuildTimeOAuthCredentials(&clientId, &clientSecret, &redirectUri);
    const QString envId = QString::fromUtf8(qgetenv("YDISQUETTE_CLIENT_ID"));
    const QString envSecret = QString::fromUtf8(qgetenv("YDISQUETTE_CLIENT_SECRET"));
    if (!envId.isEmpty()) clientId = envId;
    if (!envSecret.isEmpty()) clientSecret = envSecret;
    if (clientId.isEmpty() || clientSecret.isEmpty())
        return false;
    return oauthClient_->refresh(clientId, clientSecret);
}

}  // namespace ydisquette
