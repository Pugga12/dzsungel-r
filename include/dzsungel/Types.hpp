// Copyright (C) 2026  Adam Aptowitz
//
// This file is part of Dzsungel
//
// Dzsungel is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dzsungel.  If not, see <http://www.gnu.org/license>

//
// Created by adama on 8/9/26.
//
#pragma once
#include <cstdint>
#include <span>

enum class MidiMsgType : uint8_t {
    NoteOn, NoteOff, PitchBend, CCExpression, CCVolume, CCPan, ProgramChange
};

struct MidiMsg {
    uint32_t absoluteSample;

    MidiMsgType type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
};

struct SampleBuffer {
    std::span<float> data;
    uint8_t channels = 1;
    uint8_t stride = 1;
};