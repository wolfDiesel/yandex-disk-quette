#include <catch2/catch_test_macros.hpp>
#include <disk_tree/infrastructure/api_parse.hpp>

using namespace ydisquette::disk_tree;

TEST_CASE("parseDiskJson maps total_space and used_space to Quota") {
    std::string json = R"({"total_space": 10737418240, "used_space": 3221225472})";
    Quota q = parseDiskJson(json);
    REQUIRE(q.totalSpace == 10737418240);
    REQUIRE(q.usedSpace == 3221225472);
    REQUIRE(q.freeSpace() == 7516192768);
}

TEST_CASE("parseDiskJson returns zero Quota on invalid JSON") {
    Quota q = parseDiskJson("not json");
    REQUIRE(q.totalSpace == 0);
    REQUIRE(q.usedSpace == 0);
}

TEST_CASE("parseDiskJson returns zero Quota on empty object") {
    Quota q = parseDiskJson("{}");
    REQUIRE(q.totalSpace == 0);
    REQUIRE(q.usedSpace == 0);
}

TEST_CASE("parseDiskJson handles 401-style empty body") {
    Quota q = parseDiskJson("");
    REQUIRE(q.totalSpace == 0);
    REQUIRE(q.usedSpace == 0);
}

TEST_CASE("parseResourcesJson maps _embedded.items to Node list") {
    std::string json = R"({
        "_embedded": {
            "items": [
                {"type": "dir", "path": "/Photos", "name": "Photos", "modified": "2024-01-15T12:00:00Z"},
                {"type": "file", "path": "/readme.txt", "name": "readme.txt", "size": 1024, "modified": "2024-01-14T10:00:00Z"}
            ]
        }
    })";
    auto nodes = parseResourcesJson(json);
    REQUIRE(nodes.size() == 2);
    REQUIRE(nodes[0]->isDir());
    REQUIRE(nodes[0]->path == "/Photos");
    REQUIRE(nodes[0]->name == "Photos");
    REQUIRE(nodes[0]->modified == "2024-01-15T12:00:00Z");
    REQUIRE(nodes[1]->isFile());
    REQUIRE(nodes[1]->path == "/readme.txt");
    REQUIRE(nodes[1]->name == "readme.txt");
    REQUIRE(nodes[1]->size == 1024);
}

TEST_CASE("parseResourcesJson returns empty on invalid JSON") {
    auto nodes = parseResourcesJson("{ invalid }");
    REQUIRE(nodes.empty());
}

TEST_CASE("parseResourcesJson returns empty when _embedded or items missing") {
    REQUIRE(parseResourcesJson("{}").empty());
    REQUIRE(parseResourcesJson(R"({"_embedded": {}})").empty());
}

TEST_CASE("parseResourcesJson returns empty on 500-style body") {
    auto nodes = parseResourcesJson(R"({"error": "Internal Server Error"})");
    REQUIRE(nodes.empty());
}
