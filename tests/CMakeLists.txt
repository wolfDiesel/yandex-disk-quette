include(CTest)
find_package(Catch2 3 QUIET)
if(Catch2_FOUND)
  list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
  include(Catch)
  add_executable(tests_runner smoke_test.cpp domain_test.cpp use_case_test.cpp api_parse_test.cpp sync_index_test.cpp json_config_test.cpp cloud_path_util_test.cpp ${CMAKE_SOURCE_DIR}/src/app/json_config.cpp)
  target_include_directories(tests_runner PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(tests_runner PRIVATE Catch2::Catch2WithMain y_disquette_core Qt6::Core)
  catch_discover_tests(tests_runner)
else()
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
  )
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)
  add_executable(tests_runner smoke_test.cpp domain_test.cpp use_case_test.cpp api_parse_test.cpp sync_index_test.cpp json_config_test.cpp cloud_path_util_test.cpp ${CMAKE_SOURCE_DIR}/src/app/json_config.cpp)
  target_include_directories(tests_runner PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(tests_runner PRIVATE Catch2::Catch2WithMain y_disquette_core Qt6::Core)
  catch_discover_tests(tests_runner)
endif()
