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
        {
            MidiMsg msg;
            while (eventsRecieved < kMaxEventsPerBlock && messageQueue_.pop(msg)) {
                if (msg.absoluteSample >= blockStart && msg.absoluteSample < blockEnd) {
                    eventBuf_[eventsRecieved++] = msg;
                } else if (msg.absoluteSample < blockStart) {
                    msg.absoluteSample = blockStart;
                    eventBuf_[eventsRecieved++] = msg;
                }
            }
        }

//        std::ranges::sort(eventBuf_, [](const auto& a, const auto& b) {
//            return a.absoluteSample < b.absoluteSample;
//        });

        size_t currentSampleIdx = 0;
        const size_t blockSamples = buf.data.size() / buf.stride;

        for (size_t i = 0; i < eventsRecieved; ++i) {
            const auto& event = eventBuf_[i];
            size_t targetIdx = event.absoluteSample - blockStart;

            if (targetIdx > currentSampleIdx) {
                renderVoices(buf, currentSampleIdx, targetIdx);
                currentSampleIdx = targetIdx;
            }

            if (event.type == MidiMsgType::NoteOn) {
                handleNoteOn(event);
            } else if (event.type == MidiMsgType::NoteOff) {
                const int8_t result = allocator_.release(event.channel, event.data1);
                if (result != VoiceAllocator::kNotBound) {
                    voices_[result].noteOff();
                }
            } else {
                csiStore_.apply(event);
            }
        }

        currentTimecode_.fetch_add(currentSampleIdx, std::memory_order_release);
    }

    void AudioEngine::handleNoteOn(const MidiMsg &msg) {
        auto [status, id] = allocator_.allocate(msg.channel, msg.data1, msg.absoluteSample);
        auto& voice = voices_[id];
        uint8_t cId = voice.getChannel();

        if (
            status == VoiceAllocStatus::DUPLICATE
            || (cId == msg.channel && csiStore_.get(cId).packedProgId == voice.getProgramId())
        ) {
            voice.noteOn(msg.data1, msg.data2);
        } else {
            const ChannelState& csi = csiStore_.get(msg.channel);
            const Program* prg = patches_.find(csi.packedProgId, true).value();

            voice.provision(
                createAlgorithmFromProgram(wavetableStore_, *prg).value()
                , &csi, msg.channel, kDefaultProgram.ampEnv);
            voice.noteOn(msg.data1, msg.data2);
        }
    }

    void AudioEngine::renderVoices(SampleBuffer &buf, size_t currentIndex, size_t targetIndex) {
        if (size_t fc = bufToFrameCount(buf); currentIndex > fc || targetIndex > fc || currentIndex == targetIndex) {
            return;
        }

        for (size_t i = 0; i < voices_.size(); i++) {
            auto& v = voices_[i];
            VoiceState state = v.getState();
            if (state == VoiceState::IDLE && v.getIdleDirtyFlag()) {
                allocator_.notifyIdle(i);
                continue;
            } else if (state == VoiceState::IDLE) {
                continue;
            }

            v.processBlock(buf, currentIndex, targetIndex);
        }
    }
} // namespace dzsungel::core
