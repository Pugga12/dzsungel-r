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
#include "MidiFile.h"
#include <unordered_set>

#include "Types.hpp"
#include "core/AudioEngine.hpp"

using namespace smf;

namespace dzsungel::midi {
    constexpr uint32_t kReadaheadBufferLen = 128;

    struct ExtendedProgramState {
        uint8_t msb = 0;
        uint8_t lsb = 0;
    };

    class IOSmf {
    private:
        MidiFile file_;
        std::unordered_set<uint32_t> preloads_;
        std::vector<MidiMsg> events_;
        std::array<ExtendedProgramState, 16> states_ = {};
        bool loaded_ = false;
        size_t eventsQueued_ = 0;
        size_t numEvents = 0;

        void convertTrack(float sampleRate);
    public:
        bool load(const std::string &fName, float sampleRate = kDefaultSampleRate);

        [[nodiscard]] bool isLoaded() const {
            return loaded_;
        }

        void unload();

        void pushToEngine(AudioEngine &e, size_t readahead = kReadaheadBufferLen);
    };
}