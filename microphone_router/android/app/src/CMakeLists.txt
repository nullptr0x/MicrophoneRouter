cmake_minimum_required(VERSION 3.22.1)

project(audio_router VERSION 0.0.1 LANGUAGES CXX)
find_package(oboe REQUIRED CONFIG)

add_library(
    audio_router
    SHARED ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/audio_router.cpp
)

target_link_libraries(
    audio_router
    oboe::oboe
    log
)

target_compile_options(audio_router PRIVATE -Wall -std=c++17)