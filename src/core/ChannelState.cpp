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

#include "core/ChannelState.hpp"
#include <cstdint>

int16_t pitchBendToSInt(const MidiMsg& msg) {
    const uint16_t uFull = static_cast<uint16_t>(msg.data2 << 7) | static_cast<uint16_t>(msg.data1);
    return uFull - 8192;
}

namespace dzsungel::core {
    bool ChannelStateStore::apply(const MidiMsg &msg) {
        if (msg.channel > 16) {
            return false;
        }
        ChannelState& c = channels_[msg.channel];
        c.stateVersion++;

        // ReSharper disable once CppIncompleteSwitchStatement
        switch (msg.type) {
            case MidiMsgType::PitchBend: {
                c.pitchBendRaw = pitchBendToSInt(msg);
            }
            case MidiMsgType::CCExpression: {
                c.expression = msg.data1;
            }
            case MidiMsgType::CCVolume: {
                c.volume = msg.data1;
            }
            case MidiMsgType::CCPan: {
                c.pan = msg.data1;
            }
            case MidiMsgType::ProgramChange: {
                c.packedProgId = msg.data1;
            }
            case MidiMsgType::NoteOff: case MidiMsgType::NoteOn: break;
        }

        return true;
    }
}
