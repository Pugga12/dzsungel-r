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
#include <variant>

#include "WavetableStore.hpp"
#include "core/ADSR.hpp"

namespace dzsungel::core::algorithms {
    class StandardPmAlgorithm;
    class FeedbackAlgorithm;
    using AlgorithmImpl = std::variant<std::monostate, StandardPmAlgorithm, FeedbackAlgorithm>;
}

using namespace dzsungel::core;
using namespace dzsungel::core::algorithms;
namespace dzsungel::resources {
    enum class AlgorithmType {
        STANDARD_PM,
        FEEDBACK
    };

    struct StandardPMParams {
        std::string_view carrierTblName;
        std::string_view modulatorTblName;
        float cToMRatio = 0.0f;
        float modIndex = 0.0f;
        EnvelopeConfig modEnv{};
    };

    struct FeedbackParams {
        std::string_view carrierTblName;
        float modIndex = 0.0f;
        EnvelopeConfig modEnv{};
    };

    inline uint32_t packProgramId(uint8_t msb, uint8_t lsb, uint8_t program) {
        return (msb << 16) | (lsb << 8) | program;
    }

    struct Program {
        std::string name = "Default";
        uint32_t packedProgramId = 0;
        AlgorithmType type = AlgorithmType::STANDARD_PM;

        std::variant<StandardPMParams, FeedbackParams> algorithmParams;
        EnvelopeConfig ampEnv{};

        Program() = default;
        Program(
            std::string_view str,
            uint8_t bankMsb, uint8_t bankLsb, uint8_t program,
            StandardPMParams alParams, EnvelopeConfig ampEnv
        ) : name(str), packedProgramId(packProgramId(bankMsb, bankLsb, program)), algorithmParams(alParams), ampEnv(ampEnv) {}

        Program(
            std::string_view str,
            uint8_t bankMsb, uint8_t bankLsb, uint8_t program,
            FeedbackParams alParams, EnvelopeConfig ampEnv
        ) : name(str), packedProgramId(packProgramId(bankMsb, bankLsb, program)), type(AlgorithmType::FEEDBACK), algorithmParams(alParams), ampEnv(ampEnv) {}

        Program(std::string_view str, uint32_t packedId, StandardPMParams alParams, EnvelopeConfig ampEnv)
            : name(str), packedProgramId(packedId), algorithmParams(alParams), ampEnv(ampEnv) {}

        Program(std::string_view str, uint32_t packedId, FeedbackParams alParams, EnvelopeConfig ampEnv)
            : name(str), packedProgramId(packedId), type(AlgorithmType::FEEDBACK),algorithmParams(alParams), ampEnv(ampEnv) {}
    };

    std::optional<AlgorithmImpl> createAlgorithmFromProgram(WavetableStore &s, const Program &p);

    inline Program kDefaultProgram(
        "Default", packProgramId(0, 8, 48), FeedbackParams{
            "default-sin",
            0.5,
            EnvelopeConfig{
                std::chrono::duration<float>(0.01),
                std::chrono::duration<float>(0.2),
                0.5,
                std::chrono::duration<float>(0.5),
                false
            }
        },
        EnvelopeConfig{
            std::chrono::duration<float>(0.01),
            std::chrono::duration<float>(0.4),
            0.5,
            std::chrono::duration<float>(0.5),
            false
        }
    );
}
