#include "web_experiment_window.hpp"
#include "web_experiment_bridge.hpp"
#include "web_experiment_page.hpp"
#include "main_content_widget.hpp"
#include "json_config.hpp"
#include <QCloseEvent>
#include <QMainWindow>
#include <QShowEvent>
#include <QWebChannel>
#include <QWebEngineView>
#include <QWebEngineContextMenuRequest>
#include <QContextMenuEvent>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QUrl>
#include <QGroupBox>
#include <QMetaObject>
#include <QThread>

namespace ydisquette {

class WebExperimentView : public QWebEngineView {
public:
    explicit WebExperimentView(QWidget* parent = nullptr) : QWebEngineView(parent) {}
protected:
    void contextMenuEvent(QContextMenuEvent* event) override {
        if (QWebEngineContextMenuRequest* req = lastContextMenuRequest())
            req->setAccepted(true);
    }
};

WebExperimentWindow::WebExperimentWindow(MainContentWidget* mainContent, QWidget* parent)
    : QWidget(parent), mainContent_(mainContent) {
    setWindowTitle(tr("Y.Disquette — Web experiment"));
    resize(520, 640);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    splitter_ = new QSplitter(Qt::Vertical, this);
    view_ = new WebExperimentView(this);
    page_ = new WebExperimentPage(this);
    view_->setPage(page_);
    splitter_->addWidget(view_);
    auto* consoleBox = new QGroupBox(tr("Console"), this);
    auto* consoleLayout = new QVBoxLayout(consoleBox);
    consoleEdit_ = new QPlainTextEdit(this);
    consoleEdit_->setReadOnly(true);
    consoleEdit_->setMaximumBlockCount(2000);
    consoleEdit_->setPlaceholderText(tr("JavaScript console output (console.log, errors)…"));
    consoleLayout->addWidget(consoleEdit_);
    splitter_->addWidget(consoleBox);
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 0);
    splitter_->setSizes({400, 200});
    layout->addWidget(splitter_);
    channel_ = new QWebChannel(this);
    bridge_ = new WebExperimentBridge(mainContent, this);
    channel_->registerObject(QStringLiteral("bridge"), bridge_);
    page_->setWebChannel(channel_);
    connect(bridge_, &WebExperimentBridge::treeUpdated, this, &WebExperimentWindow::injectTree);
    connect(mainContent_, &MainContentWidget::childrenForPathLoaded, this, &WebExperimentWindow::onChildrenForPathLoaded, Qt::QueuedConnection);
    connect(mainContent_, &MainContentWidget::contentsForPathLoaded, this, &WebExperimentWindow::onContentsForPathLoaded, Qt::QueuedConnection);
    connect(mainContent_, &MainContentWidget::statusBarUpdated, this, &WebExperimentWindow::onStatusBarUpdated, Qt::QueuedConnection);
    connect(mainContent_, &MainContentWidget::treeRefreshed, this, &WebExperimentWindow::injectTree, Qt::QueuedConnection);
    connect(mainContent_, &MainContentWidget::downloadFinished, bridge_, &WebExperimentBridge::onDownloadFinished, Qt::QueuedConnection);
    connect(mainContent_, &MainContentWidget::deleteFinished, bridge_, &WebExperimentBridge::onDeleteFinished, Qt::QueuedConnection);
    connect(page_, &WebExperimentPage::javaScriptConsole, this, &WebExperimentWindow::onJsConsole, Qt::QueuedConnection);
    view_->setUrl(QUrl(QStringLiteral("qrc:/web_experiment_react/index.html")));
    connect(page_, &QWebEnginePage::loadFinished, this, &WebExperimentWindow::onLoadFinished);
}

void WebExperimentWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!geometryRestored_) {
        geometryRestored_ = true;
        JsonConfig c = JsonConfig::load();
        if (isWindow() && !c.webExperimentGeometry.isEmpty())
            restoreGeometry(c.webExperimentGeometry);
        if (!c.debugConsole && splitter_ && splitter_->count() >= 2) {
            int h = splitter_->height();
            if (h <= 0) h = 400;
            splitter_->setSizes({h, 0});
        }
    }
}

void WebExperimentWindow::closeEvent(QCloseEvent* event) {
    if (isWindow()) {
        JsonConfig c = JsonConfig::load();
        c.webExperimentGeometry = saveGeometry();
        JsonConfig::save(c);
    }
    QWidget::closeEvent(event);
}

void WebExperimentWindow::onJsConsole(int level, const QString& message, int lineNumber, const QString& sourceId) {
    QString prefix;
    if (level == 0) prefix = QStringLiteral("[log] ");
    else if (level == 1) prefix = QStringLiteral("[warn] ");
    else prefix = QStringLiteral("[error] ");
    QString line = prefix + message;
    if (!sourceId.isEmpty() || lineNumber > 0)
        line += QStringLiteral(" (") + (sourceId.isEmpty() ? QString() : sourceId + QStringLiteral(" ")) + (lineNumber > 0 ? QString::number(lineNumber) : QString()) + QLatin1Char(')');
    consoleEdit_->appendPlainText(line);
}

WebExperimentWindow::~WebExperimentWindow() = default;

void WebExperimentWindow::onLoadFinished(bool ok) {
    if (ok) {
        injectTree();
        mainContent_->requestStatusBarUpdate();
    }
}

QString WebExperimentWindow::escapeJsString(const QString& s) {
    QString out;
    out.reserve(s.size() + 10);
    for (const QChar c : s) {
        if (c == QLatin1Char('\\')) out += QStringLiteral("\\\\");
        else if (c == QLatin1Char('"')) out += QStringLiteral("\\\"");
        else if (c == QLatin1Char('\n')) out += QStringLiteral("\\n");
        else if (c == QLatin1Char('\r')) out += QStringLiteral("\\r");
        else out += c;
    }
    return out;
}

static QString escapeJsonForJs(const QByteArray& json) {
    QString out;
    const QString raw = QString::fromUtf8(json);
    out.reserve(raw.size() + 50);
    for (const QChar c : raw) {
        if (c == QLatin1Char('\\')) out += QStringLiteral("\\\\");
        else if (c == QLatin1Char('"')) out += QStringLiteral("\\\"");
        else if (c == QLatin1Char('\n')) out += QStringLiteral("\\n");
        else if (c == QLatin1Char('\r')) out += QStringLiteral("\\r");
        else out += c;
    }
    return out;
}

void WebExperimentWindow::onChildrenForPathLoaded(const QString& path, const QByteArray& json) {
    if (QThread::currentThread() != page_->thread()) {
        QMetaObject::invokeMethod(this, [this, path, json]() { onChildrenForPathLoaded(path, json); }, Qt::QueuedConnection);
        return;
    }
    QString pathEsc = escapeJsString(path);
    QString jsonEsc = escapeJsonForJs(json);
    QString script = QStringLiteral("(function(){ var p = \"") + pathEsc + QStringLiteral("\"; var raw = \"") + jsonEsc
        + QStringLiteral("\"; try { if (window.__injectChildren__) window.__injectChildren__(p, JSON.parse(raw)); } catch(e) {} })();");
    page_->runJavaScript(script);
}

void WebExperimentWindow::onContentsForPathLoaded(const QString& path, const QByteArray& json) {
    if (QThread::currentThread() != page_->thread()) {
        QMetaObject::invokeMethod(this, [this, path, json]() { onContentsForPathLoaded(path, json); }, Qt::QueuedConnection);
        return;
    }
    QString pathEsc = escapeJsString(path);
    QString jsonEsc = escapeJsonForJs(json);
    QString script = QStringLiteral("(function(){ var p = \"") + pathEsc + QStringLiteral("\"; var raw = \"") + jsonEsc
        + QStringLiteral("\"; try { var data = JSON.parse(raw); if (window.__onContentsLoaded__) window.__onContentsLoaded__(p, data); } catch(e) { if (window.__onContentsLoaded__) window.__onContentsLoaded__(p, []); } })();");
    page_->runJavaScript(script);
}

void WebExperimentWindow::onStatusBarUpdated(const QString& json) {
    if (QThread::currentThread() != page_->thread()) {
        QMetaObject::invokeMethod(this, [this, json]() { onStatusBarUpdated(json); }, Qt::QueuedConnection);
        return;
    }
    QString jsonEsc = escapeJsString(json);
    QString script = QStringLiteral("window.__updateStatusBar__ && window.__updateStatusBar__(\"") + jsonEsc + QStringLiteral("\");");
    page_->runJavaScript(script);
}

void WebExperimentWindow::injectTree() {
    if (!mainContent_) return;
    QByteArray json = mainContent_->treeAsJson();
    QString script = QStringLiteral("(function(){ window.__TREE_DATA__ = ") + QString::fromUtf8(json)
        + QStringLiteral("; if (typeof renderTree === 'function') renderTree(); if (typeof bindTreeClicks === 'function') bindTreeClicks(); console.log('Tree injected, nodes:', window.__TREE_DATA__ && window.__TREE_DATA__.length); })()");
    page_->runJavaScript(script);
}

}  // namespace ydisquette
