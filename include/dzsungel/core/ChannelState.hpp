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

#pragma once
#include <array>
#include <cstdint>
#include "Types.hpp"

namespace dzsungel::core {
    struct ChannelState {
        uint32_t stateVersion = 0;
        uint8_t bankSelectMsb  = 0;
        uint8_t bankSelectLsb = 0;
        uint32_t packedProgId = 0;

        int16_t pitchBendRaw = 0;
        uint8_t expression = 127;
        uint8_t volume = 127;
        uint8_t pan = 64;
    };

    class ChannelStateStore {
    public:
        [[nodiscard]] const ChannelState& get(const uint8_t id) const { return channels_[id];}
        bool apply(const MidiMsg& msg);
    private:
        std::array<ChannelState, 16> channels_;
    };
}