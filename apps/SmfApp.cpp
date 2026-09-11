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
#include "io/Wav.hpp"
#include "midi/Smf.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/chrono.h"

using namespace dzsungel::io;
using namespace dzsungel::midi;

int main() {
    spdlog::set_level(spdlog::level::debug);

    const std::string filename = "/home/adama/midi/monty-2.mid";
    std::ifstream smfFilestream(filename, std::ios::binary);
    if (!smfFilestream.is_open()) {
        spdlog::error("Failed to open file: {}\n\tWhat happened: {}", filename, std::strerror(errno));
        return 1;
    }

    AudioEngine eng;
    IOSmf smfReader;
    if (!smfReader.load(smfFilestream)) {
        spdlog::error("Failed to parse MIDI file: {}", filename);
        return 2;
    }

    if (spdlog::get_level() == spdlog::level::debug) {
        std::string numbers;
        for (const auto& pNo : smfReader.getPreloadIds()) {
            numbers.append(std::to_string(pNo) + ", ");
        }
        spdlog::debug("Preloaded program IDs: {}", numbers);
    }

    WAVWriter wavWriter;
    if (const std::string outFile = "out.wav"; !wavWriter.open(outFile)) {
        spdlog::error("Failed to create WAV file: {}", outFile);
    }

    size_t framesWritten = 0;
    std::vector<float> buffer(64);
    std::ranges::fill(buffer, 0.0f);
    SampleBuffer sampleBuf {
        .data = buffer,
        .channels = 1,
        .stride = 1
    };

    auto start = std::chrono::steady_clock::now();
    while (true) {
        if (smfReader.isPlaybackComplete() && eng.getActiveVoiceCount() == 0) {
            break;
        }

        smfReader.pushToEngine(eng);
        eng.renderBlock(sampleBuf);
        framesWritten += wavWriter.write(sampleBuf);

        std::ranges::fill(buffer, 0.0f);
    }
    auto end = std::chrono::steady_clock::now();
    auto finishedIn = end - start;
    std::chrono::duration<float> dSec = finishedIn;
    const float lenToWrite = static_cast<float>(framesWritten) / kDefaultSampleRate;

    spdlog::info("Done, took {} to process {} s of audio", dSec,  lenToWrite);
    spdlog::info("Speedup: {}x", lenToWrite / dSec.count());
}
