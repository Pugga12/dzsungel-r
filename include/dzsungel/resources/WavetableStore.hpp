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
#include "Wavetable.hpp"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

constexpr size_t STD_LENGTH = 4096;
namespace dzsungel::resources {
    class WavetableStore {
    public:
        const Wavetable* generateSine(std::string_view id, size_t length = STD_LENGTH);
        const Wavetable* generateTriangle(std::string_view id, size_t length = STD_LENGTH);

        std::optional<const Wavetable*> find(std::string_view id) const;

        WavetableStore() {
            generateSine("default-sin");
            generateTriangle("default-tri");
        }

    private:
        std::unordered_map<std::string, std::unique_ptr<const Wavetable>> tables_;
    };
}