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
#include "core/Voice.hpp"
#include <cmath>
#include <functional>

namespace dzsungel::core {
    static float noteToFrequency(uint8_t note) {
        return 440.0f * std::exp2((note - 69.0f) / 12.0f);
    }

    static size_t bufToFrameCount(SampleBuffer& buf) {
        if (buf.stride == 1) {
            return buf.data.size();
        }
        return buf.data.size() / buf.stride;
    }

    void Voice::noteOn(uint8_t noteNumber, uint8_t velocity) {
        float frequency = noteToFrequency(noteNumber);
        std::visit([&](auto& a) {
            using T = std::decay_t<decltype(a)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                a.noteOn(frequency, velocity);
            }
        }, algorithm_);

        currentPitch_ = noteNumber;
        currentVelocity_ = velocity;
        state_ = VoiceState::ACTIVE;
        ampEnv_.trigger();
    }

    void Voice::noteOff() {
        ampEnv_.release();
        state_ = VoiceState::RELEASING;
        std::visit([](auto& a) {
            using T = std::decay_t<decltype(a)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                a.release();
            }
        },algorithm_);
    }

    void Voice::processBlock(SampleBuffer &buf) {
        if (channelInfo_.stateVersion != lastChannelVersion_) {
            std::visit([&](auto& a) {
                using T = std::decay_t<decltype(a)>;
                if constexpr (!std::is_same_v<T, std::monostate>) {
                    a.updatePitchBend(channelInfo_.pitchBendRaw);
                }
            }, algorithm_);
        }

        for (size_t i = 0; i < bufToFrameCount(buf); ++i) {
            if (ampEnv_.getState() == ADSRState::IDLE) {
                state_ = VoiceState::IDLE;
                break;
            }

            float sample = std::visit([](auto& a) -> float {
                using T = std::decay_t<decltype(a)>;
                if constexpr(!std::is_same_v<T, std::monostate>) {
                    return a.renderNext();
                }
                return 0.0f;
            }, algorithm_);

            const float ampVal = ampEnv_.advance();
            const float vol = ampVal * (channelInfo_.expression / 127.0f) * (channelInfo_.volume / 127.0f);
            sample *= vol;

            if (buf.channels == 1) {
                buf.data[i * buf.stride] += sample;
            } else {
                const float panF = channelInfo_.pan / 127.0f;
                buf.data[i * buf.stride + 0] += sample * (1 - panF);
                buf.data[i * buf.stride + 1] += sample * panF;
            }
        }
    }
} // namespace dzsungel::core
