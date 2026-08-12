cmake_minimum_required(VERSION 3.10)
project(dzsungel-r VERSION 2.0)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include/dzsungel)

set(SOURCES
    src/resources/WavetableStore.cpp
        src/core/ChannelState.cpp
        src/core/VoiceAllocator.cpp
)

add_executable(dzsungel-r ${SOURCES} src/apps/cli-render.cpp)