#pragma once

#include <QWidget>

class QCloseEvent;
class QPlainTextEdit;
class QShowEvent;
class QSplitter;
class QWebChannel;
class QWebEngineView;

namespace ydisquette {

class MainContentWidget;
class WebExperimentBridge;
class WebExperimentPage;

class WebExperimentWindow : public QWidget {
    Q_OBJECT
public:
    explicit WebExperimentWindow(MainContentWidget* mainContent, QWidget* parent = nullptr);
    ~WebExperimentWindow() override;

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onLoadFinished(bool ok);
    void onJsConsole(int level, const QString& message, int lineNumber, const QString& sourceId);
    void onChildrenForPathLoaded(const QString& path, const QByteArray& json);
    void onContentsForPathLoaded(const QString& path, const QByteArray& json);
    void onStatusBarUpdated(const QString& json);

private:
    void injectTree();
    static QString escapeJsString(const QString& s);

    MainContentWidget* mainContent_ = nullptr;
    QWebEngineView* view_ = nullptr;
    WebExperimentPage* page_ = nullptr;
    QWebChannel* channel_ = nullptr;
    WebExperimentBridge* bridge_ = nullptr;
    QSplitter* splitter_ = nullptr;
    QPlainTextEdit* consoleEdit_ = nullptr;
    bool geometryRestored_ = false;
};

}  // namespace ydisquette
