//
// Created by adama on 8/9/26.
//
#pragma once
#include <cstdint>

enum class MidiMsgType : uint8_t {
    NoteOn, NoteOff, PitchBend, CCExpression, CCVolume, CCPan, ProgramChange
};

struct MidiMsg {
    uint32_t sampleOffset;

    MidiMsgType type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
};

struct VoiceEvent {
    uint32_t blockId;
    uint8_t sampleOffset;

    MidiMsgType type;
    uint8_t data1;
    uint8_t data2;
};