#include <catch2/catch_test_macros.hpp>
#include <disk_tree/domain/node.hpp>
#include <disk_tree/domain/quota.hpp>
#include <sync/domain/sync_status.hpp>
#include <settings/domain/app_settings.hpp>

using namespace ydisquette;

TEST_CASE("Node makeDir and makeFile") {
    auto dir = disk_tree::Node::makeDir("/Photos", "Photos", "2024-01-01");
    REQUIRE(dir->isDir());
    REQUIRE(dir->path == "/Photos");
    REQUIRE(dir->name == "Photos");
    REQUIRE(dir->size == 0);
    REQUIRE(dir->syncSelected == false);

    auto file = disk_tree::Node::makeFile("/doc.txt", "doc.txt", 1024, "2024-01-02");
    REQUIRE(file->isFile());
    REQUIRE(file->path == "/doc.txt");
    REQUIRE(file->name == "doc.txt");
    REQUIRE(file->size == 1024);
}

TEST_CASE("Quota freeSpace") {
    disk_tree::Quota q;
    q.totalSpace = 1000;
    q.usedSpace = 300;
    REQUIRE(q.freeSpace() == 700);

    q.usedSpace = 1000;
    REQUIRE(q.freeSpace() == 0);

    q.totalSpace = 500;
    q.usedSpace = 600;
    REQUIRE(q.freeSpace() == 0);

    q.totalSpace = 0;
    REQUIRE(q.freeSpace() == 0);
}

TEST_CASE("SyncStatus is enum") {
    REQUIRE(static_cast<int>(sync::SyncStatus::Idle) != static_cast<int>(sync::SyncStatus::Syncing));
}

TEST_CASE("AppSettings default") {
    settings::AppSettings s;
    REQUIRE(s.syncPath.empty());
}
