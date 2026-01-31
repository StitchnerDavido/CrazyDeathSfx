cmake_minimum_required(VERSION 3.16)
project(CrazyDeathSFX VERSION 1.0.0 LANGUAGES CXX)

find_package(Geode REQUIRED)

geode_mod("CrazyDeathSFX" "recuration.cds")

target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Geode::core)
