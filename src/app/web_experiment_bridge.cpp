#include "web_experiment_bridge.hpp"
#include "main_content_widget.hpp"

namespace ydisquette {

WebExperimentBridge::WebExperimentBridge(MainContentWidget* mainContent, QObject* parent)
    : QObject(parent), mainContent_(mainContent) {}

QString WebExperimentBridge::getTreeJson() const {
    if (!mainContent_) return QString();
    return QString::fromUtf8(mainContent_->treeAsJson());
}

void WebExperimentBridge::setPathChecked(const QString& path, bool checked) {
    if (!mainContent_) return;
    mainContent_->setPathChecked(path, checked);
    emit treeUpdated();
}

void WebExperimentBridge::requestChildrenForPath(const QString& path) {
    if (!mainContent_) return;
    mainContent_->requestChildrenForPath(path);
}

void WebExperimentBridge::requestContentsForPath(const QString& path) {
    if (!mainContent_) return;
    mainContent_->requestContentsForPath(path);
}

QString WebExperimentBridge::getSettings() const {
    if (!mainContent_) return QString();
    return mainContent_->getSettingsJson();
}

bool WebExperimentBridge::saveSettings(const QString& json) {
    if (!mainContent_) return false;
    return mainContent_->saveSettingsFromJson(json);
}

QString WebExperimentBridge::getLayoutState() const {
    if (!mainContent_) return QStringLiteral("{}");
    return mainContent_->getLayoutStateJson();
}

bool WebExperimentBridge::saveLayoutState(const QString& json) {
    if (!mainContent_) return false;
    return mainContent_->saveLayoutStateFromJson(json);
}

QString WebExperimentBridge::chooseSyncFolder(const QString& currentPath) {
    if (!mainContent_) return QString();
    return mainContent_->chooseSyncFolder(currentPath);
}

void WebExperimentBridge::startSync() {
    if (!mainContent_) return;
    mainContent_->onSyncClicked();
}

void WebExperimentBridge::stopSync() {
    if (!mainContent_) return;
    mainContent_->onStopSyncTriggered();
}

void WebExperimentBridge::downloadFile(const QString& cloudPath) {
    if (!mainContent_) return;
    mainContent_->downloadFile(cloudPath);
}

void WebExperimentBridge::openFileFromCloud(const QString& cloudPath) {
    if (!mainContent_) return;
    mainContent_->openFileFromCloud(cloudPath);
}

void WebExperimentBridge::deleteFromDisk(const QString& cloudPath) {
    if (!mainContent_) return;
    mainContent_->deleteFromDisk(cloudPath);
}

void WebExperimentBridge::onDownloadFinished(bool success, const QString& errorMessage) {
    emit downloadFinished(success, errorMessage);
}

void WebExperimentBridge::onDeleteFinished(bool success, const QString& errorMessage) {
    emit deleteFinished(success, errorMessage);
}

}  // namespace ydisquette
