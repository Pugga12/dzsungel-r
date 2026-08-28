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

template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

template<Numeric T>
class RampingValue {
    T base_;
    T current_;
    T target_;
    T increment_;
    uint32_t stepsBase_ = 0;
    uint32_t stepsRemaining_ = 0;
public:
    RampingValue(T base, T target, uint32_t steps) :
        base_(base),
        current_(base),
        target_(target),
        increment_((target - base) / steps),
        stepsBase_(steps),
        stepsRemaining_(steps) {}

    RampingValue() = default;

    [[nodiscard]] bool isFinished() const {
        return stepsRemaining_ == 0;
    }

    void reset(T base, T target, uint32_t steps) {
        base_ = base;
        current_ = base;
        target_ = target;
        increment_ = (target - base) / steps;
        stepsBase_ = steps;
        stepsRemaining_ = steps;
    }
    void reset(T base, T target) {
        base_ = base;
        current_ = base;
        target_ = target;
        increment_ = (target - base) / stepsBase_;
        stepsRemaining_ = stepsBase_;
    }
    T next() {
        if (stepsRemaining_ == 0) {
            return target_;
        } else if (stepsRemaining_ == stepsBase_) {
            return base_;
        }
        stepsRemaining_--;
        current_ += increment_;
        return current_;
    };
};

