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

#include "core/VoiceAllocator.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace dzsungel::core {
    inline size_t calculateFlatIdx(uint8_t channel, uint8_t pitch) {
        return (channel * 128) + pitch;
    }

    std::pair<int, VoiceAllocStatus> VoiceAllocator::findVictim(bool channelScoped, uint8_t channel) const {
        int bestReleasing = -1;
        int bestActive = -1;
        uint32_t oldestReleasingTime = std::numeric_limits<uint32_t>::max();
        uint32_t oldestActiveTime = std::numeric_limits<uint32_t>::max();

        for (uint8_t i = 0; i < kMaxVoices; ++i) {
            if (channelScoped && !channelVoices_[channel].test(i)) continue;

            const auto& v = voices_[i];
            if (v.status == VoiceSlot::Status::Releasing && v.triggeredAtSample < oldestReleasingTime) {
                oldestReleasingTime = v.triggeredAtSample;
                bestReleasing = i;
            } else if (v.status == VoiceSlot::Status::Active && v.triggeredAtSample < oldestActiveTime) {
                oldestActiveTime = v.triggeredAtSample;
                bestActive = i;
            }
        }

        if (bestReleasing != -1) return {bestReleasing, VoiceAllocStatus::STOLEN_FROM_RELEASING};
        if (bestActive != -1) return  {bestActive, VoiceAllocStatus::STOLEN_FROM_ACTIVE};

        return {-1, VoiceAllocStatus::FRESH};
    }

    void VoiceAllocator::bind(uint8_t id, uint8_t channel, uint8_t pitch, uint32_t triggeredAt) {
        auto& v = voices_[id];
        v.status = VoiceSlot::Status::Active;
        v.channel = channel;
        v.pitch = pitch;
        v.triggeredAtSample = triggeredAt;

        channelVoices_[channel].set(channel);
        noteToVoice_[calculateFlatIdx(channel, pitch)] = id;
    }

    void VoiceAllocator::unbind(uint8_t id) {
        const auto& oldVoice = voices_[id];

        if (const size_t oldFlatIdx = calculateFlatIdx(oldVoice.channel, oldVoice.pitch);
            noteToVoice_[oldFlatIdx] == id) {
            noteToVoice_[oldFlatIdx] = kNotBound;
        }

        channelVoices_[oldVoice.channel].reset(id);
    }

    VoiceAllocResult VoiceAllocator::allocate(uint8_t channel, uint8_t pitch, uint32_t sampleTime) {
        const size_t flatIdx = calculateFlatIdx(channel, pitch);

        if (int8_t existing = noteToVoice_[flatIdx]; existing >= 0) {
            auto voiceId = static_cast<uint8_t>(existing);
            voices_[voiceId].triggeredAtSample = sampleTime;
            voices_[voiceId].status = VoiceSlot::Status::Active;
            return {VoiceAllocStatus::DUPLICATE, voiceId};
        }

        for (uint8_t i = 0; i < kMaxVoices; ++i) {
            if (voices_[i].status == VoiceSlot::Status::Free) {
                bind(i, channel, pitch, sampleTime);
                return {VoiceAllocStatus::FRESH, i};
            }
        }

        auto [victim, status] = findVictim(true, channel);
        if (victim == -1) {
            std::tie(victim, status) = findVictim(false, channel);
        }

        const auto finalId = static_cast<uint8_t>(victim);
        unbind(finalId);
        bind(finalId, channel, pitch, sampleTime);

        return {status, finalId};
    }

    int8_t VoiceAllocator::release(uint8_t channel, uint8_t pitch) {
        const size_t flatIdx = calculateFlatIdx(channel, pitch);
        int8_t boundId = noteToVoice_[flatIdx];

        if (boundId == kNotBound) {
            return kNotBound;
        }

        noteToVoice_[flatIdx] = kNotBound;
        voices_[boundId].status = VoiceSlot::Status::Releasing;

        return boundId;
    }

    void VoiceAllocator::notifyIdle(uint8_t voiceId) {
        auto& v = voices_[voiceId];

        const size_t flatIdx = calculateFlatIdx(v.channel, v.pitch);
        if (noteToVoice_[flatIdx] == voiceId) {
            noteToVoice_[flatIdx] = kNotBound;
        }

        channelVoices_[v.channel].reset(voiceId);
        v.status = VoiceSlot::Status::Free;
    }
} // namespace dzsungel::core
