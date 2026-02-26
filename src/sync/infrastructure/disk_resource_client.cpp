#include "sync/infrastructure/disk_resource_client.hpp"
#include "auth/infrastructure/yandex_disk_api_client.hpp"
#include "auth/infrastructure/yandex_disk_path.hpp"
#include "shared/app_log.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

namespace ydisquette {
namespace sync {

DiskResourceClient::DiskResourceClient(auth::YandexDiskApiClient const& api) : api_(api) {}

DiskResourceResult DiskResourceClient::createFolder(const std::string& path) {
    const std::string norm = auth::normalizePathForApi(path);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("path"), QString::fromStdString(norm));
    auth::ApiResponse res = api_.putNoBody("/resources?" + q.query(QUrl::FullyEncoded).toStdString());
    DiskResourceResult out;
    out.success = res.ok() || res.statusCode == 409;
    out.httpStatus = res.statusCode;
    if (!out.success) {
        out.errorMessage = QString::fromStdString(res.body);
        ydisquette::logToFile(QStringLiteral("[Sync] createFolder ") + QString::fromStdString(path) + QStringLiteral(" FAIL: ") + QString::number(out.httpStatus) + QChar(' ') + out.errorMessage);
    }
    return out;
}

DiskResourceResult DiskResourceClient::downloadFile(const std::string& remotePath, const QString& localPath) {
    const QString pathQt = QString::fromStdString(remotePath);
    const std::string normPath = auth::normalizePathForApi(remotePath);
    const QByteArray pathEncoded = QUrl::toPercentEncoding(QString::fromStdString(normPath), QByteArray());
    const QString fullUrl = QStringLiteral("https://cloud-api.yandex.net/v1/disk/resources/download?path=")
                            + QString::fromUtf8(pathEncoded);
    auth::ApiResponse step1 = api_.getByFullUrl(fullUrl);
    DiskResourceResult out;
    if (!step1.ok()) {
        out.httpStatus = step1.statusCode;
        out.errorMessage = QStringLiteral("Requested path: %1. Yandex: %2")
                               .arg(pathQt, QString::fromStdString(step1.body));
        ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
        return out;
    }
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(step1.body));
    if (!doc.isObject()) {
        out.errorMessage = QStringLiteral("Requested path: %1. Invalid download response").arg(pathQt);
        ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
        return out;
    }
    QString href = doc.object().value(QStringLiteral("href")).toString();
    if (href.isEmpty()) {
        out.errorMessage = QStringLiteral("Requested path: %1. No href in download response").arg(pathQt);
        ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
        return out;
    }
    auth::ApiResponse step2 = api_.getAbsoluteUrl(href);
    if (!step2.ok()) {
        out.httpStatus = step2.statusCode;
        out.errorMessage = QStringLiteral("Requested path: %1. Download URL: %2. Yandex: %3")
                               .arg(pathQt, href, QString::fromStdString(step2.body));
        ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
        return out;
    }
    QFile f(localPath);
    if (!f.open(QIODevice::WriteOnly)) {
        out.errorMessage = f.errorString();
        ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
        return out;
    }
    f.write(step2.body.data(), static_cast<qint64>(step2.body.size()));
    out.success = true;
    return out;
}

void DiskResourceClient::downloadFileAsync(const std::string& remotePath, const QString& localPath,
                                           std::function<void(DiskResourceResult)> cb) {
    const QString pathQt = QString::fromStdString(remotePath);
    const std::string normPath = auth::normalizePathForApi(remotePath);
    const QByteArray pathEncoded = QUrl::toPercentEncoding(QString::fromStdString(normPath), QByteArray());
    const QString fullUrl = QStringLiteral("https://cloud-api.yandex.net/v1/disk/resources/download?path=")
                            + QString::fromUtf8(pathEncoded);
    api_.getByFullUrlAsync(fullUrl, [this, pathQt, localPath, cb](auth::ApiResponse step1) {
        if (!step1.ok()) {
            DiskResourceResult out;
            out.httpStatus = step1.statusCode;
            out.errorMessage = QStringLiteral("Requested path: %1. Yandex: %2")
                                   .arg(pathQt, QString::fromStdString(step1.body));
            ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
            if (cb) cb(out);
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(step1.body));
        if (!doc.isObject()) {
            DiskResourceResult out;
            out.errorMessage = QStringLiteral("Requested path: %1. Invalid download response").arg(pathQt);
            ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
            if (cb) cb(out);
            return;
        }
        QString href = doc.object().value(QStringLiteral("href")).toString();
        if (href.isEmpty()) {
            DiskResourceResult out;
            out.errorMessage = QStringLiteral("Requested path: %1. No href in download response").arg(pathQt);
            ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
            if (cb) cb(out);
            return;
        }
        api_.getAbsoluteUrlAsync(href, [pathQt, href, localPath, cb](auth::ApiResponse step2) {
        DiskResourceResult out;
        if (!step2.ok()) {
            out.httpStatus = step2.statusCode;
            out.errorMessage = QStringLiteral("Requested path: %1. Download URL: %2. Yandex: %3")
                                   .arg(pathQt, href, QString::fromStdString(step2.body));
            ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
            if (cb) cb(out);
            return;
        }
        QFile f(localPath);
        if (!f.open(QIODevice::WriteOnly)) {
            out.errorMessage = f.errorString();
            ydisquette::logToFile(QStringLiteral("[Sync] download ") + pathQt + QStringLiteral(" -> FAIL: ") + out.errorMessage);
            if (cb) cb(out);
            return;
        }
        f.write(step2.body.data(), static_cast<qint64>(step2.body.size()));
        out.success = true;
        if (cb) cb(out);
    });
    });
}

DiskResourceResult DiskResourceClient::uploadFile(const std::string& remotePath, const QString& localPath) {
    QFile f(localPath);
    if (!f.open(QIODevice::ReadOnly)) {
        DiskResourceResult out;
        out.errorMessage = f.errorString();
        ydisquette::logToFile(QStringLiteral("[Sync] upload ") + QString::fromStdString(remotePath) + QStringLiteral(" FAIL open: ") + f.errorString());
        return out;
    }
    QByteArray data = f.readAll();
    f.close();

    const std::string normPath = auth::normalizePathForApi(remotePath);
    const QByteArray pathEncoded = QUrl::toPercentEncoding(QString::fromStdString(normPath), QByteArray());
    const QString fullUrl = QStringLiteral("https://cloud-api.yandex.net/v1/disk/resources/upload?path=")
        + QString::fromUtf8(pathEncoded) + QStringLiteral("&overwrite=true");
    auth::ApiResponse step1 = api_.getByFullUrl(fullUrl);
    DiskResourceResult out;
    if (!step1.ok()) {
        out.httpStatus = step1.statusCode;
        out.errorMessage = QString::fromStdString(step1.body);
        ydisquette::logToFile(QStringLiteral("[Sync] upload ") + QString::fromStdString(remotePath) + QStringLiteral(" FAIL: ") + QString::number(step1.statusCode) + QChar(' ') + out.errorMessage);
        return out;
    }
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(step1.body));
    if (!doc.isObject()) {
        out.errorMessage = QStringLiteral("Invalid upload response");
        return out;
    }
    QString href = doc.object().value(QStringLiteral("href")).toString();
    if (href.isEmpty()) {
        out.errorMessage = QStringLiteral("No href in upload response");
        return out;
    }
    auth::ApiResponse step2 = api_.putToAbsoluteUrl(href, data);
    out.success = step2.ok();
    out.httpStatus = step2.statusCode;
    if (!step2.ok()) {
        out.errorMessage = QString::fromStdString(step2.body);
        ydisquette::logToFile(QStringLiteral("[Sync] upload ") + QString::fromStdString(remotePath) + QStringLiteral(" FAIL: ") + QString::number(out.httpStatus) + QChar(' ') + out.errorMessage);
    }
    return out;
}

DiskResourceResult DiskResourceClient::deleteResource(const std::string& path) {
    const std::string norm = auth::normalizePathForApi(path);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("path"), QString::fromStdString(norm));
    auth::ApiResponse res = api_.deleteResource("/resources?" + q.query(QUrl::FullyEncoded).toStdString());
    DiskResourceResult out;
    out.success = res.ok() || res.statusCode == 202;
    out.httpStatus = res.statusCode;
    if (!out.success) {
        out.errorMessage = QString::fromStdString(res.body);
        ydisquette::logToFile(QStringLiteral("[Sync] delete ") + QString::fromStdString(path) + QStringLiteral(" FAIL: ") + QString::number(out.httpStatus) + QChar(' ') + out.errorMessage);
    }
    return out;
}

void DiskResourceClient::deleteResourceAsync(const std::string& path,
                                             std::function<void(DiskResourceResult)> cb) {
    const std::string norm = auth::normalizePathForApi(path);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("path"), QString::fromStdString(norm));
    std::string apiPath = "/resources?" + q.query(QUrl::FullyEncoded).toStdString();
    api_.deleteResourceAsync(apiPath, [cb](auth::ApiResponse res) {
        DiskResourceResult out;
        out.success = res.ok() || res.statusCode == 202;
        out.httpStatus = res.statusCode;
        if (!out.success) out.errorMessage = QString::fromStdString(res.body);
        if (cb) cb(out);
    });
}

}  // namespace sync
}  // namespace ydisquette
