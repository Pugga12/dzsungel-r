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
#include "core/AudioEngine.hpp"

namespace dzsungel::core {
    constexpr size_t kMaxEventsPerBlock = 64;

    void AudioEngine::renderBlock(SampleBuffer &buf) {
        const size_t blockStart = currentTimecode_.load(std::memory_order_acquire);
        const size_t blockEnd = blockStart + (buf.data.size() / buf.stride);

        std::array<MidiMsg, kMaxEventsPerBlock> eventBuf_ = {};
        size_t eventsRecieved = 0;

        MidiMsg msg;
        while (eventsRecieved < kMaxEventsPerBlock && messageQueue_.pop(msg)) {
            if (msg.absoluteSample >= blockStart && msg.absoluteSample < blockEnd) {
                eventBuf_[eventsRecieved++] = msg;
            } else if (msg.absoluteSample < blockStart) {
                msg.absoluteSample = blockStart;
                eventBuf_[eventsRecieved++] = msg;
            }
        }

        std::ranges::sort(eventBuf_, [](const auto& a, const auto& b) {
            return a.absoluteSample < b.absoluteSample;
        });

        size_t currentSampleIdx = 0;
        const size_t blockSamples = buf.data.size() / buf.stride;

        for (size_t i = 0; i < eventsRecieved; ++i) {
            const auto& event = eventBuf_[i];
            const size_t targetIdx = event.absoluteSample - blockStart;

            if (targetIdx > currentSampleIdx) {
                // do something
            }

            if (msg.type == MidiMsgType::NoteOn) {
                handleNoteOn(msg);
            }
        }
    }

    void AudioEngine::handleNoteOn(const MidiMsg &msg) {
        auto [status, id] = allocator_.allocate(msg.channel, msg.data1, msg.absoluteSample);
    }
} // namespace dzsungel::core
