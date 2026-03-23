#include "sync/infrastructure/last_uploaded_parser.hpp"
#include "shared/cloud_path_util.hpp"
#include "sync/domain/cloud_datetime.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ydisquette {
namespace sync {

QVector<LastUploadedItem> parseLastUploadedJson(const std::string& body) {
    QVector<LastUploadedItem> out;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(body));
    if (!doc.isObject()) return out;
    QJsonArray items = doc.object().value(QStringLiteral("items")).toArray();
    for (const QJsonValue& v : items) {
        QJsonObject o = v.toObject();
        QString path = o.value(QStringLiteral("path")).toString();
        QString modified = o.value(QStringLiteral("modified")).toString();
        qint64 size = o.value(QStringLiteral("size")).toInteger(0);
        QString type = o.value(QStringLiteral("type")).toString();
        LastUploadedItem item;
        item.relativePath = ydisquette::cloudPathToRelativeQString(path.toStdString());
        item.modifiedSec = parseCloudModifiedToSec(modified.toStdString());
        item.size = size;
        item.type = type;
        if (item.relativePath.isEmpty()) continue;
        out.append(item);
    }
    return out;
}

}  // namespace sync
}  // namespace ydisquette
