#pragma once
#include "Wavetable.hpp"
#include <cstddef>
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