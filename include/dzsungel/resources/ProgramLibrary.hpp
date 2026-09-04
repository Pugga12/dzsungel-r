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
#include <cstdint>
#include <unordered_map>

#include "SynthProgram.hpp"

namespace dzsungel::resources {
    class ProgramLibrary {
    private:
        std::unordered_map<uint32_t, Program> store_;
        Program defaultEntry_ = kDefaultProgram;
    public:
        [[nodiscard]] std::optional<const Program*> find(uint32_t packedId, bool useDefaultOnFail = true) {
            const auto it = store_.find(packedId);

            if (it == store_.end()) {
                if (useDefaultOnFail) {
                    return std::make_optional(&defaultEntry_);
                }
                return std::nullopt;
            }

            return std::make_optional(&it->second);
        }

        [[nodiscard]] std::optional<const Program*> find(uint8_t msb, uint8_t lsb, uint8_t program, bool useDefaultOnFail = true) {
            return find(packProgramId(msb, lsb, program), useDefaultOnFail);
        }

        bool add(Program& p) {
            auto [it, inserted] = store_.emplace(p.packedProgramId, p);
            return inserted;
        }

        size_t add(std::span<Program> pList) {
            size_t added = 0;
            for (auto& p : pList) {
                if (add(p)) {
                    added++;
                }
            }

            return added;
        }
    };
}