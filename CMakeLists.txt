cmake_minimum_required(VERSION 3.16)
project(Y.Disquette VERSION 0.0.36 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql WebEngineCore WebEngineWidgets WebChannel LinguistTools)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(src)
enable_testing()
add_subdirectory(tests)
