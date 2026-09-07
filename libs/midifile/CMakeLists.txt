cmake_minimum_required(VERSION 3.10)
project(midifile C CXX)

set(MIDIFILE_SOURCES
        src/Options.cpp
        src/Binasc.cpp
        src/MidiEvent.cpp
        src/MidiEventList.cpp
        src/MidiFile.cpp
        src/MidiMessage.cpp
)

add_library(midifile STATIC ${MIDIFILE_SOURCES})

target_include_directories(midifile PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)