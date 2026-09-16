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

set(MIDIFILE_INCLUDES 
        include/Options.h
        include/Binasc.h
        include/MidiEvent.h
        include/MidiEventList.h
        include/MidiFile.h
        include/MidiMessage.h
)

add_library(midifile STATIC ${MIDIFILE_SOURCES})

target_include_directories(midifile PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)