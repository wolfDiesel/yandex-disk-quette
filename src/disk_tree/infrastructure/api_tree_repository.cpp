#include "disk_tree/infrastructure/api_tree_repository.hpp"
#include "disk_tree/infrastructure/api_parse.hpp"
#include "auth/infrastructure/yandex_disk_api_client.hpp"
#include <QUrlQuery>

namespace ydisquette {
namespace disk_tree {

ApiTreeRepository::ApiTreeRepository(auth::YandexDiskApiClient const& api) : api_(api) {}

std::shared_ptr<Node> ApiTreeRepository::getRoot() {
    return Node::makeDir("/", "");
}

std::vector<std::shared_ptr<Node>> ApiTreeRepository::getChildren(const std::string& path) {
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("path"), QString::fromStdString(path));
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("1000"));
    auth::ApiResponse res = api_.get("/resources", q);
    if (!res.ok()) return {};
    return parseResourcesJson(res.body);
}

void ApiTreeRepository::getChildrenAsync(const std::string& path,
                                         std::function<void(std::vector<std::shared_ptr<Node>>)> cb) {
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("path"), QString::fromStdString(path));
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("1000"));
    api_.getAsync("/resources", q, [cb](auth::ApiResponse res) {
        cb(res.ok() ? parseResourcesJson(res.body) : std::vector<std::shared_ptr<Node>>{});
    });
}

}  // namespace disk_tree
}  // namespace ydisquette
