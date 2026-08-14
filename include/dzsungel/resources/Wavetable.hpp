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
#include <utility>
#include <vector>
#include <span>
namespace dzsungel::resources {
    class Wavetable {
    public:
        explicit Wavetable(std::vector<float> samples) : samples_(std::move(samples)) {}

        [[nodiscard]] std::span<const float> data() const noexcept {return samples_; }
        [[nodiscard]] size_t size() const {return samples_.size(); }

    private:
        std::vector<float> samples_;
    };
}