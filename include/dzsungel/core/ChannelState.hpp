//
// Created by adama on 8/9/26.
//
#pragma once
#include <array>
#include <cstdint>
#include "Types.hpp"

namespace dzsungel::core {
    struct ChannelState {
        uint32_t stateVersion = 0;
        uint32_t packedProgId = 0;

        int16_t pitchBendRaw = 0;
        uint8_t expression = 127;
        uint8_t volume = 127;
        uint8_t pan = 64;
    };

    class ChannelStateStore {
    public:
        const ChannelState& get(const uint8_t id) const { return channels_[id];}
        bool apply(const MidiMsg& msg);
    private:
        std::array<ChannelState, 16> channels_;
    };
}