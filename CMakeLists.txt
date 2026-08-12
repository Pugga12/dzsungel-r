cmake_minimum_required(VERSION 3.14)
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

# Library target containing all engine logic, with no main() of its own.
# Both the CLI app and the test suite link against this so tests exercise
# the exact same object code as the shipped binary.
add_library(dzsungel-core STATIC ${SOURCES})
target_include_directories(dzsungel-core PUBLIC ${PROJECT_SOURCE_DIR}/include/dzsungel)

add_executable(dzsungel-r src/apps/cli-render.cpp)
target_link_libraries(dzsungel-r PRIVATE dzsungel-core)

option(DZSUNGEL_BUILD_TESTS "Build the GoogleTest-based test suite" ON)
if (DZSUNGEL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
