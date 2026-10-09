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

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>

namespace dzsungel::core {
    static uint32_t calculateFlatIdx(uint32_t channel, uint32_t pitch) {
        return (channel * 128) + pitch;
    }

    std::pair<uint32_t, VoiceAllocStatus> VoiceAllocator::findVictim(bool channelScoped, uint32_t channel) const {
        uint32_t bestReleasing = -1;
        uint32_t bestActive = -1;
        uint32_t oldestReleasingTime = std::numeric_limits<uint32_t>::max();
        uint32_t oldestActiveTime = std::numeric_limits<uint32_t>::max();

        for (uint32_t i = 0; i < kMaxVoices; ++i) {
            if (channelScoped && !channelVoices_[channel].test(i)) continue;

            if (const auto &v = voices_[i];
                v.status == VoiceSlot::Status::Releasing && v.triggeredAtSample < oldestReleasingTime) {
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

    void VoiceAllocator::bind(uint32_t id, uint32_t channel, uint32_t pitch, uint32_t triggeredAt) {
        auto& v = voices_[id];
        v.status = VoiceSlot::Status::Active;
        v.channel = channel;
        v.pitch = pitch;
        v.triggeredAtSample = triggeredAt;

        channelVoices_[channel].set(id);
        noteToVoice_[calculateFlatIdx(channel, pitch)] = id;
    }

    void VoiceAllocator::unbind(uint32_t id) {
        const auto& oldVoice = voices_[id];

        if (const size_t oldFlatIdx = calculateFlatIdx(oldVoice.channel, oldVoice.pitch);
            noteToVoice_[oldFlatIdx] == id) {
            noteToVoice_[oldFlatIdx] = kNotBound;
        }

        channelVoices_[oldVoice.channel].reset(id);
    }

    VoiceAllocResult VoiceAllocator::allocate(uint32_t channel, uint32_t pitch, uint32_t sampleTime) {
        const size_t flatIdx = calculateFlatIdx(channel, pitch);

        if (const uint32_t existing = noteToVoice_[flatIdx]; existing <= kDuplicateNotes) {
            voices_[existing].triggeredAtSample = sampleTime;
            voices_[existing].status = VoiceSlot::Status::Active;
            return {VoiceAllocStatus::DUPLICATE, existing};
        }

        for (uint32_t i = 0; i < kMaxVoices; ++i) {
            if (voices_[i].status == VoiceSlot::Status::Free) {
                bind(i, channel, pitch, sampleTime);
                incrementActiveCount();
                return {VoiceAllocStatus::FRESH, i};
            }
        }

        auto [victim, status] = findVictim(true, channel);
        if (victim == -1) {
            std::tie(victim, status) = findVictim(false, channel);
        }

        assert(victim >= 0 && victim < kMaxVoices);

        unbind(victim);
        bind(victim, channel, pitch, sampleTime);

        return {status, victim};
    }

    uint32_t VoiceAllocator::release(uint32_t channel, uint32_t pitch) {
        const size_t flatIdx = calculateFlatIdx(channel, pitch);
        const uint32_t boundId = noteToVoice_[flatIdx];

        if (boundId == kNotBound) {
            return kNotBound;
        }

        noteToVoice_[flatIdx] = kNotBound;
        voices_[boundId].status = VoiceSlot::Status::Releasing;

        return boundId;
    }

    void VoiceAllocator::notifyIdle(uint32_t voiceId) {
        auto& v = voices_[voiceId];

        if (const uint32_t flatIdx = calculateFlatIdx(v.channel, v.pitch); noteToVoice_[flatIdx] == voiceId) {
            noteToVoice_[flatIdx] = kNotBound;
        }

        channelVoices_[v.channel].reset(voiceId);
        v.status = VoiceSlot::Status::Free;
        decrementActiveCount();
    }
} // namespace dzsungel::core
