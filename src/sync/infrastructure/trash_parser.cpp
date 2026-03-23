#include "sync/infrastructure/trash_parser.hpp"
#include "shared/cloud_path_util.hpp"
#include "sync/domain/cloud_datetime.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ydisquette {
namespace sync {

QVector<TrashItem> parseTrashJson(const std::string& body) {
    QVector<TrashItem> out;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(body));
    if (!doc.isObject()) return out;
    QJsonObject root = doc.object();
    QJsonArray items = root.value(QStringLiteral("_embedded")).toObject().value(QStringLiteral("items")).toArray();
    if (items.isEmpty())
        items = root.value(QStringLiteral("items")).toArray();
    for (const QJsonValue& v : items) {
        QJsonObject o = v.toObject();
        QString path = o.value(QStringLiteral("path")).toString();
        QString originPath = o.value(QStringLiteral("origin_path")).toString();
        QString deletedStr = o.value(QStringLiteral("deleted")).toString();
        if (deletedStr.isEmpty()) deletedStr = o.value(QStringLiteral("modified")).toString();
        QString type = o.value(QStringLiteral("type")).toString();
        if (originPath.isEmpty()) continue;
        TrashItem item;
        item.originPath = originPath;
        item.deletedSec = parseCloudModifiedToSec(deletedStr.toStdString());
        item.type = type;
        out.append(item);
    }
    return out;
}

}  // namespace sync
}  // namespace ydisquette
