cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(dzsungel-r VERSION 0.1)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
find_package(Boost REQUIRED)

add_subdirectory(libs/midifile)
add_subdirectory(libs/dr_wav)
add_subdirectory(libs/CLI11)
include(FetchContent)
fetchcontent_declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
)
fetchcontent_makeavailable(spdlog)

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
    src/midi/Smf.cpp
    src/io/Wav.cpp
)

set (HEADERS 
    include/dzsungel/resources/WavetableStore.hpp
    include/dzsungel/core/ChannelState.hpp
    include/dzsungel/core/VoiceAllocator.hpp
    include/dzsungel/core/Oscillators.hpp
    include/dzsungel/core/ADSR.hpp
    include/dzsungel/core/Algorithms.hpp
    include/dzsungel/core/Voice.hpp
    include/dzsungel/core/AudioEngine.hpp
    include/dzsungel/resources/SynthProgram.hpp
    include/dzsungel/midi/Smf.hpp
    include/dzsungel/io/Wav.hpp
    include/dzsungel/Constants.hpp
    include/dzsungel/RampingValue.hpp
    include/dzsungel/Types.hpp
)

add_library(dzsungel-core STATIC ${SOURCES} ${HEADERS})
target_include_directories(dzsungel-core PUBLIC ${PROJECT_SOURCE_DIR}/include/dzsungel)
target_link_libraries(dzsungel-core PRIVATE Boost::headers midifile dr_wav)

add_executable(dzsmf apps/SmfApp.cpp)

if (MSVC)
    target_compile_options(dzsmf PRIVATE
        /W4
        /permissive- 
        /Zc:enumTypes 
        $<$<CONFIG:Debug>:/Od /ZI>
        $<$<CONFIG:Release>:/Zi>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang") 
    target_compile_definitions(dzsmf PRIVATE /Wall /Wextra /Wpedantic)
endif()

target_link_libraries(dzsmf PRIVATE dzsungel-core midifile spdlog::spdlog Boost::headers CLI11::CLI11)

option(DZSUNGEL_BUILD_TESTS "Build the GoogleTest-based test suite" OFF)
option(DBG_RV_LOG_CSV "Log RampingValue operations to csv file" OFF)
if (DZSUNGEL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()