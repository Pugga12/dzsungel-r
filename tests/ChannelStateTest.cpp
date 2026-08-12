// Tests for dzsungel::core::ChannelStateStore
//
// Previously, ChannelStateStore::apply() had two bugs which have since
// been fixed and are now covered by regression tests below instead of
// being documented as known-broken behavior:
//   1. The switch over MidiMsgType was missing `break` after every case,
//      so e.g. a PitchBend message also clobbered expression/volume/pan/
//      packedProgId. Fixed by adding `break;` to each case.
//   2. The bounds check was `channel > 16`, but channels_ is a 16-element
//      array (valid indices 0-15), so channel == 16 slipped through as an
//      out-of-bounds array access. Fixed to `channel > 15`.

#include <gtest/gtest.h>

#include "core/ChannelState.hpp"

using dzsungel::core::ChannelState;
using dzsungel::core::ChannelStateStore;

namespace {
MidiMsg makeMsg(uint8_t channel, MidiMsgType type, uint8_t data1 = 0, uint8_t data2 = 0) {
    MidiMsg m{};
    m.sampleOffset = 0;
    m.channel = channel;
    m.type = type;
    m.data1 = data1;
    m.data2 = data2;
    return m;
}
} // namespace

// ---- basic acceptance / bounds ----------------------------------------

TEST(ChannelStateStoreTest, ApplyReturnsTrueForValidChannel) {
    ChannelStateStore store;
    EXPECT_TRUE(store.apply(makeMsg(0, MidiMsgType::NoteOn)));
    EXPECT_TRUE(store.apply(makeMsg(15, MidiMsgType::NoteOn))); // last valid index
}

TEST(ChannelStateStoreTest, ApplyRejectsChannelAtOrAboveSixteen) {
    ChannelStateStore store;
    // Regression test: channel == 16 used to slip past the old `> 16`
    // guard and index one past the end of the 16-element channels_ array.
    EXPECT_FALSE(store.apply(makeMsg(16, MidiMsgType::NoteOn)));
    EXPECT_FALSE(store.apply(makeMsg(17, MidiMsgType::NoteOn)));
    EXPECT_FALSE(store.apply(makeMsg(255, MidiMsgType::NoteOn)));
}

TEST(ChannelStateStoreTest, StateVersionIncrementsOnEveryAcceptedApply) {
    ChannelStateStore store;
    EXPECT_EQ(store.get(0).stateVersion, 0u);

    store.apply(makeMsg(0, MidiMsgType::NoteOn));
    EXPECT_EQ(store.get(0).stateVersion, 1u);

    store.apply(makeMsg(0, MidiMsgType::NoteOff));
    EXPECT_EQ(store.get(0).stateVersion, 2u);
}

TEST(ChannelStateStoreTest, StateVersionDoesNotIncrementOnRejectedApply) {
    ChannelStateStore store;
    store.apply(makeMsg(200, MidiMsgType::NoteOn));
    EXPECT_EQ(store.get(0).stateVersion, 0u);
}

TEST(ChannelStateStoreTest, ChannelsAreIndependent) {
    ChannelStateStore store;
    store.apply(makeMsg(1, MidiMsgType::ProgramChange, 42));

    EXPECT_EQ(store.get(1).packedProgId, 42u);
    EXPECT_EQ(store.get(2).packedProgId, 0u);
    EXPECT_EQ(store.get(2).stateVersion, 0u);
}

TEST(ChannelStateStoreTest, DefaultChannelStateMatchesMidiDefaults) {
    const ChannelStateStore store;
    const ChannelState& c = store.get(0);

    EXPECT_EQ(c.stateVersion, 0u);
    EXPECT_EQ(c.packedProgId, 0u);
    EXPECT_EQ(c.pitchBendRaw, 0);
    EXPECT_EQ(c.expression, 127);
    EXPECT_EQ(c.volume, 127);
    EXPECT_EQ(c.pan, 64);
}

// ---- NoteOn / NoteOff: the only two cases that actually `break` -------

TEST(ChannelStateStoreTest, NoteOnDoesNotModifyControllerState) {
    ChannelStateStore store;
    store.apply(makeMsg(0, MidiMsgType::NoteOn, 60, 100));

    const ChannelState& c = store.get(0);
    EXPECT_EQ(c.pitchBendRaw, 0);
    EXPECT_EQ(c.expression, 127);
    EXPECT_EQ(c.volume, 127);
    EXPECT_EQ(c.pan, 64);
    EXPECT_EQ(c.packedProgId, 0u);
}

TEST(ChannelStateStoreTest, NoteOffDoesNotModifyControllerState) {
    ChannelStateStore store;
    store.apply(makeMsg(0, MidiMsgType::NoteOff, 60, 0));

    const ChannelState& c = store.get(0);
    EXPECT_EQ(c.pitchBendRaw, 0);
    EXPECT_EQ(c.expression, 127);
    EXPECT_EQ(c.volume, 127);
    EXPECT_EQ(c.pan, 64);
    EXPECT_EQ(c.packedProgId, 0u);
}

// ---- ProgramChange: the only controller case that ISN'T buggy ---------

TEST(ChannelStateStoreTest, ProgramChangeSetsOnlyPackedProgId) {
    ChannelStateStore store;
    store.apply(makeMsg(0, MidiMsgType::ProgramChange, 5));

    const ChannelState& c = store.get(0);
    EXPECT_EQ(c.packedProgId, 5u);
    EXPECT_EQ(c.pitchBendRaw, 0);
    EXPECT_EQ(c.expression, 127);
    EXPECT_EQ(c.volume, 127);
    EXPECT_EQ(c.pan, 64);
}

// ---- Case isolation regression tests ------------------------------------
//
// Each MidiMsgType case in the switch used to fall through into every case
// below it (missing `break`). These tests confirm each case now touches
// ONLY its own field and leaves every other field at its default.

TEST(ChannelStateStoreTest, CCPanTouchesOnlyPan) {
    ChannelStateStore store;
    store.apply(makeMsg(0, MidiMsgType::CCPan, 10));

    const ChannelState& c = store.get(0);
    EXPECT_EQ(c.pan, 10);
    EXPECT_EQ(c.packedProgId, 0u);
    EXPECT_EQ(c.pitchBendRaw, 0);
    EXPECT_EQ(c.expression, 127);
    EXPECT_EQ(c.volume, 127);
}

TEST(ChannelStateStoreTest, CCVolumeTouchesOnlyVolume) {
    ChannelStateStore store;
    store.apply(makeMsg(0, MidiMsgType::CCVolume, 20));

    const ChannelState& c = store.get(0);
    EXPECT_EQ(c.volume, 20);
    EXPECT_EQ(c.pan, 64);
    EXPECT_EQ(c.packedProgId, 0u);
    EXPECT_EQ(c.pitchBendRaw, 0);
    EXPECT_EQ(c.expression, 127);
}

TEST(ChannelStateStoreTest, CCExpressionTouchesOnlyExpression) {
    ChannelStateStore store;
    store.apply(makeMsg(0, MidiMsgType::CCExpression, 30));

    const ChannelState& c = store.get(0);
    EXPECT_EQ(c.expression, 30);
    EXPECT_EQ(c.volume, 127);
    EXPECT_EQ(c.pan, 64);
    EXPECT_EQ(c.packedProgId, 0u);
    EXPECT_EQ(c.pitchBendRaw, 0);
}

TEST(ChannelStateStoreTest, PitchBendTouchesOnlyPitchBendRaw) {
    ChannelStateStore store;
    // data1 = 0, data2 = 100 -> a clearly non-default, non-zero raw value,
    // chosen so a regression to the old fallthrough (which reused data1,
    // here 0) would be easy to tell apart from a genuine default.
    store.apply(makeMsg(0, MidiMsgType::PitchBend, 0, 100));

    const ChannelState& c = store.get(0);
    EXPECT_EQ(c.pitchBendRaw, 4608);
    EXPECT_EQ(c.expression, 127);
    EXPECT_EQ(c.volume, 127);
    EXPECT_EQ(c.pan, 64);
    EXPECT_EQ(c.packedProgId, 0u);
}

// ---- Pitch bend decoding ------------------------------------------------
//
// pitchBendToSInt() lives in ChannelState.cpp with external linkage but no
// declaration in the public header, so it's forward-declared here to test
// the 14-bit MIDI pitch-bend decode directly.
int16_t pitchBendToSInt(const MidiMsg& msg);

TEST(PitchBendToSIntTest, CenterValueDecodesToZero) {
    MidiMsg m{};
    m.data1 = 0;
    m.data2 = 64; // 14-bit center: (64 << 7) | 0 == 8192
    EXPECT_EQ(pitchBendToSInt(m), 0);
}

TEST(PitchBendToSIntTest, MinimumValueDecodesToMinusEightThousandOneNinetyTwo) {
    MidiMsg m{};
    m.data1 = 0;
    m.data2 = 0;
    EXPECT_EQ(pitchBendToSInt(m), -8192);
}

TEST(PitchBendToSIntTest, MaximumValueDecodesToPositiveMax) {
    MidiMsg m{};
    m.data1 = 127;
    m.data2 = 127;
    EXPECT_EQ(pitchBendToSInt(m), 8191);
}

TEST(ChannelStateStoreTest, PitchBendCenterAndExtremesDecodeThroughApply) {
    ChannelStateStore store;

    store.apply(makeMsg(0, MidiMsgType::PitchBend, 0, 0));
    EXPECT_EQ(store.get(0).pitchBendRaw, -8192);

    store.apply(makeMsg(0, MidiMsgType::PitchBend, 127, 127));
    EXPECT_EQ(store.get(0).pitchBendRaw, 8191);
}
