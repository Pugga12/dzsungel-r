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
#include <bitset>
#include <cstddef>
#include <cstdint>

static constexpr size_t kNumChannels = 16;
static constexpr size_t kNumNotes = 128;
static constexpr size_t kMaxVoices = 16;
namespace dzsungel::core {
    enum class VoiceAllocStatus : uint8_t {
        FRESH, STOLEN_FROM_RELEASING, STOLEN_FROM_ACTIVE, DUPLICATE
    };

    struct VoiceAllocResult {
        VoiceAllocStatus status;
        uint8_t voice;
    };

    class VoiceAllocator {
    public:
        static constexpr int8_t kNotBound = -1;
        static constexpr int8_t kDuplicateNotes = -2;

        VoiceAllocResult allocate(uint8_t channel, uint8_t pitch, uint32_t sampleTime);
        uint8_t release(uint8_t channel, uint8_t pitch);
        void notifyIdle(uint8_t voiceId);
        VoiceAllocator() {
            noteToVoice_.fill(kNotBound);
        }
    private:
        struct VoiceSlot {
            enum class Status : uint8_t {Free, Active, Releasing} status = Status::Free;
            uint8_t channel = 255;
            uint8_t pitch = 255;
            uint32_t triggeredAtSample = 0;
        };

        std::array<VoiceSlot, kMaxVoices> voices_;
        std::array<int8_t, kNumChannels * kNumNotes> noteToVoice_{};
        std::array<std::bitset<kMaxVoices>, kNumChannels> channelVoices_;

        bool bind(uint8_t id, uint8_t channel, uint8_t pitch, uint32_t triggeredAt);
        void unbind(uint8_t id);
        std::pair<int, VoiceAllocStatus> findVictim(bool channelScoped, uint8_t channel) const;
    };
}