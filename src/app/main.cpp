#include "composition_root.hpp"
#include "main_content_widget.hpp"
#include "main_window.hpp"
#include "oauth_credentials.hpp"
#include "auth/ui/login_widget.hpp"
#include "auth/infrastructure/oauth_callback_scheme_handler.hpp"
#include "auth/infrastructure/token_store.hpp"
#include "cloud_path_util.hpp"
#include "json_config.hpp"
#include "settings/domain/app_settings.hpp"
#include "shared/app_log.hpp"
#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStyleFactory>
#include <QTimer>
#include <QTranslator>
#include <QLocale>
#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>
#include <QWidget>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void loadConfigIntoApp(ydisquette::CompositionRoot& root, ydisquette::MainWindow* mainWindow,
                              ydisquette::MainContentWidget* mainContent) {
    ydisquette::JsonConfig c = ydisquette::JsonConfig::load();
    if (!c.accessToken.isEmpty() && !c.refreshToken.isEmpty())
        root.tokenStore().setTokens(c.accessToken.toStdString(), c.refreshToken.toStdString());
    if (!c.windowGeometry.isEmpty())
        mainWindow->restoreGeometry(c.windowGeometry);
    if (mainContent && !c.splitterState.isEmpty())
        mainContent->restoreState(c.splitterState);
    {
        ydisquette::settings::AppSettings s = root.getSettingsUseCase().run();
        if (!c.syncFolder.isEmpty()) s.syncPath = c.syncFolder.toStdString();
        if (c.maxRetries >= 1 && c.maxRetries <= 100) s.maxRetries = c.maxRetries;
        if (c.cloudCheckIntervalSec >= 5 && c.cloudCheckIntervalSec <= 3600) s.cloudCheckIntervalSec = c.cloudCheckIntervalSec;
        if (c.refreshIntervalSec >= 5 && c.refreshIntervalSec <= 3600) s.refreshIntervalSec = c.refreshIntervalSec;
        s.hideToTray = c.hideToTray;
        s.closeToTray = c.closeToTray;
        root.saveSettingsUseCase().run(s);
    }
    if (!c.selectedNodePaths.isEmpty()) {
        std::vector<std::string> paths;
        for (const QString& s : c.selectedNodePaths) {
            std::string n = ydisquette::normalizeCloudPath(s.toStdString());
            if (ydisquette::isValidCloudPath(n))
                paths.push_back(std::move(n));
        }
        root.setSelectedPaths(std::move(paths));
    }
}

static bool saveConfigFromApp(ydisquette::CompositionRoot& root, ydisquette::MainWindow* mainWindow,
                              ydisquette::MainContentWidget* mainContent) {
    ydisquette::JsonConfig c = ydisquette::JsonConfig::load();
    auto at = root.tokenStore().getAccessToken();
    auto rt = root.tokenStore().getRefreshToken();
    if (at) c.accessToken = QString::fromStdString(*at);
    if (rt) c.refreshToken = QString::fromStdString(*rt);
    c.windowGeometry = mainWindow->saveGeometry();
    if (mainContent)
        c.splitterState = mainContent->saveState();
    ydisquette::settings::AppSettings s = root.getSettingsUseCase().run();
    c.syncFolder = QString::fromStdString(s.syncPath);
    c.maxRetries = s.maxRetries;
    c.cloudCheckIntervalSec = s.cloudCheckIntervalSec;
    c.refreshIntervalSec = s.refreshIntervalSec;
    c.hideToTray = s.hideToTray;
    c.closeToTray = s.closeToTray;
    c.selectedNodePaths.clear();
    for (const std::string& p : root.getSelectedPaths())
        c.selectedNodePaths.append(QString::fromStdString(ydisquette::normalizeCloudPath(p)));
    return ydisquette::JsonConfig::save(c);
}

static FILE* g_logFile = nullptr;
static QMutex g_logMutex;

static void consoleMessageHandler(QtMsgType, const QMessageLogContext&, const QString& msg) {
    const QByteArray line = msg.toUtf8() + '\n';
    fprintf(stderr, "%s", line.constData());
    fflush(stderr);
    QMutexLocker lock(&g_logMutex);
    if (g_logFile) {
        fwrite(line.constData(), 1, static_cast<size_t>(line.size()), g_logFile);
        fflush(g_logFile);
    }
}

static ydisquette::LogLevel parseLogLevel(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "--loglevel=", 11) != 0) continue;
        int v = (argv[i][11] >= '0' && argv[i][11] <= '2') ? (argv[i][11] - '0') : -1;
        if (v >= 0) return static_cast<ydisquette::LogLevel>(v);
    }
    return ydisquette::LogLevel::Normal;
}

static QIcon loadAppIcon() {
    QIcon icon;
    QImage img(QStringLiteral(":/app_icon.png"));
    if (img.isNull()) return icon;
    img = img.convertToFormat(QImage::Format_ARGB32);
    const QList<int> sizes = {16, 22, 24, 32, 48, 64, 128};
    for (int s : sizes) {
        QPixmap px = QPixmap::fromImage(img.scaled(s, s, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        if (!px.isNull())
            icon.addPixmap(px);
    }
    if (icon.isNull())
        icon.addPixmap(QPixmap::fromImage(img));
    return icon;
}

int main(int argc, char* argv[]) {
    setvbuf(stderr, nullptr, _IONBF, 0);

    if (std::getenv("APPIMAGE")) {
        const char* e = std::getenv("QT_PLUGIN_PATH");
        char buf[2048];
        std::snprintf(buf, sizeof(buf),
                     "/usr/lib64/qt6/plugins:/usr/lib/qt6/plugins:/usr/lib/x86_64-linux-gnu/qt6/plugins%s%s",
                     e && *e ? ":" : "", e && *e ? e : "");
        setenv("QT_PLUGIN_PATH", buf, 1);
    }

    ydisquette::setLogLevel(parseLogLevel(argc, argv));

    QWebEngineUrlScheme scheme(QByteArrayLiteral("ydisquette"));
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
    QWebEngineUrlScheme::registerScheme(scheme);

    QApplication app(argc, argv);
    app.setWindowIcon(loadAppIcon());
    for (const QString& key : QStyleFactory::keys()) {
        if (key.compare(QStringLiteral("breeze"), Qt::CaseInsensitive) == 0) {
            if (QStyle* s = QStyleFactory::create(key)) {
                app.setStyle(s);
                break;
            }
        }
    }

    const QString logPath = QDir::temp().absoluteFilePath(QStringLiteral("y_disquette.log"));
    const QByteArray logPathBytes = logPath.toUtf8();
    g_logFile = fopen(logPathBytes.constData(), "a");
    if (g_logFile) {
        setvbuf(g_logFile, nullptr, _IONBF, 0);
        fprintf(g_logFile, "[Y.Disquette] log started: %s\n", logPathBytes.constData());
        fflush(g_logFile);
    } else {
        fprintf(stderr, "Y.Disquette: failed to open log %s\n", logPathBytes.constData());
    }
    if (g_logFile)
        ydisquette::setLogFile(g_logFile);
    qInstallMessageHandler(consoleMessageHandler);
    if (g_logFile)
        ydisquette::log(ydisquette::LogLevel::Normal, QStringLiteral("Y.Disquette started, log: ") + logPath);

    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("y_disquette"), QStringLiteral("_"),
                       QCoreApplication::applicationDirPath()))
        app.installTranslator(&translator);

    ydisquette::CompositionRoot root;

    ydisquette::MainContentWidget* mainContent = new ydisquette::MainContentWidget(root);
    ydisquette::MainWindow mainWindow(root);
    mainWindow.setWindowTitle(QStringLiteral("Y.Disquette"));
    mainWindow.setWindowIcon(app.windowIcon());
    mainWindow.setMinimumSize(400, 300);
    mainWindow.resize(800, 600);
    mainWindow.setCentralWidget(mainContent);
    QMenuBar* menuBar = mainWindow.menuBar();
    QMenu* fileMenu = menuBar->addMenu(QObject::tr("File"));
    QAction* settingsAction = fileMenu->addAction(QObject::tr("Settings…"));
    QObject::connect(settingsAction, &QAction::triggered, mainContent, &ydisquette::MainContentWidget::openSettings);
    QAction* syncAction = fileMenu->addAction(QObject::tr("Synchronize"));
    QObject::connect(syncAction, &QAction::triggered, mainContent, &ydisquette::MainContentWidget::onSyncClicked);
    mainContent->setSyncAction(syncAction);
    QAction* stopSyncAction = fileMenu->addAction(QObject::tr("Stop"));
    stopSyncAction->setEnabled(false);
    QObject::connect(stopSyncAction, &QAction::triggered, mainContent, &ydisquette::MainContentWidget::onStopSyncTriggered);
    mainContent->setStopSyncAction(stopSyncAction);
    loadConfigIntoApp(root, &mainWindow, mainContent);

    QString clientId, clientSecret, redirectUri;
    ydisquette::getBuildTimeOAuthCredentials(&clientId, &clientSecret, &redirectUri);
    const QString envId = QString::fromUtf8(qgetenv("YDISQUETTE_CLIENT_ID"));
    const QString envSecret = QString::fromUtf8(qgetenv("YDISQUETTE_CLIENT_SECRET"));
    if (!envId.isEmpty()) clientId = envId;
    if (!envSecret.isEmpty()) clientSecret = envSecret;
    const bool hasCredentials = !clientId.isEmpty() && !clientSecret.isEmpty() && !redirectUri.isEmpty();
    if (!hasCredentials) {
        ydisquette::log(ydisquette::LogLevel::Normal,
                        "[Auth] NO CREDENTIALS: client_id/secret/redirect are empty; login window will not be shown.");
    }

    const auto existingToken = root.tokenProvider().getAccessToken();
    if (existingToken) {
        ydisquette::log(ydisquette::LogLevel::Normal,
                        "[Auth] Existing access token found on startup, skipping login UI.");
        mainWindow.show();
    } else if (hasCredentials) {
        ydisquette::log(ydisquette::LogLevel::Normal,
                        "[Auth] Credentials present and no token, showing login UI.");
        auto* handler = new ydisquette::auth::OAuthCallbackSchemeHandler(&app);
        QWebEngineProfile::defaultProfile()->installUrlSchemeHandler(
            QByteArrayLiteral("ydisquette"), handler);

        auto* login = new ydisquette::auth::LoginWidget(
            root.oauthClient(), clientId, clientSecret, redirectUri);
        login->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(handler, &ydisquette::auth::OAuthCallbackSchemeHandler::codeReceived,
                         login, &ydisquette::auth::LoginWidget::onCodeReceived);
        QObject::connect(login, &ydisquette::auth::LoginWidget::loggedIn, [&]() {
            saveConfigFromApp(root, &mainWindow, mainContent);
            mainContent->setProperty("skipFirstShow", true);
            mainWindow.show();
            mainWindow.hide();
            mainWindow.show();
            mainWindow.raise();
            mainWindow.activateWindow();
            QTimer::singleShot(300, &app, [login]() {
                login->hide();
                login->deleteLater();
            });
        });
        QObject::connect(login, &ydisquette::auth::LoginWidget::loginFailed, [login](const QString& err) {
            QMessageBox::warning(login, QObject::tr("Sign-in error"), err);
        });
        login->startLogin();
        login->show();
        login->raise();
        login->activateWindow();
    } else {
        if (!hasCredentials) {
            ydisquette::log(ydisquette::LogLevel::Normal,
                            "[Auth] No credentials configured, starting without login UI.");
        }
        mainWindow.show();
    }

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        if (!saveConfigFromApp(root, &mainWindow, mainContent))
            QMessageBox::warning(&mainWindow, QObject::tr("Settings"), QObject::tr("Failed to save settings to disk."));
    });

    return app.exec();
}
