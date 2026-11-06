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
#include <atomic>

namespace dzsungel::core {
    constexpr size_t kMaxEventsPerBlock = 256;

    size_t AudioEngine::eventRecv(const std::span<MidiMsg> &eventBuf, const size_t blockStart, const size_t blockEnd) {
        size_t eventsRecievedQueue = 0;
        {
            MidiMsg msg;
            while (eventsRecievedQueue < kMaxEventsPerBlock && messageQueue_.pop(msg)) {
                if (msg.absoluteSample >= blockStart && msg.absoluteSample < blockEnd) {
                    eventBuf[eventsRecievedQueue++] = msg;
                } else if (msg.absoluteSample < blockStart) {
                    countLate_.fetch_add(1, std::memory_order_release);
                    countTotal_.fetch_add(1, std::memory_order_release);
                    totalDelay_.fetch_add((static_cast<float>(blockStart) - msg.absoluteSample) / sampleRate_,
                                          std::memory_order_release);
                    msg.absoluteSample = blockStart;
                    eventBuf[eventsRecievedQueue++] = msg;
                } else if (msg.absoluteSample > blockEnd) {
                    countEarly_.fetch_add(1, std::memory_order_release);
                    countTotal_.fetch_add(1, std::memory_order_release);
                    totalDelay_.fetch_add((static_cast<float>(msg.absoluteSample) - blockEnd) / sampleRate_,
                                          std::memory_order_release);
                }
            }
        }

        size_t ivEventsRecieved = 0;
        if (inputTimeline_.data() != nullptr && ivCurrentPosition_ != ivEventCount_) {
            while (ivEventsRecieved + eventsRecievedQueue < kMaxEventsPerBlock && ivCurrentPosition_ < ivEventCount_) {
                const MidiMsg &ev = inputTimeline_[ivCurrentPosition_];

                if (ev.absoluteSample >= blockStart && ev.absoluteSample < blockEnd) {
                    eventBuf[eventsRecievedQueue + ivEventsRecieved++] = ev;
                    ivCurrentPosition_++;
                } else {
                    break;
                }
            }
        }

        const size_t totalRecieved = ivEventsRecieved + eventsRecievedQueue;
        if (eventsRecievedQueue != 0 && ivEventsRecieved != 0) {
            std::sort(eventBuf.begin(), eventBuf.begin() + totalRecieved,
                      [](const auto &a, const auto &b) { return a.absoluteSample < b.absoluteSample; });
        }

        return totalRecieved;
    }

    void AudioEngine::renderBlock(SampleBuffer &buf) {
        const size_t blockStart = currentTimecode_.load(std::memory_order_acquire);
        const size_t blockEnd = blockStart + (buf.data.size() / buf.stride);

        std::array<MidiMsg, kMaxEventsPerBlock> eventBuf = {};
        size_t totalRecieved = eventRecv(eventBuf, blockStart, blockEnd);
        
        size_t currentSampleIdx = 0;
        const size_t blockSamples = buf.data.size() / buf.stride;

        for (size_t i = 0; i < totalRecieved; ++i) {
            const auto& event = eventBuf[i];
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

        if (currentSampleIdx < blockSamples) {
            renderVoices(buf, currentSampleIdx, blockSamples );
        }

        currentTimecode_.fetch_add(blockSamples, std::memory_order_release);
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
            const Program prg = *patches_.find(csi.packedProgId, true).value();

            voice.provision(
                createAlgorithmFromProgram(wavetableStore_, prg, sampleRate_).value()
                , &csi, msg.channel, kDefaultProgram.ampEnv, sampleRate_);
            voice.noteOn(msg.data1, msg.data2);
        }
    }

    void AudioEngine::renderVoices(SampleBuffer &buf, size_t currentIndex, size_t targetIndex) {
        if (size_t fc = bufToFrameCount(buf); currentIndex > fc || targetIndex > fc || currentIndex == targetIndex) {
            return;
        }

        for (uint8_t i = 0; i < voices_.size(); i++) {
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
