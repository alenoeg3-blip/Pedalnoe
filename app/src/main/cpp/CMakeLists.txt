cmake_minimum_required(VERSION 3.18.1)
project("pedalera_audio")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(oboe REQUIRED CONFIG)

add_library(
    pedalera_audio
    SHARED
    AudioEngine.cpp
    OverdriveEffect.cpp
    native-lib.cpp
)

target_link_libraries(
    pedalera_audio
    oboe::oboe
    log
)
