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
    std::optional<AlgorithmImpl> createAlgorithmFromProgram(WavetableStore& s, Program& p) {
        if (std::holds_alternative<StandardPMParams>(p.algorithmParams)) {
            const auto& params = std::get<StandardPMParams>(p.algorithmParams);

            const auto carrierTable = s.find(params.carrierTblName);
            const auto modTable = s.find(params.modulatorTblName);

            if (!carrierTable.has_value() || !modTable.has_value()) return std::nullopt;

            PhaseOsc carrier = WavetableOsc(carrierTable.value());
            PhaseOsc modulator = WavetableOsc(modTable.value());

            return std::make_optional(
                StandardPmAlgorithm(carrier, modulator, p)
            );
        } else {
            const auto& params = std::get<FeedbackParams>(p.algorithmParams);

            const auto carrierTable = s.find(params.carrierTblName);
            if (!carrierTable.has_value()) return std::nullopt;

            PhaseOsc carrier = WavetableOsc(carrierTable.value());

            return std::make_optional(
                FeedbackAlgorithm(carrier, p)
            );
        }
    }
}
