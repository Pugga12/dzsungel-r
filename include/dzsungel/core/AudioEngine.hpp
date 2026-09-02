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
#include <boost/lockfree/spsc_queue.hpp>
#include <atomic>

#include "ChannelState.hpp"
#include "Types.hpp"
#include "Voice.hpp"
#include "VoiceAllocator.hpp"
#include "resources/WavetableStore.hpp"

using namespace dzsungel::resources;
namespace dzsungel::core {
    class AudioEngine {
    private:
        boost::lockfree::spsc_queue<MidiMsg> messageQueue_{256};
        std::atomic<size_t> currentTimecode_ = 0;

        ChannelStateStore csiStore_;
        WavetableStore wavetableStore_;
        VoiceAllocator allocator_;
        std::array<Voice, 16> voices_;
        const AlgorithmImpl tempDefaultAlgorithm_ = createAlgorithmFromProgram(wavetableStore_, kDefaultProgram).value();

        void handleNoteOn(const MidiMsg& msg);
        void renderVoices(SampleBuffer& buf, size_t currentIndex, size_t targetIndex);

    public:
        [[nodiscard]] size_t getCurrentTimecode() const {
            return currentTimecode_.load(std::memory_order_acquire);
        }

        bool midiPush(const MidiMsg& msg) {
            return messageQueue_.push(msg);
        }

        void renderBlock(SampleBuffer& buf);
    };
}