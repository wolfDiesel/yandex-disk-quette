#include <catch2/catch_test_macros.hpp>
#include <sync/application/to_delete_batches.hpp>
#include <QCoreApplication>

using namespace ydisquette::sync;

static void requireSameContents(const QStringList& a, const QStringList& b) {
    REQUIRE(a.size() == b.size());
    for (const QString& x : a)
        REQUIRE(b.contains(x));
}

TEST_CASE("computeToDeleteBatches empty") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    ToDeleteBatches b = computeToDeleteBatches(QStringList());
    REQUIRE(b.rootFiles.isEmpty());
    REQUIRE(b.minimalFolders.isEmpty());
}

TEST_CASE("computeToDeleteBatches root files only") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    ToDeleteBatches b = computeToDeleteBatches({ QStringLiteral("a.txt"), QStringLiteral("b.txt") });
    requireSameContents(b.rootFiles, { QStringLiteral("a.txt"), QStringLiteral("b.txt") });
    REQUIRE(b.minimalFolders.isEmpty());
}

TEST_CASE("computeToDeleteBatches one folder prefix") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    ToDeleteBatches b = computeToDeleteBatches({
        QStringLiteral("Photos/a.jpg"),
        QStringLiteral("Photos/2024/b.jpg")
    });
    REQUIRE(b.rootFiles.isEmpty());
    REQUIRE(b.minimalFolders.size() == 1);
    REQUIRE(b.minimalFolders.contains(QStringLiteral("Photos")));
}

TEST_CASE("computeToDeleteBatches two sibling folders") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    ToDeleteBatches b = computeToDeleteBatches({
        QStringLiteral("Photos/a.jpg"),
        QStringLiteral("Docs/readme.txt")
    });
    REQUIRE(b.rootFiles.isEmpty());
    REQUIRE(b.minimalFolders.size() == 2);
    REQUIRE(b.minimalFolders.contains(QStringLiteral("Photos")));
    REQUIRE(b.minimalFolders.contains(QStringLiteral("Docs")));
}

TEST_CASE("computeToDeleteBatches root file and folder with same name - folder wins") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    ToDeleteBatches b = computeToDeleteBatches({
        QStringLiteral("Photos"),
        QStringLiteral("Photos/a.jpg")
    });
    REQUIRE(b.rootFiles.isEmpty());
    REQUIRE(b.minimalFolders.size() == 1);
    REQUIRE(b.minimalFolders.contains(QStringLiteral("Photos")));
}

TEST_CASE("computeToDeleteBatches root file not a prefix") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    ToDeleteBatches b = computeToDeleteBatches({
        QStringLiteral("Photos"),
        QStringLiteral("Docs/readme.txt")
    });
    REQUIRE(b.rootFiles.size() == 1);
    REQUIRE(b.rootFiles.contains(QStringLiteral("Photos")));
    REQUIRE(b.minimalFolders.size() == 1);
    REQUIRE(b.minimalFolders.contains(QStringLiteral("Docs")));
}

TEST_CASE("computeToDeleteBatches nested folders minimal set") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);
    ToDeleteBatches b = computeToDeleteBatches({
        QStringLiteral("A/B/c.txt"),
        QStringLiteral("A/B/D/e.txt"),
        QStringLiteral("A/f.txt")
    });
    REQUIRE(b.rootFiles.isEmpty());
    REQUIRE(b.minimalFolders.size() == 1);
    REQUIRE(b.minimalFolders.contains(QStringLiteral("A")));
}
