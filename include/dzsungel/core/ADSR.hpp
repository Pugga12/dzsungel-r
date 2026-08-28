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
#include <chrono>
#include "Constants.hpp"

namespace dzsungel::core {
    enum class ADSRState {
        IDLE, ATTACK, DECAY, SUSTAIN, RELEASE
    };

    struct EnvelopeConfig {
        std::chrono::duration<float> attack;
        std::chrono::duration<float> decay;
        float sustain;
        std::chrono::duration<float> release;
        bool oneShot;
    };

    class ADSR {
    public:
        void configure(const EnvelopeConfig& cfg, float sampleRate = kDefaultSampleRate);
        void trigger(bool oneShot = false);
        void release();
        float advance();
        [[nodiscard]] ADSRState getState() const { return state_; }
    private:
        ADSRState state_ = ADSRState::IDLE;
        bool oneShot_ = false;

        float rA_ = 0, rD_ = 0, rR_ = 0;
        float y_ = 0;
        float sustain_ = 0;
    };
}