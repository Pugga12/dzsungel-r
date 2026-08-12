// Tests for dzsungel::core::VoiceAllocator
//
// Previously, VoiceAllocator::bind() did `channelVoices_[channel].set(channel)`
// instead of `.set(id)`, so channel-scoped voice stealing only ever
// considered voice-slot index == channel as a stealing candidate rather
// than every voice actually owned by that channel. Fixed to `.set(id)`;
// see ChannelScopedStealingPicksTrueOldestVoiceOnThatChannel below.

#include <gtest/gtest.h>

#include "core/VoiceAllocator.hpp"

using dzsungel::core::VoiceAllocator;
using dzsungel::core::VoiceAllocResult;
using dzsungel::core::VoiceAllocStatus;

// ---- allocate(): fresh allocation --------------------------------------

TEST(VoiceAllocatorTest, FirstAllocationIsFreshAndUsesLowestFreeId) {
    VoiceAllocator va;
    const VoiceAllocResult r = va.allocate(0, 60, 100);

    EXPECT_EQ(r.status, VoiceAllocStatus::FRESH);
    EXPECT_EQ(r.voice, 0);
}

TEST(VoiceAllocatorTest, DistinctNotesGetDistinctFreshVoices) {
    VoiceAllocator va;
    const auto a = va.allocate(0, 60, 100);
    const auto b = va.allocate(0, 61, 101);

    EXPECT_EQ(a.status, VoiceAllocStatus::FRESH);
    EXPECT_EQ(b.status, VoiceAllocStatus::FRESH);
    EXPECT_NE(a.voice, b.voice);
}

TEST(VoiceAllocatorTest, SameNoteOnDifferentChannelsGetDifferentVoices) {
    VoiceAllocator va;
    const auto a = va.allocate(0, 60, 100);
    const auto b = va.allocate(1, 60, 100); // same pitch, different channel

    EXPECT_EQ(a.status, VoiceAllocStatus::FRESH);
    EXPECT_EQ(b.status, VoiceAllocStatus::FRESH);
    EXPECT_NE(a.voice, b.voice);
}

// ---- allocate(): duplicate re-trigger -----------------------------------

TEST(VoiceAllocatorTest, RetriggeringSameNoteReturnsDuplicateWithSameVoice) {
    VoiceAllocator va;
    const auto first = va.allocate(0, 60, 100);
    const auto second = va.allocate(0, 60, 200); // same channel+pitch

    EXPECT_EQ(second.status, VoiceAllocStatus::DUPLICATE);
    EXPECT_EQ(second.voice, first.voice);
}

// ---- allocate(): exhaustion fills every slot before stealing -----------

TEST(VoiceAllocatorTest, AllSixteenVoicesCanBeAllocatedFreshBeforeStealing) {
    VoiceAllocator va;
    for (uint8_t i = 0; i < kMaxVoices; ++i) {
        const auto r = va.allocate(0, i, 100 + i);
        EXPECT_EQ(r.status, VoiceAllocStatus::FRESH) << "note " << int(i);
        EXPECT_EQ(r.voice, i);
    }
}

TEST(VoiceAllocatorTest, AllocatingBeyondCapacityStealsRatherThanFails) {
    VoiceAllocator va;
    for (uint8_t i = 0; i < kMaxVoices; ++i) {
        va.allocate(0, i, 100 + i);
    }

    const auto overflow = va.allocate(0, 250, 900);
    EXPECT_NE(overflow.status, VoiceAllocStatus::FRESH);
    EXPECT_LT(overflow.voice, kMaxVoices);
}

// ---- release() ------------------------------------------------------

TEST(VoiceAllocatorTest, ReleasingUnboundNoteReturnsNotBound) {
    VoiceAllocator va;
    EXPECT_EQ(va.release(0, 60), VoiceAllocator::kNotBound);
}

TEST(VoiceAllocatorTest, ReleasingBoundNoteReturnsItsVoiceId) {
    VoiceAllocator va;
    const auto allocated = va.allocate(0, 60, 100);
    const int8_t releasedVoice = va.release(0, 60);
    EXPECT_EQ(releasedVoice, static_cast<int8_t>(allocated.voice));
}

TEST(VoiceAllocatorTest, ReleasingSameNoteTwiceReturnsNotBoundOnSecondCall) {
    VoiceAllocator va;
    va.allocate(0, 60, 100);
    va.release(0, 60);
    EXPECT_EQ(va.release(0, 60), VoiceAllocator::kNotBound);
}

TEST(VoiceAllocatorTest, ReleasedNoteCanBeRetriggeredAsFreshRatherThanDuplicate) {
    VoiceAllocator va;
    va.allocate(0, 60, 100);
    va.release(0, 60);

    // Since release() unbinds the note from noteToVoice_, retriggering the
    // exact same (channel, pitch) is no longer treated as a DUPLICATE.
    const auto retrigger = va.allocate(0, 60, 200);
    EXPECT_NE(retrigger.status, VoiceAllocStatus::DUPLICATE);
}

// ---- Releasing voices are preferred steal targets over active ones ----

TEST(VoiceAllocatorTest, ReleasingVoiceIsStolenBeforeActiveVoiceWhenFull) {
    VoiceAllocator va;
    for (uint8_t i = 0; i < kMaxVoices; ++i) {
        va.allocate(0, i, 100 + i);
    }

    const int8_t releasedVoice = va.release(0, 0); // voice bound to note 0
    ASSERT_GE(releasedVoice, 0);

    const auto stolen = va.allocate(0, 250, 999);
    EXPECT_EQ(stolen.status, VoiceAllocStatus::STOLEN_FROM_RELEASING);
    EXPECT_EQ(stolen.voice, static_cast<uint8_t>(releasedVoice));
}

// ---- notifyIdle() ----------------------------------------------------

TEST(VoiceAllocatorTest, NotifyIdleFreesSlotForFreshReuse) {
    VoiceAllocator va;
    const auto first = va.allocate(3, 40, 10);
    va.notifyIdle(first.voice);

    const auto second = va.allocate(3, 41, 20); // different pitch
    EXPECT_EQ(second.status, VoiceAllocStatus::FRESH);
    EXPECT_EQ(second.voice, first.voice);
}

TEST(VoiceAllocatorTest, NotifyIdleUnbindsNoteSoItsNoLongerADuplicate) {
    VoiceAllocator va;
    const auto first = va.allocate(3, 40, 10);
    va.notifyIdle(first.voice);

    const auto retrigger = va.allocate(3, 40, 20); // same channel+pitch again
    EXPECT_NE(retrigger.status, VoiceAllocStatus::DUPLICATE);
}

// ---- Channel-scoped stealing correctness --------------------------------

TEST(VoiceAllocatorTest, ChannelScopedStealingPicksTrueOldestVoiceOnThatChannel) {
    VoiceAllocator va;

    // Fill all 16 voices on channel 0, giving voice id 0 the NEWEST trigger
    // time and voice id 15 the OLDEST. Channel-scoped stealing should steal
    // the true oldest voice on channel 0, which is voice 15.
    for (uint8_t i = 0; i < kMaxVoices; ++i) {
        const auto r = va.allocate(0, i, 1000 - i);
        ASSERT_EQ(r.voice, i);
    }

    const auto stolen = va.allocate(0, 250, 5000);

    EXPECT_EQ(stolen.status, VoiceAllocStatus::STOLEN_FROM_ACTIVE);
    EXPECT_EQ(stolen.voice, 15);
}

TEST(VoiceAllocatorTest, ChannelScopedStealingDoesNotTouchOtherChannelsVoices) {
    VoiceAllocator va;

    // Channel 0 gets 8 voices, all triggered earlier than channel 1's 8
    // voices, so a *global* search would prefer stealing from channel 0.
    // But since channel 1 is the one that's actually full and channel 0
    // still has free slots elsewhere in the allocator... (both channels
    // combined fill all 16 slots) -- verify channel-scoped stealing for
    // channel 1 only ever steals a voice that belongs to channel 1.
    for (uint8_t i = 0; i < 8; ++i) va.allocate(0, i, 100 + i); // older
    for (uint8_t i = 0; i < 8; ++i) va.allocate(1, i, 900 + i); // newer

    // All 16 voices are now used. Allocating a new note on channel 1 must
    // steal from channel 1 specifically (voices 8-15), not the globally
    // older voices on channel 0 (voices 0-7).
    const auto stolen = va.allocate(1, 250, 5000);
    EXPECT_GE(stolen.voice, 8);
    EXPECT_LT(stolen.voice, 16);
}

TEST(VoiceAllocatorTest, GlobalStealingPicksOldestActiveVoiceAcrossChannels) {
    VoiceAllocator va;

    // Spread 16 notes across two channels so the (buggy) per-channel
    // bitset only ever tags voice-slot 0 for channel 0 and voice-slot 1
    // for channel 1. A THIRD, previously-unused channel has no tagged
    // voices at all, so findVictim always falls back to the untagged
    // global search, which correctly scans every voice regardless of
    // channel.
    for (uint8_t i = 0; i < 8; ++i) va.allocate(0, i, 100 + i);
    for (uint8_t i = 0; i < 8; ++i) va.allocate(1, i, 200 + i);

    // Channel 2 has zero voices bound, so its channel-scoped search always
    // comes up empty and allocate() falls back to the global (unscoped)
    // search, which is not affected by the bitset bug.
    const auto stolen = va.allocate(2, 77, 9999);
    EXPECT_NE(stolen.status, VoiceAllocStatus::FRESH);
    // The oldest voice overall is voice 0 (channel 0, note 0, time 100).
    EXPECT_EQ(stolen.voice, 0);
}
