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
#include <concepts>
#include <cstdint>
#include <variant>

#include "ADSR.hpp"
#include "Oscillators.hpp"
#include "RampingValue.hpp"
#include "resources/SynthProgram.hpp"

using namespace dzsungel::core::oscillators;
using namespace dzsungel::resources;
namespace dzsungel::core::algorithms {
    template <typename T>
    concept Algorithm = requires(T algo, float baseFreqHz, uint8_t velocity, int16_t pitchBendRaw)
    {
        {algo.noteOn(baseFreqHz, velocity)} -> std::same_as<void>;
        {algo.updatePitchBend(pitchBendRaw)} -> std::same_as<void>;
        {algo.renderNext()} -> std::same_as<float>;
        {algo.release()} -> std::same_as<void>;
    };

    class StandardPmAlgorithm {
    private:
        PhaseOsc carrier_, modulator_;
        ADSR modEnv_;
        float cToMRatio_ = 0, modIndex_ = 0;
        float baseFrequency_ = 0;
        RampingValue<float> pitchBendRamp_;

        void setOscillatorFrequencies(float freqHz);

    public:
        StandardPmAlgorithm(PhaseOsc carrier, PhaseOsc modulator, Program& p)
            : carrier_(carrier), modulator_(modulator) {
            const auto& params = std::get<StandardPMParams>(p.algorithmParams);
            modEnv_.configure(params.modEnv);
            cToMRatio_ = params.cToMRatio;
            modIndex_ = params.modIndex;
        }

        void noteOn(float baseFreqHz, uint8_t velocity);
        void updatePitchBend(int16_t pitchBendRaw);
        void release();
        float renderNext();
    };

    class FeedbackAlgorithm {
    private:
        PhaseOsc carrier_;
        ADSR modEnv_;
        float modIndex_ = 0;
        float lastOutput = 0;
        float baseFrequency_ = 0;
        RampingValue<float> pitchBendRamp_;
        void setOscillatorFrequencies(float freqHz);

    public:
        FeedbackAlgorithm(PhaseOsc carrier, Program& p)
            : carrier_(carrier) {
            const auto& params = std::get<FeedbackParams>(p.algorithmParams);
            modIndex_ = params.modIndex;
            modEnv_.configure(params.modEnv);
        }

        void noteOn(float baseFreqHz, uint8_t velocity);
        void updatePitchBend(int16_t pitchBendRaw);
        void release();
        float renderNext();
    };

    using AlgorithmImpl = std::variant<std::monostate, StandardPmAlgorithm, FeedbackAlgorithm>;
    static_assert(Algorithm<StandardPmAlgorithm>);
    static_assert(Algorithm<FeedbackAlgorithm>);
}