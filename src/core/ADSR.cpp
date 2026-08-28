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

#include "core/ADSR.hpp"

#include <cmath>
#include <chrono>


static const float kLnEpsilon = std::log(EPSILON);

namespace dzsungel::core {
    static float calcExpRatio(std::chrono::duration<float> t, float sampleRate) {
        const auto sampleCount = static_cast<uint32_t>(t.count() * sampleRate);
        if (sampleCount == 0) return 1.0f;
        const float exponent = kLnEpsilon / static_cast<float>(sampleCount);
        return 1.0f - std::exp(exponent);
    }

    void ADSR::configure(const EnvelopeConfig &cfg, float sampleRate) {
        oneShot_ = cfg.oneShot;
        rA_ = calcExpRatio(cfg.attack, sampleRate);
        rD_ = calcExpRatio(cfg.release, sampleRate);
        rR_ = calcExpRatio(cfg.decay, sampleRate);
        sustain_ = cfg.sustain;
        state_ = ADSRState::IDLE;
    }

    void ADSR::trigger(bool oneShot) {
        state_ = ADSRState::ATTACK;
        oneShot_ = oneShot;
    }
    void ADSR::release() {
        state_ = ADSRState::RELEASE;
    }

    float ADSR::advance() {
        switch (state_) {
            case ADSRState::ATTACK: {
                y_ += (1.0f - y_) * rA_;
                if (y_ >= 1.0f - EPSILON) {
                    state_ = ADSRState::DECAY;
                    y_ = 1.0f;
                }
                break;
            };
            case ADSRState::DECAY: {
                y_ = (sustain_ - y_) * rD_;
                if (y_ <= sustain_ + EPSILON) {
                    state_ = oneShot_ ? ADSRState::IDLE : ADSRState::SUSTAIN;
                    y_ = oneShot_ ? 0 : sustain_;
                }
                break;
            }
            case ADSRState::SUSTAIN: break;
            case ADSRState::RELEASE: {
                y_ *= 1.0f - rR_;
                if (y_ <= EPSILON) {
                    y_ = 0;
                    state_ = ADSRState::IDLE;
                }
                break;
            }
            case ADSRState::IDLE: return 0;
        }

        return y_;
    }
} // namespace dzsungel::core
