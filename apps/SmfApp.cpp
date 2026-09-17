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
#include "CLI/CLI.hpp"

using namespace dzsungel::io;
using namespace dzsungel::midi;

int main(int argc, char** argv) {
    // setup logging and command parsing
    CLI::App app{"Synthesizes notes from a .mid file into audio", "dzsmf"};
    

    float masterGainDb = -6.0f;
    std::string infileName;
    std::string outfileName = "out.wav";
    bool logVerbose = false;
    bool linearGainFlag = false;
    float sampleRate = kDefaultSampleRate;
    auto* optGain = app.add_option("-g,--gain", masterGainDb, "Master gain in dB")->capture_default_str();
    app.add_option("InputFile", infileName, "Path to input .midi file")->required()->check(CLI::ExistingFile);
    app.add_option("-o,--output", outfileName, "The name of the output file to write to.")->capture_default_str();
    app.add_option("-s,--sample-rate", sampleRate, "Output sample rate")->check(CLI::PositiveNumber)->capture_default_str();
    app.add_flag("-v,--verbose", logVerbose, "Enable verbose logging. Does not do much right now, but will later");
    app.add_flag("-l,--linear-gain", linearGainFlag, "Treat specified gain as linear. Requires you to input a gain level.")
            ->needs(optGain);
    app.allow_windows_style_options();

    CLI11_PARSE(app, argc, argv);

    if (logVerbose) {
        spdlog::set_level(spdlog::level::debug);
    }

    float linearizedGain = 0;
    if (!linearGainFlag) {
        linearizedGain = std::pow(10, masterGainDb * 0.05f);
    } else {
        //  gain is set and linearized, pass right through
        linearizedGain = masterGainDb;
    }

    // Load input file
    std::ifstream smfFilestream(infileName, std::ios::binary);
    if (!smfFilestream.is_open()) {
        int errCode = errno;
        spdlog::error("Failed to open file: {}\n\tWhat happened: ({}) {}", infileName, errCode, std::system_category().message(errCode));
        return 1;
    }

    // Parse input file
    AudioEngine eng(sampleRate);
    IOSmf smfReader;
    if (!smfReader.load(smfFilestream, sampleRate)) {
        spdlog::error("Failed to parse MIDI file: {}", infileName);
        return 2;
    }

    // If verbose log level: output the programs preloaded in the midi file
    if (logVerbose) {
        std::string numbers;
        for (const auto& pNo : smfReader.getPreloadIds()) {
            numbers.append(std::to_string(pNo) + ", ");
        }
        spdlog::debug("Preloaded program IDs: {}", numbers);
    }

    // Open outfile; currently will overwrite out.wav
    WAVWriter wavWriter;
    if (!wavWriter.open(outfileName, sampleRate, 2)) {
        spdlog::error("Failed to create WAV file: {}", outfileName);
        return 3;
    }

    // initialize buffer
    size_t framesWritten = 0;
    std::vector<float> buffer(8192);
    std::ranges::fill(buffer, 0.0f);
    SampleBuffer sampleBuf {
        .data = buffer,
        .channels = 2,
        .stride = 2
    };

    auto start = std::chrono::steady_clock::now();
    while (true) {
        // halt if all messages have been queued by the reader and if all voices are inactive
        if (smfReader.isPlaybackComplete() && eng.getActiveVoiceCount() == 0) {
            break;
        }

        smfReader.pushToEngine(eng);
        eng.renderBlock(sampleBuf);

        for (auto &v: buffer) {
            v *= linearizedGain;
        }
        framesWritten += wavWriter.write(sampleBuf);

        std::ranges::fill(buffer, 0.0f);
    }

    // calculate time taken at end of loop
    auto end = std::chrono::steady_clock::now();
    auto finishedIn = end - start;
    std::chrono::duration<float> dSec = finishedIn;
    const float lenToWrite = static_cast<float>(framesWritten) / sampleRate;

    spdlog::info("Done, took {} to process {} s of audio", dSec,  lenToWrite);
    spdlog::info("Speedup: {}x", lenToWrite / dSec.count());
}
