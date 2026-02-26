#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace ydisquette {

struct JsonConfig {
    QString accessToken;
    QString refreshToken;
    QByteArray windowGeometry;
    QByteArray splitterState;
    QString syncFolder;
    int maxRetries = 3;
    int cloudCheckIntervalSec = 30;
    int refreshIntervalSec = 60;
    QStringList selectedNodePaths;

    static QString configPath();
    static QString syncIndexDbPath();
    static JsonConfig load();
    static bool save(const JsonConfig& c);
    static JsonConfig loadFromPath(const QString& path);
    static bool saveToPath(const QString& path, const JsonConfig& c);
};

}  // namespace ydisquette
