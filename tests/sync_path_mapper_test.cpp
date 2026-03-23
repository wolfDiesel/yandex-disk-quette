#include <catch2/catch_test_macros.hpp>
#include <sync/application/sync_path_mapper.hpp>
#include <QCoreApplication>

using namespace ydisquette::sync;

TEST_CASE("localPathToRelative strips sync root prefix") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    QString syncRoot = QStringLiteral("/home/user/sync");
    REQUIRE(localPathToRelative(QStringLiteral("/home/user/sync"), syncRoot) == QString());
    REQUIRE(localPathToRelative(QStringLiteral("/home/user/sync/file.txt"), syncRoot) == QStringLiteral("file.txt"));
    REQUIRE(localPathToRelative(QStringLiteral("/home/user/sync/a/b.txt"), syncRoot) == QStringLiteral("a/b.txt"));
    REQUIRE(localPathToRelative(QStringLiteral("/other/path"), syncRoot).isEmpty());
    REQUIRE(localPathToRelative(QStringLiteral("/home/user/syncx"), syncRoot).isEmpty());
}
