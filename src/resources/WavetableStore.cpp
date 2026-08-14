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

#include "resources/WavetableStore.hpp"

#include <cmath>

namespace dzsungel::resources {
    const Wavetable *WavetableStore::generateSine(std::string_view id, size_t length) {
        std::vector<float> v(length);

        for (size_t i = 0; i < length; ++i) {
            const float theta = (2 * std::numbers::pi_v<float>) * (static_cast<float>(i) / length);
            v[i] = std::sin(theta);
        }

        auto [it, inserted] = tables_.emplace(id, std::make_unique<const Wavetable>(v));
        return it->second.get();
    }

    const Wavetable *WavetableStore::generateTriangle(std::string_view id, size_t length) {
        std::vector<float> v(length);

        for (size_t i = 0; i < length; ++i) {
            const float theta = static_cast<float>(i) / length;
            v[i] = 1.0f - 4.0f * std::abs(theta - 0.5f);
        }

        auto [it, inserted] = tables_.emplace(id, std::make_unique<const Wavetable>(v));
        return it->second.get();
    }

    std::optional<const Wavetable *> WavetableStore::find(std::string_view id) const {
        const auto it = tables_.find(static_cast<std::string>(id));

        if (it == tables_.end()) {
            return std::nullopt;
        }
        return std::make_optional(it->second.get());
    }
}
