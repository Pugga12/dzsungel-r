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

#include "core/AudioEngine.hpp"
#include "core/Oscillators.hpp"
#include "resources/WavetableStore.hpp"

int main() {
    AudioEngine en;
    std::array<float, 4096> outputTemp = {};
    SampleBuffer b{
        outputTemp,
        1,
        1
    };

    en.midiPush({
        0,
        MidiMsgType::CCBankLSB,
        0,
        8,
        0
    });
    en.midiPush({
        0,
        MidiMsgType::ProgramChange,
        0,
        48,
        0
    });
    en.midiPush({
        10,
        MidiMsgType::NoteOn,
        0,
        60,
        0
    });
    en.midiPush({
        4000,
        MidiMsgType::NoteOff,
        0,
        60,
        0
    });

    en.renderBlock(b);
}