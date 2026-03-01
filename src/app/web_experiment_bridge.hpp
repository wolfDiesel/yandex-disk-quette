#pragma once

#include <QObject>
#include <QString>

namespace ydisquette {

class MainContentWidget;

class WebExperimentBridge : public QObject {
    Q_OBJECT
public:
    explicit WebExperimentBridge(MainContentWidget* mainContent, QObject* parent = nullptr);

    Q_INVOKABLE QString getTreeJson() const;
    Q_INVOKABLE void setPathChecked(const QString& path, bool checked);
    Q_INVOKABLE void requestChildrenForPath(const QString& path);
    Q_INVOKABLE void requestContentsForPath(const QString& path);
    Q_INVOKABLE QString getSettings() const;
    Q_INVOKABLE bool saveSettings(const QString& json);
    Q_INVOKABLE QString getLayoutState() const;
    Q_INVOKABLE bool saveLayoutState(const QString& json);
    Q_INVOKABLE QString chooseSyncFolder(const QString& currentPath);
    Q_INVOKABLE void startSync();
    Q_INVOKABLE void stopSync();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void downloadFile(const QString& cloudPath);
    Q_INVOKABLE void openFileFromCloud(const QString& cloudPath);
    Q_INVOKABLE void deleteFromDisk(const QString& cloudPath);
    Q_INVOKABLE QString getAboutInfo() const;
    Q_INVOKABLE void quitApplication();

signals:
    void treeUpdated();
    void downloadFinished(bool success, const QString& errorMessage);
    void deleteFinished(bool success, const QString& errorMessage);

public slots:
    void onDownloadFinished(bool success, const QString& errorMessage);
    void onDeleteFinished(bool success, const QString& errorMessage);

private:
    MainContentWidget* mainContent_ = nullptr;
};

}  // namespace ydisquette
