#include <catch2/catch_test_macros.hpp>
#include <disk_tree/application/load_tree_use_case.hpp>
#include <disk_tree/application/get_quota_use_case.hpp>
#include <disk_tree/infrastructure/mock_tree_repository.hpp>
#include <disk_tree/infrastructure/mock_quota_service.hpp>
#include <settings/application/get_settings_use_case.hpp>
#include <settings/application/save_settings_use_case.hpp>
#include <settings/application/toggle_node_selection_use_case.hpp>
#include <settings/infrastructure/mock_settings_store.hpp>
#include <settings/infrastructure/mock_selected_nodes_store.hpp>

using namespace ydisquette;

TEST_CASE("LoadTreeUseCase returns tree from repository") {
    disk_tree::MockTreeRepository repo;
    auto root = disk_tree::Node::makeDir("/", "root");
    root->children.push_back(disk_tree::Node::makeDir("/Photos", "Photos"));
    repo.setRoot(root);

    disk_tree::LoadTreeUseCase useCase(repo);
    auto result = useCase.run();
    REQUIRE(result != nullptr);
    REQUIRE(result->path == "/");
    REQUIRE(result->children.size() == 1);
    REQUIRE(result->children[0]->name == "Photos");
}

TEST_CASE("GetQuotaUseCase returns quota from service") {
    disk_tree::MockQuotaService service;
    disk_tree::Quota q;
    q.totalSpace = 1000;
    q.usedSpace = 250;
    service.setQuota(q);

    disk_tree::GetQuotaUseCase useCase(service);
    auto result = useCase.run();
    REQUIRE(result.freeSpace() == 750);
}

TEST_CASE("GetSettingsUseCase and SaveSettingsUseCase") {
    settings::MockSettingsStore store;
    settings::AppSettings s;
    s.syncPath = "/home/user/Sync";
    store.save(s);

    settings::GetSettingsUseCase getUseCase(store);
    auto loaded = getUseCase.run();
    REQUIRE(loaded.syncPath == "/home/user/Sync");

    settings::SaveSettingsUseCase saveUseCase(store);
    settings::AppSettings s2;
    s2.syncPath = "/other";
    saveUseCase.run(s2);
    REQUIRE(getUseCase.run().syncPath == "/other");
}

TEST_CASE("ToggleNodeSelectionUseCase toggles path") {
    settings::MockSelectedNodesStore store;

    settings::ToggleNodeSelectionUseCase useCase(store);
    useCase.run("/Photos");
    auto paths = store.getSelectedPaths();
    REQUIRE(paths.size() == 1);
    REQUIRE(paths[0] == "/Photos");

    useCase.run("/Photos");
    paths = store.getSelectedPaths();
    REQUIRE(paths.empty());

    useCase.run("/A");
    useCase.run("/B");
    paths = store.getSelectedPaths();
    REQUIRE(paths.size() == 2);
}
