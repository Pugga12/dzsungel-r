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
#include "midi/Smf.hpp"

namespace dzsungel::midi {
    static bool isSupportedController(uint8_t cc) {
        switch (cc) {
            case 0:
            case 7:
            case 10:
            case 11:
            case 32:
                return true;
            default:
                return false;
        }
    }

    enum class PayloadKind {
        KeyValue, ValueOnly, Unsupported
    };

    static PayloadKind classify(MidiMsgType t) {
        switch (t) {
            case MidiMsgType::NoteOn:
            case MidiMsgType::NoteOff:
                return PayloadKind::KeyValue;

            case MidiMsgType::CCBankLSB:
            case MidiMsgType::CCBankMSB:
            case MidiMsgType::CCVolume:
            case MidiMsgType::CCPan:
            case MidiMsgType::CCExpression:
            case MidiMsgType::PitchBend:
                return PayloadKind::KeyValue;

            case MidiMsgType::ProgramChange:
                return PayloadKind::ValueOnly;



            default:
                return PayloadKind::Unsupported;
        }
    }

    static MidiMsgType decodeType(const MidiEvent& ev) {
        switch (ev.getCommandNibble()) {
            case 0x80: return MidiMsgType::NoteOff;
            case 0x90: return MidiMsgType::NoteOn;
            case 0xC0: return MidiMsgType::ProgramChange;
            case 0xE0: return MidiMsgType::PitchBend;

            case 0xB0:
                return isSupportedController(static_cast<uint8_t>(ev.getControllerNumber()))
                        ? static_cast<MidiMsgType>(ev.getControllerNumber())
                        : MidiMsgType::Invalid;
            default: return MidiMsgType::Invalid;
        }
    }

    void IOSmf::convertTrack(float sampleRate) {
        for (size_t i = 0; i < file_[0].size(); ++i) {
            const auto& ev = file_[0][i];
            const MidiMsgType type = decodeType(ev);
            if (type == MidiMsgType::Invalid) continue;

            MidiMsg msg = {};
            msg.type = type;
            msg.channel = static_cast<uint8_t>(ev.getChannel());

            if (type == MidiMsgType::NoteOn && ev.getVelocity() == 0) {
                // note on events with v = 0 are treated as note off, rewire
                msg.type = MidiMsgType::NoteOff;
            }
            if (type == MidiMsgType::CCBankMSB) states_[msg.channel].msb = ev.getControllerValue();
            if (type == MidiMsgType::CCBankLSB) states_[msg.channel].lsb = ev.getControllerValue();
            if (type == MidiMsgType::ProgramChange) {
                preloads_.insert(packProgramId(states_[msg.channel].msb, states_[msg.channel].lsb, ev[1]));
            }

            switch (classify(msg.type)) {
                case PayloadKind::KeyValue:
                    msg.data1 = ev[1];
                    msg.data2 = ev[2];
                    break;
                case PayloadKind::ValueOnly:
                    msg.data1 = ev[1];
                    msg.data2 = 0;
                    break;
                default:
                    continue;
            }

            events_.push_back(msg);
        }
    }

    bool IOSmf::load(const std::string &fName, float sampleRate) {
        if (loaded_) unload();
        if (!file_.read(fName)) return false;

        file_.doTimeAnalysis();
        file_.joinTracks();

        events_.reserve(file_[0].size());
        convertTrack(sampleRate);

        assert(std::ranges::is_sorted(events_, [](const MidiMsg& a, const MidiMsg& b) {
            return a.absoluteSample < b.absoluteSample;
        }));

        file_.clear();
        loaded_ = true;
        return true;
    }

    void IOSmf::unload() {
        events_.clear();
        events_.shrink_to_fit();
        loaded_ = false;
    }
} // namespace dzsungel::midi
