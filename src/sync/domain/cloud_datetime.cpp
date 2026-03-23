#include "sync/domain/cloud_datetime.hpp"
#include <QString>

namespace ydisquette {
namespace sync {

QDateTime parseCloudModified(const std::string& modified) {
    QString s = QString::fromStdString(modified).trimmed();
    if (s.isEmpty()) return QDateTime();
    if (!s.endsWith(QLatin1Char('Z')) && s.indexOf(QLatin1Char('+'), 10) < 0
        && (s.length() <= 19 || (s.at(19) != QLatin1Char('+') && s.at(19) != QLatin1Char('-'))))
        s += QStringLiteral("Z");
    QDateTime dt = QDateTime::fromString(s, Qt::ISODate);
    if (dt.isValid())
        dt = dt.toUTC();
    return dt;
}

}  // namespace sync
}  // namespace ydisquette
