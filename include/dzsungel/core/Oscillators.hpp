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
#include <variant>

#include "Constants.hpp"
#include "resources/Wavetable.hpp"
using dzsungel::resources::Wavetable;

namespace dzsungel::core::oscillators {
    template <typename T>
    concept PhaseAddressableOscillator = requires(T osc, const T constOsc, float phase, float phaseIncrement)
    {
        {constOsc.get()} -> std::same_as<float>;
        {constOsc.at(phase)} -> std::same_as<float>;
        {osc.advance()} -> std::same_as<void>;
    };

    class WavetableOsc {
    private:
        explicit WavetableOsc(const Wavetable* table) : table_(table), tableSize_(table->size()) {}

        float phase_ = 0.0f;
        float phaseIncrement_ = 0.0f;
        const Wavetable* table_;
        const size_t tableSize_;

    public:
        [[nodiscard]] float get() const;
        [[nodiscard]] float at(float phase) const;
        void advance();
        void frequencySet(float frequency, float sampleRate = kDefaultSampleRate);
    };

    using PhaseOsc = std::variant<WavetableOsc>;
    static_assert(PhaseAddressableOscillator<WavetableOsc>);
}