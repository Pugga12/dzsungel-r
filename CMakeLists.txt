cmake_minimum_required(VERSION 3.14)
project(dzsungel-r VERSION 0.1)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Boost REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include/dzsungel)

set(SOURCES
    src/resources/WavetableStore.cpp
        src/core/ChannelState.cpp
        src/core/VoiceAllocator.cpp
        src/core/Oscillators.cpp
        src/core/ADSR.cpp
        src/core/Algorithms.cpp
        src/core/Voice.cpp
        src/core/AudioEngine.cpp
        src/resources/SynthProgram.cpp
)

# Library target containing all engine logic, with no main() of its own.
# Both the CLI app and the test suite link against this so tests exercise
# the exact same object code as the shipped binary.
add_library(dzsungel-core STATIC ${SOURCES})
target_include_directories(dzsungel-core PUBLIC ${PROJECT_SOURCE_DIR}/include/dzsungel)
target_link_libraries(dzsungel-core PRIVATE Boost::headers)

add_executable(dzsmf src/apps/SmfApp.cpp)
target_link_libraries(dzsmf PRIVATE dzsungel-core)

option(DZSUNGEL_BUILD_TESTS "Build the GoogleTest-based test suite" OFF)
if (DZSUNGEL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
