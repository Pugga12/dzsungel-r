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

#include "core/Oscillators.hpp"

namespace dzsungel::core::oscillators {
    float WavetableOsc::get() const {
        return at(phase_);
    }
    float WavetableOsc::at(float phase) const {
        return table_->data()[static_cast<size_t>(phase)];
    }
    void WavetableOsc::advance() {
        phase_ += phaseIncrement_;
        while (phase_ >= tableSize_) phase_ -= tableSize_;
    }
    void WavetableOsc::frequencySet(float frequency, float sampleRate) {
        phaseIncrement_ = (static_cast<float>(tableSize_) * frequency) / sampleRate;
    }
} // namespace dzsungel::core::oscillators
