#include <catch2/catch_test_macros.hpp>
#include <app/cloud_path_util.hpp>

using namespace ydisquette;

TEST_CASE("normalizeCloudPath produces leading slash") {
    REQUIRE(normalizeCloudPath("") == "");
    REQUIRE(normalizeCloudPath("foo") == "/foo");
    REQUIRE(normalizeCloudPath("/foo") == "/foo");
    REQUIRE(normalizeCloudPath("disk:/foo") == "/foo");
    REQUIRE(normalizeCloudPath("  /bar  ") == "/bar");
    REQUIRE(normalizeCloudPath("//bar") == "/bar");
}

TEST_CASE("isValidCloudPath rejects empty or invalid") {
    REQUIRE_FALSE(isValidCloudPath(""));
    REQUIRE_FALSE(isValidCloudPath("   "));
    REQUIRE_FALSE(isValidCloudPath("/path/"));
}

TEST_CASE("isValidCloudPath accepts valid paths") {
    REQUIRE(isValidCloudPath("/"));
    REQUIRE(isValidCloudPath("/Disk"));
    REQUIRE(isValidCloudPath("/Disk/Apps"));
    std::string n = normalizeCloudPath("disk:/Docs");
    REQUIRE(isValidCloudPath(n));
}
