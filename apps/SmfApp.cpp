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
#include "magic_enum/magic_enum.hpp"
#include "magic_enum/magic_enum_format.hpp"

using namespace dzsungel::io;
using namespace dzsungel::midi;

static bool g_monoFlag = false;
static bool g_logVerbose = false;
static float g_linearizedGain = 0.0f;

static struct AudioStatus {
    std::atomic_bool hasError{false};
    std::atomic<RtAudioErrorType> type{RTAUDIO_NO_ERROR};
} g_status;

static void listOutputDevices(RtAudio &dac) { 
    std::vector<uint> deviceIds = dac.getDeviceIds();

    if (deviceIds.empty()) {
        spdlog::error("No devices detected on this system.");
        return;
    }

    std::vector<uint> outputDeviceIds;
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

    uint defaultId = dac.getDefaultOutputDevice();
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

static std::optional<uint> resolveDeviceId(RtAudio& dac, std::optional<uint> deviceIdOpt) {
    uint defaultId = dac.getDefaultOutputDevice();
    std::vector<uint> deviceIds = dac.getDeviceIds();

    if (!deviceIdOpt.has_value()) {
        if (defaultId != 0) {
            spdlog::debug("Selected default device {} ({})", defaultId, dac.getDeviceInfo(defaultId).name);
            return defaultId;
        }
        listOutputDevices(dac);
        return std::nullopt;
    }

    uint requestedId = deviceIdOpt.value();
    bool valid = std::ranges::find(deviceIds, requestedId) != deviceIds.end();

    if (!valid) {
        spdlog::error("Device {} does not exist. Specify a valid ID.", requestedId);
        listOutputDevices(dac);
        return std::nullopt;
    }

    RtAudio::DeviceInfo dev = dac.getDeviceInfo(requestedId);

    if (dev.outputChannels == 0) {
        spdlog::error("Device {} ({}) is not an output device. Specify a valid output device ID.", requestedId, dev.name);
        listOutputDevices(dac);
        return std::nullopt;
    }

    spdlog::debug("Selected output device: {} ({})", requestedId, dev.name);
    return requestedId;
}

static bool loadMidiFile(const std::string& path, IOSmf& smfReader, float sampleRate) { 
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

static int xOutputCallback(void *outputBuffer, void *, unsigned int nFrames, double streamTime, RtAudioStreamStatus status,
                    void *userData) {
    auto buffer = static_cast<float *>(outputBuffer);
    auto eng = static_cast<AudioEngine *>(userData);

    SampleBuffer buf{
	    .data = std::span<float>(buffer, nFrames * 2), 
	    .channels = static_cast<uint8_t>(g_monoFlag ? 1 : 2), 
	    .stride = 2
    };
    std::ranges::fill(buf.data, 0.0f);
    eng->renderBlock(buf);

    if (g_monoFlag) {
        for (uint i = 0; i + 1 < nFrames * 2; i += 2) {
            buffer[i] *= g_linearizedGain;
            buffer[i + 1] = buffer[i];
        }
    } else {
        for (uint i = 0; i < nFrames * 2; ++i) {
            buffer[i] *= g_linearizedGain;
        }
    }

    return 0;
}

void xErrorCallback(RtAudioErrorType type, const std::string &errorText) { 
#ifdef WIN32
    // likely disconnect in my experience on windows. usually returns "RtApiWasapi::wasapiThread: Unable to retrieve render buffer size"
    if (type == RTAUDIO_DRIVER_ERROR) {
        errorText.rfind("render buffer size");
        // re-emit as disconnect
        g_status.type.store(RTAUDIO_DEVICE_DISCONNECT, std::memory_order_release);
        g_status.hasError.store(true, std::memory_order_release);
        return;
    }
#endif

    g_status.type.store(type, std::memory_order_release);
    g_status.hasError.store(true, std::memory_order_release);
}

static bool offlineMain(std::string &infileName, std::string &outfileName, float sampleRate) {
    AudioEngine eng(sampleRate);
    IOSmf smfReader(4096);

    if (!loadMidiFile(infileName, smfReader, sampleRate)) {
        return false;
    }

    if (g_logVerbose) {
        std::vector<uint32_t> ids(smfReader.getPreloadIds().begin(), smfReader.getPreloadIds().end());
        spdlog::debug("Preloaded programs: {:#x}", fmt::join(ids, ", "));
    }

    WAVWriter wavWriter;
    if (!wavWriter.open(outfileName, sampleRate, 2)) {
        spdlog::error("Failed to create WAV file: {}", outfileName);
        return false;
    }

    // initialize buffer
    size_t framesWritten = 0;
    std::vector<float> buffer(8192);
    std::ranges::fill(buffer, 0.0f);
    SampleBuffer sampleBuf{.data = buffer, .channels = static_cast<uint8_t>(g_monoFlag ? 1 : 2), .stride = static_cast<uint8_t>(g_monoFlag ? 1 : 2)};

    auto start = std::chrono::steady_clock::now();
    while (true) {
        // halt if all messages have been queued by the reader and if all voices are inactive
        if (smfReader.isPlaybackComplete() && eng.getActiveVoiceCount() == 0) {
            break;
        }

        smfReader.pushToEngine(eng);
        eng.renderBlock(sampleBuf);

        for (auto &v: buffer) {
            v *= g_linearizedGain;
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

static bool liveMain(RtAudio &dac, std::string &infileName, uint devId, float sampleRate) {
    AudioEngine eng(sampleRate);
    IOSmf smfReader;

    if (!loadMidiFile(infileName, smfReader, sampleRate)) {
        return false;
    }

    if (g_logVerbose) {
        std::vector<uint32_t> ids(smfReader.getPreloadIds().begin(), smfReader.getPreloadIds().end());
        spdlog::debug("Preloaded programs: {:#x}", fmt::join(ids, ", "));
    }

    uint bufferSize = 64;
    RtAudio::StreamParameters oParams{
            .deviceId = devId,
            .nChannels = 2,
    };
    RtAudio::StreamOptions options{.flags = RTAUDIO_SCHEDULE_REALTIME};

    if (dac.openStream(&oParams, nullptr, RTAUDIO_FLOAT32, static_cast<uint>(sampleRate), &bufferSize, xOutputCallback,
        &eng, &options)) {
        spdlog::error("Error while opening audio stream: {}", dac.getErrorText());
        return false;
    }

    if (dac.startStream()) {
        spdlog::error("Error when starting audio stream: {}", dac.getErrorText());
        if (dac.isStreamOpen())
            dac.closeStream();
        return false;
    }

    while (true) {
        // halt if all messages have been queued by the reader and if all voices are inactive
        if (smfReader.isPlaybackComplete() && eng.getActiveVoiceCount() == 0) {
            break;
        }

        smfReader.pushToEngine(eng);

        if (g_status.hasError.load(std::memory_order_acquire)) {
            RtAudioErrorType t = g_status.type.load(std::memory_order_acquire);

            if (t == RTAUDIO_DEVICE_DISCONNECT) {
                spdlog::critical("Output device disconnected, halting...");
                break;
            } else if (t == RTAUDIO_MEMORY_ERROR ||
                t == RTAUDIO_DRIVER_ERROR || 
                t == RTAUDIO_SYSTEM_ERROR || 
                t == RTAUDIO_THREAD_ERROR) {
                spdlog::critical("RtAudio: {}", t);
                break;
            }

            g_status.hasError.store(false, std::memory_order_release);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    dac.stopStream();
    dac.closeStream();

    return true;
};

int main(int argc, char **argv) { 
    CLI::App app{"Dzsungel - MIDI synthesizer", "dzsmf"}; 
#ifdef WIN32
    app.allow_windows_style_options();
#endif
    app.require_subcommand(1);
    app.set_version_flag("--version,--ver", "0.1.0");
    app.set_help_all_flag("-H,--help-all", "All options");
    app.fallthrough();

    // Common options
    float masterGainDb = -6.0f;
    float sampleRate = kDefaultSampleRate;
    bool linearGainFlag = false;
    std::string infileName;
    
    // Gain option
    auto gainOpt = app.add_option("-g,--gain", masterGainDb, "Master gain in dB")->default_str("(default -6dB)");

    // Sample rate 
    app.add_option("-s,--sample-rate", sampleRate, "Output sample rate (Hz)")
        ->check(CLI::PositiveNumber)
        ->capture_default_str();

    // Verbose logging flag
    app.add_flag("-v,--verbose", g_logVerbose, "Enable debug logging");

    // Linear gain flag
    app.add_flag("-l,--linear-gain", linearGainFlag, "Interpret --gain as linear multiplier (not dB)")
        ->needs(gainOpt);

    // Mono flag 
    app.add_flag("-m,--mono", g_monoFlag, "Output mono audio (single channel)");

    // SUBCOMMAND: render (wav output)
    auto renderCmd = app.add_subcommand("render", "Render MIDI to wav file (default mode)");

    renderCmd->add_option("INFILE", infileName, "Input MIDI file")->required()->check(CLI::ExistingFile);
    std::string outfileName = "out.wav";
    renderCmd->add_option("-o,--output", outfileName, "Output WAV filename")->capture_default_str();

    // SUBCOMMAND: play
    auto playCmd = app.add_subcommand("play", "Streams audio to an output device (experimental)");
    playCmd->add_option("INFILE", infileName, "Input MIDI file")->required()->check(CLI::ExistingFile);

    std::optional<uint> deviceIdOpt;
    playCmd->add_option("-d,--device", deviceIdOpt, "Output device ID")->check(CLI::PositiveNumber);

    bool listDevices = false;
    playCmd->add_flag("-L,--list-devices", listDevices, "List available output devices and exit");
   
    CLI11_PARSE(app, argc, argv);

    if (g_logVerbose) {
        spdlog::set_level(spdlog::level::debug);
        spdlog::debug("Debug logging on.");
    }

    g_linearizedGain = linearGainFlag ? masterGainDb : std::pow(10.0f, masterGainDb * 0.05f);

    if (renderCmd->parsed()) {
        bool success = offlineMain(infileName, outfileName, sampleRate);
        return success ? 0 : 2;
    } else if (playCmd->parsed()) {
        RtAudio dac(RtAudio::Api::UNSPECIFIED, xErrorCallback);

        if (listDevices) {
            listOutputDevices(dac);
            return 0;
        }

        std::optional<uint> devId = resolveDeviceId(dac, deviceIdOpt);
        if (!devId.value()) {
            return 3;
        }
        
        bool success = liveMain(dac, infileName, devId.value(), sampleRate);
        return success ? 0 : 3;
    }
}
