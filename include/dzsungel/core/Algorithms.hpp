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

using dzsungel::core::oscillators::PhaseOsc;

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

    public:
        void noteOn(float baseFreqHz, uint8_t velocity);
        void updatePitchBend(uint16_t pitchBendRaw);
        void release();
        float renderNext();
    };

    class FeedbackAlgorithm {
    private:
        PhaseOsc carrier_;
        ADSR modEnv_;
        float feedbackDepth_ = 0;
        float lastOutput = 0;

    public:
        void noteOn(float baseFreqHz, uint8_t velocity);
        void updatePitchBend(uint16_t pitchBendRaw);
        void release();
        float renderNext();
    };

    using AlgorithmImpl = std::variant<std::monostate, StandardPmAlgorithm, FeedbackAlgorithm>;
    static_assert(Algorithm<StandardPmAlgorithm>);
    static_assert(Algorithm<FeedbackAlgorithm>);
}