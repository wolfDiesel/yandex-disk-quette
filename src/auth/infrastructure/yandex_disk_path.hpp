#pragma once

#include <QString>
#include <QUrl>
#include <string>

namespace ydisquette {
namespace auth {

inline std::string normalizePathForApi(std::string path) {
    if (path.empty()) return path;
    QUrl u(QString::fromStdString(path));
    QString p = u.path();
    if (p.isEmpty())
        p = QString::fromStdString(path);
    while (p.startsWith("//"))
        p = QStringLiteral("/") + p.mid(2);
    if (!p.startsWith(QStringLiteral("/")))
        p = QStringLiteral("/") + p;
    return p.toStdString();
}

}  // namespace auth
}  // namespace ydisquette
