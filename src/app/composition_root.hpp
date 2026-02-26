#pragma once

#include <auth/application/itoken_provider.hpp>
#include <auth/infrastructure/oauth_client.hpp>
#include <auth/infrastructure/token_store.hpp>
#include <auth/infrastructure/yandex_disk_api_client.hpp>
#include <disk_tree/application/get_quota_use_case.hpp>
#include <disk_tree/application/itree_repository.hpp>
#include <disk_tree/application/load_tree_use_case.hpp>
#include <disk_tree/infrastructure/api_quota_service.hpp>
#include <disk_tree/infrastructure/api_tree_repository.hpp>
#include <settings/application/get_settings_use_case.hpp>
#include <settings/application/save_settings_use_case.hpp>
#include <settings/application/toggle_node_selection_use_case.hpp>
#include <settings/infrastructure/mock_selected_nodes_store.hpp>
#include <settings/infrastructure/mock_settings_store.hpp>
#include <sync/application/delete_resource_use_case.hpp>
#include <sync/application/download_file_use_case.hpp>
#include <sync/infrastructure/disk_resource_client.hpp>
#include <sync/infrastructure/sync_service.hpp>
#include <memory>
#include <vector>

#include <QNetworkAccessManager>

namespace ydisquette {

struct CompositionRoot {
    CompositionRoot();

    auth::ITokenProvider const& tokenProvider() const { return *tokenStore_; }
    auth::TokenStore& tokenStore() { return *tokenStore_; }
    auth::OAuthClient& oauthClient() { return *oauthClient_; }
    disk_tree::LoadTreeUseCase& loadTreeUseCase() { return *loadTree_; }
    disk_tree::ITreeRepository& treeRepository() { return *treeRepo_; }
    disk_tree::GetQuotaUseCase& getQuotaUseCase() { return *getQuota_; }
    settings::GetSettingsUseCase& getSettingsUseCase() { return *getSettings_; }
    settings::SaveSettingsUseCase& saveSettingsUseCase() { return *saveSettings_; }
    settings::ToggleNodeSelectionUseCase& toggleNodeSelectionUseCase() { return *toggleNode_; }
    std::vector<std::string> getSelectedPaths() const { return selectedStore_->getSelectedPaths(); }
    void setSelectedPaths(std::vector<std::string> paths) { selectedStore_->setSelectedPaths(std::move(paths)); }
    QString getSyncIndexDbPath() const;
    sync::DownloadFileUseCase& downloadFileUseCase() { return *downloadFile_; }
    sync::DeleteResourceUseCase& deleteResourceUseCase() { return *deleteResource_; }
    sync::SyncService& syncService() { return *syncService_; }

    bool refreshToken();

private:
    std::unique_ptr<QNetworkAccessManager> nam_;
    std::unique_ptr<auth::TokenStore> tokenStore_;
    std::unique_ptr<auth::OAuthClient> oauthClient_;
    std::unique_ptr<auth::YandexDiskApiClient> apiClient_;
    std::unique_ptr<disk_tree::ApiTreeRepository> treeRepo_;
    std::unique_ptr<disk_tree::ApiQuotaService> quotaService_;
    std::unique_ptr<settings::MockSettingsStore> settingsStore_;
    std::unique_ptr<settings::MockSelectedNodesStore> selectedStore_;
    std::unique_ptr<disk_tree::LoadTreeUseCase> loadTree_;
    std::unique_ptr<disk_tree::GetQuotaUseCase> getQuota_;
    std::unique_ptr<settings::GetSettingsUseCase> getSettings_;
    std::unique_ptr<settings::SaveSettingsUseCase> saveSettings_;
    std::unique_ptr<settings::ToggleNodeSelectionUseCase> toggleNode_;
    std::unique_ptr<sync::DiskResourceClient> diskResourceClient_;
    std::unique_ptr<sync::DownloadFileUseCase> downloadFile_;
    std::unique_ptr<sync::DeleteResourceUseCase> deleteResource_;
    std::unique_ptr<sync::SyncService> syncService_;
};

}  // namespace ydisquette
