#include <catch2/catch_test_macros.hpp>
#include <sync/infrastructure/last_uploaded_parser.hpp>
#include <QCoreApplication>

using namespace ydisquette::sync;

TEST_CASE("parseLastUploadedJson maps items to LastUploadedItem list") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    std::string json = R"({
        "items": [
            {"path": "disk:/foo/bar.txt", "modified": "2024-01-15T12:00:00Z", "size": 100, "type": "file"},
            {"path": "disk:/root.txt", "modified": "2024-01-14T10:30:00Z", "size": 50, "type": "file"}
        ],
        "limit": 20
    })";
    QVector<LastUploadedItem> out = parseLastUploadedJson(json);
    REQUIRE(out.size() == 2);
    REQUIRE(out[0].relativePath == QStringLiteral("foo/bar.txt"));
    REQUIRE(out[0].modifiedSec > 0);
    REQUIRE(out[0].size == 100);
    REQUIRE(out[0].type == QStringLiteral("file"));
    REQUIRE(out[1].relativePath == QStringLiteral("root.txt"));
    REQUIRE(out[1].modifiedSec > 0);
    REQUIRE(out[1].size == 50);
    REQUIRE(out[1].type == QStringLiteral("file"));
}

TEST_CASE("parseLastUploadedJson normalizes path without disk prefix") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    std::string json = R"({"items": [{"path": "/Photos/IMG.jpg", "modified": "2024-01-01T00:00:00Z", "size": 0, "type": "file"}]})";
    QVector<LastUploadedItem> out = parseLastUploadedJson(json);
    REQUIRE(out.size() == 1);
    REQUIRE(out[0].relativePath == QStringLiteral("Photos/IMG.jpg"));
}

TEST_CASE("parseLastUploadedJson returns empty on invalid JSON") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    REQUIRE(parseLastUploadedJson("not json").empty());
    REQUIRE(parseLastUploadedJson("{ invalid }").empty());
}

TEST_CASE("parseLastUploadedJson returns empty on empty object or missing items") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    REQUIRE(parseLastUploadedJson("{}").empty());
    REQUIRE(parseLastUploadedJson(R"({"limit": 20})").empty());
}

TEST_CASE("parseLastUploadedJson skips item with empty path") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    std::string json = R"({"items": [
        {"path": "", "modified": "2024-01-15T12:00:00Z", "size": 0, "type": "file"},
        {"path": "disk:/", "modified": "2024-01-15T12:00:00Z", "size": 0, "type": "file"}
    ]})";
    QVector<LastUploadedItem> out = parseLastUploadedJson(json);
    REQUIRE(out.empty());
}

TEST_CASE("parseLastUploadedJson includes dir type") {
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    std::string json = R"({"items": [{"path": "disk:/folder", "modified": "2024-01-15T12:00:00Z", "size": 0, "type": "dir"}]})";
    QVector<LastUploadedItem> out = parseLastUploadedJson(json);
    REQUIRE(out.size() == 1);
    REQUIRE(out[0].type == QStringLiteral("dir"));
    REQUIRE(out[0].relativePath == QStringLiteral("folder"));
}
