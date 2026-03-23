#include <catch2/catch_test_macros.hpp>
#include <sync/infrastructure/sync_index.hpp>
#include <sync/infrastructure/sqlite_poll_run_repository.hpp>
#include <QCoreApplication>
#include <QTemporaryDir>

using namespace ydisquette::sync;

TEST_CASE("SyncIndex creates poll_run table usable by SqlitePollRunRepository") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString dbPath = dir.filePath(QStringLiteral("sync_index.db"));

    SyncIndex index;
    REQUIRE(index.open(dbPath));
    index.close();

    SqlitePollRunRepository repo;
    REQUIRE(repo.open(dbPath));
    qint64 id = repo.insert(QStringLiteral("/home/sync"), 1700000000);
    REQUIRE(id > 0);
    bool ok = repo.updateRun(id, QStringLiteral("completed"), 1700000010, 2, QString());
    REQUIRE(ok);
    repo.close();
}

TEST_CASE("SqlitePollRunRepository insert and updateRun") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString dbPath = dir.filePath(QStringLiteral("sync_index.db"));

    SyncIndex index;
    REQUIRE(index.open(dbPath));
    index.close();

    SqlitePollRunRepository repo;
    REQUIRE(repo.open(dbPath));
    qint64 id1 = repo.insert(QStringLiteral("/root"), 1000);
    REQUIRE(id1 > 0);
    REQUIRE(repo.updateRun(id1, QStringLiteral("failed"), 1001, 0, QStringLiteral("Network error")));
    qint64 id2 = repo.insert(QStringLiteral("/root"), 2000);
    REQUIRE(id2 > id1);
    REQUIRE(repo.updateRun(id2, QStringLiteral("completed"), 2005, 3, QString()));
    repo.close();
}

TEST_CASE("SqlitePollRunRepository pruneKeepingLast does not fail") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString dbPath = dir.filePath(QStringLiteral("sync_index.db"));

    SyncIndex index;
    REQUIRE(index.open(dbPath));
    index.close();

    SqlitePollRunRepository repo;
    REQUIRE(repo.open(dbPath));
    repo.insert(QStringLiteral("/r"), 1);
    repo.insert(QStringLiteral("/r"), 2);
    repo.insert(QStringLiteral("/r"), 3);
    REQUIRE(repo.pruneKeepingLast(2));
    REQUIRE(repo.pruneKeepingLast(3000));
    repo.close();
}

TEST_CASE("SqlitePollRunRepository getLastFinishedAt returns last completed run") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString dbPath = dir.filePath(QStringLiteral("sync_index.db"));

    SyncIndex index;
    REQUIRE(index.open(dbPath));
    index.close();

    SqlitePollRunRepository repo;
    REQUIRE(repo.open(dbPath));
    REQUIRE(!repo.getLastFinishedAt(QStringLiteral("/root")).has_value());

    qint64 id1 = repo.insert(QStringLiteral("/root"), 1000);
    REQUIRE(id1 > 0);
    REQUIRE(repo.updateRun(id1, QStringLiteral("completed"), 1005, 0, QString()));
    auto t1 = repo.getLastFinishedAt(QStringLiteral("/root"));
    REQUIRE(t1.has_value());
    REQUIRE(*t1 == 1005);

    qint64 id2 = repo.insert(QStringLiteral("/root"), 2000);
    REQUIRE(id2 > 0);
    auto t2 = repo.getLastFinishedAt(QStringLiteral("/root"));
    REQUIRE(t2.has_value());
    REQUIRE(*t2 == 1005);

    REQUIRE(repo.updateRun(id2, QStringLiteral("completed"), 2010, 0, QString()));
    auto t3 = repo.getLastFinishedAt(QStringLiteral("/root"));
    REQUIRE(t3.has_value());
    REQUIRE(*t3 == 2010);

    repo.close();
}
