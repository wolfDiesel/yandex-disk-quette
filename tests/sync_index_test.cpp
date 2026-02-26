#include <catch2/catch_test_macros.hpp>
#include <sync/infrastructure/sync_index.hpp>
#include <sync/domain/sync_file_status.hpp>
#include <QCoreApplication>
#include <QTemporaryDir>

using namespace ydisquette::sync;

TEST_CASE("SyncIndex create table, set, get, remove, removePrefix") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString dbPath = dir.filePath(QStringLiteral("sync_index.db"));

    SyncIndex index;
    REQUIRE(index.open(dbPath));
    REQUIRE(index.beginTransaction());

    REQUIRE(index.set(QStringLiteral("/home/sync"), QStringLiteral("a/file.txt"), 1000, 500));
    auto e = index.get(QStringLiteral("/home/sync"), QStringLiteral("a/file.txt"));
    REQUIRE(e.has_value());
    REQUIRE(e->mtime_sec == 1000);
    REQUIRE(e->size == 500);
    REQUIRE(e->status == QLatin1String(FileStatus::SYNCED));
    REQUIRE(e->retries == 0);
    REQUIRE(index.get(QStringLiteral("/other"), QStringLiteral("a/file.txt")) == std::nullopt);
    index.commit();
    index.close();

    REQUIRE(index.open(dbPath));
    REQUIRE(index.beginTransaction());
    index.set(QStringLiteral("/root"), QStringLiteral("dir/file1"), 1, 10);
    index.set(QStringLiteral("/root"), QStringLiteral("dir/file2"), 2, 20);
    index.set(QStringLiteral("/root"), QStringLiteral("dir/sub/file3"), 3, 30);
    index.commit();

    REQUIRE(index.beginTransaction());
    index.remove(QStringLiteral("/root"), QStringLiteral("dir/file1"));
    REQUIRE(index.get(QStringLiteral("/root"), QStringLiteral("dir/file1")) == std::nullopt);
    REQUIRE(index.get(QStringLiteral("/root"), QStringLiteral("dir/file2")).has_value());
    index.removePrefix(QStringLiteral("/root"), QStringLiteral("dir"));
    REQUIRE(index.get(QStringLiteral("/root"), QStringLiteral("dir/file2")) == std::nullopt);
    REQUIRE(index.get(QStringLiteral("/root"), QStringLiteral("dir/sub/file3")) == std::nullopt);
    index.commit();
    index.close();
}

TEST_CASE("SyncIndex status, retries, setStatus, upsertNew") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString dbPath = dir.filePath(QStringLiteral("sync_index.db"));
    SyncIndex index;
    REQUIRE(index.open(dbPath));
    REQUIRE(index.beginTransaction());
    REQUIRE(index.upsertNew(QStringLiteral("/home/sync"), QStringLiteral("new.txt"), 100, 200));
    auto e = index.get(QStringLiteral("/home/sync"), QStringLiteral("new.txt"));
    REQUIRE(e.has_value());
    REQUIRE(e->status == QLatin1String(FileStatus::NEW));
    REQUIRE(e->retries == 0);
    REQUIRE(e->mtime_sec == 100);
    REQUIRE(e->size == 200);
    REQUIRE(index.setStatus(QStringLiteral("/home/sync"), QStringLiteral("new.txt"), QString::fromUtf8(FileStatus::UPLOADING), 0));
    e = index.get(QStringLiteral("/home/sync"), QStringLiteral("new.txt"));
    REQUIRE(e.has_value());
    REQUIRE(e->status == QLatin1String(FileStatus::UPLOADING));
    REQUIRE(index.setStatus(QStringLiteral("/home/sync"), QStringLiteral("new.txt"), QString::fromUtf8(FileStatus::FAILED), 1));
    e = index.get(QStringLiteral("/home/sync"), QStringLiteral("new.txt"));
    REQUIRE(e.has_value());
    REQUIRE(e->status == QLatin1String(FileStatus::FAILED));
    REQUIRE(e->retries == 1);
    index.commit();
    index.close();
}

TEST_CASE("normalizeSyncRoot") {
    REQUIRE(normalizeSyncRoot(QStringLiteral("  /home/user/sync/  ")) == QStringLiteral("/home/user/sync"));
    REQUIRE(normalizeSyncRoot(QStringLiteral("/home/user/sync")) == QStringLiteral("/home/user/sync"));
}

TEST_CASE("SyncIndex getTopLevelRelativePaths") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString dbPath = dir.filePath(QStringLiteral("sync_index.db"));
    SyncIndex index;
    REQUIRE(index.open(dbPath));
    REQUIRE(index.beginTransaction());
    index.set(QStringLiteral("/home/sync"), QStringLiteral("Apps/f1.txt"), 1, 2);
    index.set(QStringLiteral("/home/sync"), QStringLiteral("Downloads/vmware/x"), 2, 3);
    index.set(QStringLiteral("/home/sync"), QStringLiteral("trr"), 3, 4);
    index.commit();
    QStringList top = index.getTopLevelRelativePaths(QStringLiteral("/home/sync"));
    REQUIRE(top.size() == 3);
    REQUIRE(top.contains(QStringLiteral("Apps")));
    REQUIRE(top.contains(QStringLiteral("Downloads")));
    REQUIRE(top.contains(QStringLiteral("trr")));
    index.close();
}

TEST_CASE("SyncIndex normalizes relative_path, no duplicates") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString dbPath = dir.filePath(QStringLiteral("sync_index.db"));
    SyncIndex index;
    REQUIRE(index.open(dbPath));
    REQUIRE(index.beginTransaction());
    index.set(QStringLiteral("/home/sync"), QStringLiteral("a/b.txt"), 1, 2);
    REQUIRE(index.get(QStringLiteral("/home/sync"), QStringLiteral("a/b.txt"))->mtime_sec == 1);
    REQUIRE(index.get(QStringLiteral("/home/sync"), QStringLiteral("/a/b.txt"))->mtime_sec == 1);
    index.set(QStringLiteral("/home/sync"), QStringLiteral("/a/b.txt"), 3, 4);
    auto e = index.get(QStringLiteral("/home/sync"), QStringLiteral("a/b.txt"));
    REQUIRE(e.has_value());
    REQUIRE(e->mtime_sec == 3);
    REQUIRE(e->size == 4);
    index.commit();
    index.close();
}
