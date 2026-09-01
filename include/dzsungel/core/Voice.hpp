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
#include "Algorithms.hpp"
#include "ChannelState.hpp"

using namespace dzsungel::core::algorithms;
namespace dzsungel::core {
    enum class VoiceState {
        ACTIVE, RELEASING, IDLE
    };

    class Voice {
    private:
        AlgorithmImpl algorithm_;
        ADSR ampEnv_;
        const ChannelState* channelInfo_ = nullptr;
        VoiceState state_ = VoiceState::IDLE;

        uint8_t currentPitch_ = 255;
        uint8_t currentVelocity_ = 0;
        uint32_t lastChannelVersion_ = 0;

        uint8_t channelId_ = 0xff;
        uint32_t packedProgram_ = 0;
    public:
        void provision(
            const AlgorithmImpl& newAlgorithm,
            const ChannelState* channel,
            const uint8_t cId,
            const EnvelopeConfig &ampEnvConfig
        ) {
            algorithm_ = newAlgorithm;
            ampEnv_.configure(ampEnvConfig);
            channelInfo_ = channel;
            channelId_ = cId;
            packedProgram_ = channel->packedProgId;
        }

        void noteOn(uint8_t noteNumber, uint8_t velocity);
        void noteOff();
        void processBlock(SampleBuffer& buf);

        [[nodiscard]] VoiceState getState() const {
            return state_;
        }
        [[nodiscard]] uint8_t getChannel() const {
            return channelId_;
        }
        [[nodiscard]] uint32_t getProgramId() const {
            return packedProgram_;
        }
    };
}