#include "disk_tree/infrastructure/api_parse.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ydisquette {
namespace disk_tree {

Quota parseDiskJson(const std::string& body) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(body));
    if (!doc.isObject()) return {};
    QJsonObject o = doc.object();
    Quota q;
    q.totalSpace = static_cast<int64_t>(o.value(QStringLiteral("total_space")).toInteger(0));
    q.usedSpace = static_cast<int64_t>(o.value(QStringLiteral("used_space")).toInteger(0));
    return q;
}

std::vector<std::shared_ptr<Node>> parseResourcesJson(const std::string& body) {
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(body));
    if (!doc.isObject()) return {};
    QJsonObject root = doc.object();
    QJsonObject embedded = root.value(QStringLiteral("_embedded")).toObject();
    QJsonArray items = embedded.value(QStringLiteral("items")).toArray();

    std::vector<std::shared_ptr<Node>> out;
    for (const QJsonValue& v : items) {
        QJsonObject o = v.toObject();
        QString type = o.value(QStringLiteral("type")).toString();
        std::string pathStr = o.value(QStringLiteral("path")).toString().toUtf8().toStdString();
        std::string name = o.value(QStringLiteral("name")).toString().toUtf8().toStdString();
        std::string modified = o.value(QStringLiteral("modified")).toString().toUtf8().toStdString();
        if (type == QLatin1String("dir")) {
            out.push_back(Node::makeDir(std::move(pathStr), std::move(name), std::move(modified)));
        } else {
            qint64 size = o.value(QStringLiteral("size")).toInteger(0);
            out.push_back(Node::makeFile(std::move(pathStr), std::move(name), static_cast<int64_t>(size), std::move(modified)));
        }
    }
    return out;
}

}  // namespace disk_tree
}  // namespace ydisquette
