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
#include "resources/SynthProgram.hpp"
#include "core/Algorithms.hpp"
#include "resources/WavetableStore.hpp"

using namespace dzsungel::core::algorithms;

namespace dzsungel::resources {
    std::optional<AlgorithmImpl> createAlgorithmFromProgram(WavetableStore &s, const Program &p, float sampleRate) {
        return std::visit([&](const auto& params) -> std::optional<AlgorithmImpl> {
            using T = std::decay_t<decltype(params)>;
            if constexpr (std::is_same_v<T, StandardPmParams>) {
                const auto carrierTable = s.find(params.carrierWavetableName);
                const auto modTable = s.find(params.modulatorWavetableName);

                if (!carrierTable.has_value() || !modTable.has_value()) return std::nullopt;

                PhaseOsc carrier = WavetableOsc(carrierTable.value(), sampleRate);
                PhaseOsc modulator = WavetableOsc(modTable.value(), sampleRate);

                return StandardPmAlgorithm(carrier, modulator, p, sampleRate);
            } else if constexpr (std::is_same_v<T, FeedbackParams>) {
                const auto carrierTable = s.find(params.carrierWavetableName);
                if (!carrierTable.has_value()) return std::nullopt;

                PhaseOsc carrier = WavetableOsc(carrierTable.value(), sampleRate);

                return FeedbackAlgorithm(carrier, p, sampleRate);
            }
        }, p.algorithmParams);
    }
}
