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
#include "spdlog/fmt/ranges.h"
#include "CLI/CLI.hpp"
#include "RtAudio.h"

using namespace dzsungel::io;
using namespace dzsungel::midi;

void listOutputDevices(RtAudio &dac) { 
    auto deviceIds = dac.getDeviceIds();

    if (deviceIds.empty()) {
        spdlog::error("No devices detected on this system.");
        return;
    }

    std::vector<dz_uint> outputDeviceIds;
    for (auto &d: deviceIds) {
        auto dev = dac.getDeviceInfo(d);
        if (dev.outputChannels > 0 && dev.nativeFormats & RTAUDIO_FLOAT32) {
            outputDeviceIds.push_back(d);
        }
    }

    if (outputDeviceIds.empty()) {
        spdlog::error("No compatible output devices detected!");
        spdlog::info("All devices: ");
        for (auto &d: deviceIds) {
            auto dev = dac.getDeviceInfo(d);
            spdlog::info("    ({:03d}) {} in / {} ch out: {}", 
                d, dev.inputChannels, dev.outputChannels, dev.name);
        }

        return;
    }

    dz_uint defaultId = dac.getDefaultOutputDevice();
    bool hasDefault = (defaultId != 0 
        && std::ranges::find(outputDeviceIds, defaultId) != outputDeviceIds.end());

    spdlog::info("Available output devices: ");
    for (auto &d : outputDeviceIds) {
        auto dev = dac.getDeviceInfo(d);
        spdlog::info("    ({:03d}) {} ch out: {} {}",
            d, dev.outputChannels, dev.name, 
            (hasDefault && d == defaultId) ? "(default)" : ""
        );
    }

    if (!hasDefault) {
        spdlog::warn("No default output device configured - you must specify one with --device");
    }
}

bool loadMidiFile(const std::string& path, IOSmf& smfReader, float sampleRate) { 
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        int errCode = errno;
        spdlog::error("Failed to open input file '{}': {}", 
            path, std::system_category().message(errCode));

        return false;
    }

    if (!smfReader.load(file, sampleRate)) {
        spdlog::error("Failed to parse MIDI file '{}'. Is it a valid smf file?", path);
        return false;
    }
    
    return true;
}

static bool offlineMain(AudioEngine &eng, IOSmf &smfReader, std::string &outfileName, float sampleRate, bool monoFlag,
                      float linearizedGain) {
    WAVWriter wavWriter;
    if (!wavWriter.open(outfileName, sampleRate, 2)) {
        spdlog::error("Failed to create WAV file: {}", outfileName);
        return false;
    }

    // initialize buffer
    size_t framesWritten = 0;
    std::vector<float> buffer(8192);
    std::ranges::fill(buffer, 0.0f);
    SampleBuffer sampleBuf{.data = buffer, .channels = monoFlag ? 1u : 2u, .stride = monoFlag ? 1u : 2u};

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

    spdlog::info("Done, took {} to process {}s of audio", dSec, lenToWrite);
    spdlog::info("Speedup: {}x", lenToWrite / dSec.count());

    return true;
}

int main(int argc, char **argv) { 
    CLI::App app{"Dzsungel - MIDI synthesizer", "dzsmf"}; 
    app.set_version_flag("--version,--ver", "0.1.0");
    app.fallthrough();

    // Common options
    float masterGainDb = -6.0f;
    float sampleRate = kDefaultSampleRate;
    bool logVerbose = false;
    bool linearGainFlag = false;
    bool monoFlag = false;
    std::string infileName;

    // Input file
    app.add_option("INFILE", infileName, "Input MIDI file")
        ->required()
        ->check(CLI::ExistingFile);

    // Gain option
    auto gainOpt = app.add_option("-g,--gain", masterGainDb, "Master gain in dB")->default_str("(default -6dB)");

    // Sample rate 
    app.add_option("-s,--sample-rate", sampleRate, "Output sample rate (Hz)")
        ->check(CLI::PositiveNumber)
        ->capture_default_str();

    // Verbose logging flag
    app.add_flag("-v,--verbose", logVerbose, "Enable debug logging");

    // Linear gain flag
    app.add_flag("-l,--linear-gain", linearGainFlag, "Interpret --gain as linear multiplier (not dB)")
        ->needs(gainOpt);

    // Mono flag 
    app.add_flag("-m,--mono", monoFlag, "Output mono audio (single channel)");

    // SUBCOMMAND: render (wav output)
    auto renderCmd = app.add_subcommand("render", "Render MIDI to wav file (default mode)");
    
    std::string outfileName = "out.wav";
    renderCmd->add_option("-o,--output", outfileName, "Output WAV filename")->capture_default_str();

    // SUBCOMMAND: play
    auto playCmd = app.add_subcommand("play", "Streams audio to an output device (experimental)");

    std::optional<dz_uint> deviceIdOpt;
    playCmd->add_option("-d,--device", deviceIdOpt, "Output device ID")->check(CLI::PositiveNumber);

    bool listDevices = false;
    playCmd->add_flag("-L,--list-devices", listDevices, "List available output devices and exit");

    app.allow_windows_style_options();
   
    CLI11_PARSE(app, argc, argv);

    if (logVerbose) {
        spdlog::set_level(spdlog::level::debug);
        spdlog::debug("Debug logging on.");
    }

    float linearizedGain = linearGainFlag ? masterGainDb : std::pow(10.0f, masterGainDb * 0.05f);

    if (renderCmd->parsed()) {
        AudioEngine eng(sampleRate);
        IOSmf smfReader;

        if (!loadMidiFile(infileName, smfReader, sampleRate)) {
            return 1;
        }

        if (logVerbose) {
            std::vector<uint32_t> ids(
                smfReader.getPreloadIds().begin(), 
                smfReader.getPreloadIds().end()
            );

            spdlog::debug("Preloaded programs: {:#x}", fmt::join(ids, ", "));
        }

        bool success = offlineMain(eng, smfReader, outfileName, sampleRate, monoFlag, linearizedGain);

        return success ? 0 : 2;
    }
}
