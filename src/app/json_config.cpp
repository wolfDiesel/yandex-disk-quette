#include "json_config.hpp"
#include "shared/app_log.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace ydisquette {

QString JsonConfig::configPath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (!dir.isEmpty())
        dir += QStringLiteral("/Y.Disquette");
    return dir + QStringLiteral("/config.json");
}

QString JsonConfig::syncIndexDbPath() {
    return QFileInfo(configPath()).absolutePath() + QStringLiteral("/sync_index.db");
}

static JsonConfig loadFromFile(const QString& path) {
    JsonConfig c;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return c;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        logToFile(QStringLiteral("[Config] load failed ") + path + QStringLiteral(": ") + err.errorString());
        return c;
    }
    QJsonObject o = doc.object();
    c.accessToken = o.value(QStringLiteral("access_token")).toString();
    c.refreshToken = o.value(QStringLiteral("refresh_token")).toString();
    c.windowGeometry = QByteArray::fromBase64(o.value(QStringLiteral("window_geometry")).toString().toUtf8());
    c.splitterState = QByteArray::fromBase64(o.value(QStringLiteral("splitter_state")).toString().toUtf8());
    c.syncFolder = o.value(QStringLiteral("sync_folder")).toString();
    int mr = o.value(QStringLiteral("sync_max_retries")).toInt(3);
    c.maxRetries = (mr > 0 && mr <= 100) ? mr : 3;
    int cc = o.value(QStringLiteral("cloud_check_interval_sec")).toInt(30);
    c.cloudCheckIntervalSec = (cc >= 5 && cc <= 3600) ? cc : 30;
    int rr = o.value(QStringLiteral("refresh_interval_sec")).toInt(60);
    c.refreshIntervalSec = (rr >= 5 && rr <= 3600) ? rr : 60;
    c.hideToTray = o.value(QStringLiteral("hide_to_tray")).toBool(true);
    c.closeToTray = o.value(QStringLiteral("close_to_tray")).toBool(true);
    for (const QJsonValue& v : o.value(QStringLiteral("selected_node_paths")).toArray())
        c.selectedNodePaths.append(v.toString());
    return c;
}

static bool saveToFile(const QString& path, const JsonConfig& c) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return false;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    QJsonObject o;
    o.insert(QStringLiteral("access_token"), c.accessToken);
    o.insert(QStringLiteral("refresh_token"), c.refreshToken);
    o.insert(QStringLiteral("window_geometry"), QString::fromUtf8(c.windowGeometry.toBase64()));
    o.insert(QStringLiteral("splitter_state"), QString::fromUtf8(c.splitterState.toBase64()));
    o.insert(QStringLiteral("sync_folder"), c.syncFolder);
    o.insert(QStringLiteral("sync_max_retries"), c.maxRetries);
    o.insert(QStringLiteral("cloud_check_interval_sec"), c.cloudCheckIntervalSec);
    o.insert(QStringLiteral("refresh_interval_sec"), c.refreshIntervalSec);
    o.insert(QStringLiteral("hide_to_tray"), c.hideToTray);
    o.insert(QStringLiteral("close_to_tray"), c.closeToTray);
    QJsonArray arr;
    for (const QString& p : c.selectedNodePaths)
        arr.append(p);
    o.insert(QStringLiteral("selected_node_paths"), arr);
    qint64 written = f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    f.close();
    return written >= 0;
}

JsonConfig JsonConfig::load() {
    return loadFromFile(configPath());
}

bool JsonConfig::save(const JsonConfig& c) {
    return saveToFile(configPath(), c);
}

JsonConfig JsonConfig::loadFromPath(const QString& path) {
    return loadFromFile(path);
}

bool JsonConfig::saveToPath(const QString& path, const JsonConfig& c) {
    return saveToFile(path, c);
}

}  // namespace ydisquette
