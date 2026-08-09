//
// Created by adama on 8/9/26.
//
#include "core/ChannelState.hpp"

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
