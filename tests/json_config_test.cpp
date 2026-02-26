#include <catch2/catch_test_macros.hpp>
#include <app/json_config.hpp>
#include <QCoreApplication>
#include <QTemporaryDir>

using namespace ydisquette;

TEST_CASE("JsonConfig save and load round-trip in temp dir") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString configPath = dir.filePath(QStringLiteral("config.json"));

    JsonConfig c;
    c.syncFolder = QStringLiteral("/home/user/Sync");
    c.maxRetries = 5;
    c.cloudCheckIntervalSec = 45;
    c.refreshIntervalSec = 120;
    c.selectedNodePaths = { QStringLiteral("/Disk/Apps"), QStringLiteral("/Disk/Docs") };

    JsonConfig::saveToPath(configPath, c);
    JsonConfig loaded = JsonConfig::loadFromPath(configPath);

    REQUIRE(loaded.syncFolder == c.syncFolder);
    REQUIRE(loaded.maxRetries == c.maxRetries);
    REQUIRE(loaded.cloudCheckIntervalSec == c.cloudCheckIntervalSec);
    REQUIRE(loaded.refreshIntervalSec == c.refreshIntervalSec);
    REQUIRE(loaded.selectedNodePaths.size() == 2u);
    REQUIRE(loaded.selectedNodePaths.at(0) == QStringLiteral("/Disk/Apps"));
    REQUIRE(loaded.selectedNodePaths.at(1) == QStringLiteral("/Disk/Docs"));
}

TEST_CASE("JsonConfig load missing file returns defaults") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString missingPath = dir.filePath(QStringLiteral("nonexistent.json"));

    JsonConfig loaded = JsonConfig::loadFromPath(missingPath);

    REQUIRE(loaded.syncFolder.isEmpty());
    REQUIRE(loaded.selectedNodePaths.isEmpty());
    REQUIRE(loaded.maxRetries == 3);
    REQUIRE(loaded.cloudCheckIntervalSec == 30);
    REQUIRE(loaded.refreshIntervalSec == 60);
}

TEST_CASE("JsonConfig load invalid JSON returns defaults") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    QString path = dir.filePath(QStringLiteral("bad.json"));
    {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write("{ invalid ");
        f.close();
    }

    JsonConfig loaded = JsonConfig::loadFromPath(path);

    REQUIRE(loaded.syncFolder.isEmpty());
    REQUIRE(loaded.selectedNodePaths.isEmpty());
}
