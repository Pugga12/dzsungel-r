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
#include "nlohmann/json.hpp"
using nlohmann::json;

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
    NLOHMANN_JSON_SERIALIZE_ENUM(AlgorithmType,{
        {AlgorithmType::STANDARD_PM, "standardPm"}, 
        {AlgorithmType::FEEDBACK, "feedback"}
    }); 

    struct StandardPmParams {
        std::string_view carrierWavetableName;
        std::string_view modulatorWavetableName;
        float cToMRatio = 0.0f;
        float modIndex = 0.0f;
        EnvelopeConfig modEnv{};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StandardPmParams, carrierWavetableName, modulatorWavetableName, cToMRatio, modIndex, modEnv);

    struct FeedbackParams {
        std::string_view carrierWavetableName;
        float modIndex = 0.0f;
        EnvelopeConfig modEnv{};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FeedbackParams, carrierWavetableName, modIndex, modEnv);

    inline uint32_t packProgramId(uint8_t msb, uint8_t lsb, uint8_t program) {
        return (msb << 16) | (lsb << 8) | program;
    }

    struct Program {
        std::string name = "Default";
        uint32_t packedProgramId = 0;
        AlgorithmType type = AlgorithmType::STANDARD_PM;

        std::variant<StandardPmParams, FeedbackParams> algorithmParams;
        EnvelopeConfig ampEnv{};

        Program() = default;
        Program(
            std::string_view str,
            uint8_t bankMsb, uint8_t bankLsb, uint8_t program,
            StandardPmParams alParams, EnvelopeConfig ampEnv
        ) : name(str), packedProgramId(packProgramId(bankMsb, bankLsb, program)), algorithmParams(alParams), ampEnv(ampEnv) {}

        Program(
            std::string_view str,
            uint8_t bankMsb, uint8_t bankLsb, uint8_t program,
            FeedbackParams alParams, EnvelopeConfig ampEnv
        ) : name(str), packedProgramId(packProgramId(bankMsb, bankLsb, program)), type(AlgorithmType::FEEDBACK), algorithmParams(alParams), ampEnv(ampEnv) {}

        Program(std::string_view str, uint32_t packedId, StandardPmParams alParams, EnvelopeConfig ampEnv)
            : name(str), packedProgramId(packedId), algorithmParams(alParams), ampEnv(ampEnv) {}

        Program(std::string_view str, uint32_t packedId, FeedbackParams alParams, EnvelopeConfig ampEnv)
            : name(str), packedProgramId(packedId), type(AlgorithmType::FEEDBACK),algorithmParams(alParams), ampEnv(ampEnv) {}
    };
 
    inline void from_json(const json& j, Program& p) {
        j.at("name").get_to(p.name);
        j.at("id").get_to(p.packedProgramId);
        j.at("type").get_to(p.type);
        j.at("ampEnv").get_to(p.ampEnv);

        if (p.type == AlgorithmType::STANDARD_PM) {
            StandardPmParams params;
            from_json(j.at("algorithmParams"), params);
            p.algorithmParams = params;
        } else if (p.type == AlgorithmType::FEEDBACK) {
            FeedbackParams params;
            from_json(j.at("algorithmParams"), params);
            p.algorithmParams = params;
        }
    }

    std::optional<AlgorithmImpl> createAlgorithmFromProgram(WavetableStore &s, const Program &p, float sampleRate);

    inline Program kDefaultProgram(
        "Default", packProgramId(0, 8, 48), FeedbackParams{
            "default-sin",
            1.5,
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
