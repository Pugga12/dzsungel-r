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

#include "core/Algorithms.hpp"

#include <cmath>

namespace dzsungel::core::algorithms {
    void StandardPmAlgorithm::setOscillatorFrequencies(float freqHz) {
        std::visit([&](auto& osc){
            osc.frequencySet(freqHz);
        }, carrier_);
        std::visit([&](auto& osc) {
            osc.frequencySet(freqHz * cToMRatio_);
        }, modulator_);
    }

    void StandardPmAlgorithm::noteOn(float baseFreqHz, uint8_t velocity) {
        baseFrequency_ = baseFreqHz;
        pitchBendRamp_.reset(baseFreqHz, baseFreqHz, 64);
        modEnv_.trigger();
    }

    void StandardPmAlgorithm::updatePitchBend(int16_t pitchBendRaw) {
        float bendSemitones = (pitchBendRaw / 8192.0f) * 2.0f;
        float ratio = std::exp2(bendSemitones / 12.0f);
        float targetFreq = baseFrequency_ * ratio;

        pitchBendRamp_.reset(pitchBendRamp_.isFinished() ? baseFrequency_ : pitchBendRamp_.next(), targetFreq, 64);
    }

    void StandardPmAlgorithm::release() { modEnv_.release(); }

    float StandardPmAlgorithm::renderNext() {
        if (!pitchBendRamp_.isFinished()) {
            setOscillatorFrequencies(pitchBendRamp_.next());
        }

        const float modEnvVal = modEnv_.advance();
        const float modVal = std::visit([&](auto& mod) {
            return mod.get();
        }, modulator_);

        const float currentModDepth = modIndex_ *
            std::visit([&](auto& osc) {
                return static_cast<float>(osc.getTableSize()) / 8;
            }, carrier_)
            * modEnvVal;
        const float delta = modVal * currentModDepth;
        const float retVal = std::visit([&](auto& osc){ return osc.get(delta); }, carrier_);

        std::visit([&](auto& osc){ osc.advance(); }, carrier_);
        std::visit([&](auto& osc){ osc.advance(); }, modulator_);

        return retVal;
    }

    void FeedbackAlgorithm::noteOn(float baseFreqHz, uint8_t velocity) {
        std::visit([&](auto& osc){ osc.frequencySet(baseFreqHz); }, carrier_);
        modEnv_.trigger();
    }

    void FeedbackAlgorithm::updatePitchBend(int16_t pitchBendRaw) {
        float bendSemitones = (pitchBendRaw / 8192.0f) * 2.0f;
        float ratio = std::exp2(bendSemitones / 12.0f);
        float targetFreq = baseFrequency_ * ratio;

        pitchBendRamp_.reset(pitchBendRamp_.isFinished() ? baseFrequency_ : pitchBendRamp_.next(), targetFreq, 64);
    }

    void FeedbackAlgorithm::release() { modEnv_.release(); }

    float FeedbackAlgorithm::renderNext() {
        const float modEnvVal = modEnv_.advance();
        const float feedbackDepth = modEnvVal * modIndex_;

        const float delta = lastOutput *
            std::visit([&](auto& osc) {
                return static_cast<float>(osc.getTableSize()) / std::numbers::pi_v<float>;
            }, carrier_)
            * feedbackDepth;

        const float retVal = std::visit([&](auto& osc){ return osc.get(delta); }, carrier_);

        std::visit([&](auto& osc){ osc.advance(); }, carrier_);
        return retVal;
    }
} // namespace dzsungel::core::algorithms
