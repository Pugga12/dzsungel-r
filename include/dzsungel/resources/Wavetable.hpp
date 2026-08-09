#pragma once
#include <utility>
#include <vector>
#include <span>
namespace dzsungel::resources {
    class Wavetable {
    public:
        explicit Wavetable(std::vector<float> samples) : samples_(std::move(samples)) {};

        std::span<const float> data() const noexcept {return samples_; };

    private:
        std::vector<float> samples_;
    };
}