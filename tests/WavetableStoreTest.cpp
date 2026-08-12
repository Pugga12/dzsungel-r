// Tests for dzsungel::resources::Wavetable / WavetableStore
#include <gtest/gtest.h>

#include "resources/WavetableStore.hpp"

using dzsungel::resources::Wavetable;
using dzsungel::resources::WavetableStore;

namespace {
constexpr float kEps = 1e-4f;
}

// ---- Wavetable -------------------------------------------------------

TEST(WavetableTest, DataAccessorExposesConstructorSamples) {
    const Wavetable wt(std::vector<float>{1.0f, -1.0f, 0.5f});
    auto span = wt.data();

    ASSERT_EQ(span.size(), 3u);
    EXPECT_FLOAT_EQ(span[0], 1.0f);
    EXPECT_FLOAT_EQ(span[1], -1.0f);
    EXPECT_FLOAT_EQ(span[2], 0.5f);
}

TEST(WavetableTest, EmptySamplesProduceEmptySpan) {
    const Wavetable wt(std::vector<float>{});
    EXPECT_TRUE(wt.data().empty());
}

// ---- WavetableStore: construction -------------------------------------

TEST(WavetableStoreTest, DefaultConstructorSeedsDefaultSineAndTriangle) {
    const WavetableStore store;

    const auto sine = store.find("default-sin");
    const auto tri = store.find("default-tri");

    ASSERT_TRUE(sine.has_value());
    ASSERT_TRUE(tri.has_value());
    EXPECT_EQ((*sine)->data().size(), STD_LENGTH);
    EXPECT_EQ((*tri)->data().size(), STD_LENGTH);
}

TEST(WavetableStoreTest, FindReturnsNulloptForMissingId) {
    const WavetableStore store;
    EXPECT_FALSE(store.find("does-not-exist").has_value());
}

// ---- generateSine -------------------------------------------------------

TEST(WavetableStoreTest, GenerateSineHonorsRequestedLength) {
    WavetableStore store;
    const auto* wt = store.generateSine("s8", 8);
    EXPECT_EQ(wt->data().size(), 8u);
}

TEST(WavetableStoreTest, GenerateSineProducesExpectedSamples) {
    WavetableStore store;
    const auto* wt = store.generateSine("s8", 8);
    const auto data = wt->data();

    ASSERT_EQ(data.size(), 8u);
    EXPECT_NEAR(data[0], 0.0f, kEps);       // sin(0)
    EXPECT_NEAR(data[1], 0.70711f, kEps);   // sin(pi/4)
    EXPECT_NEAR(data[2], 1.0f, kEps);       // sin(pi/2), waveform peak
    EXPECT_NEAR(data[3], 0.70711f, kEps);
    EXPECT_NEAR(data[4], 0.0f, kEps);       // sin(pi)
    EXPECT_NEAR(data[5], -0.70711f, kEps);
    EXPECT_NEAR(data[6], -1.0f, kEps);      // sin(3pi/2), waveform trough
    EXPECT_NEAR(data[7], -0.70711f, kEps);
}

TEST(WavetableStoreTest, GenerateSineStaysWithinUnitRange) {
    WavetableStore store;
    const auto* wt = store.generateSine("s-range", 4096);
    for (const float sample : wt->data()) {
        EXPECT_LE(sample, 1.0f + kEps);
        EXPECT_GE(sample, -1.0f - kEps);
    }
}

TEST(WavetableStoreTest, GenerateSineReturnsSamePointerForRepeatedCalls) {
    WavetableStore store;
    const auto* wt = store.generateSine("s-same", 32);
    const auto* wt2 = store.generateSine("s-same", 32);
    EXPECT_EQ(wt, wt2);
}

// ---- generateTriangle ---------------------------------------------------

TEST(WavetableStoreTest, GenerateTriangleHonorsRequestedLength) {
    WavetableStore store;
    const auto* wt = store.generateTriangle("t8", 8);
    EXPECT_EQ(wt->data().size(), 8u);
}

TEST(WavetableStoreTest, GenerateTriangleProducesExpectedSamples) {
    WavetableStore store;
    const auto* wt = store.generateTriangle("t8", 8);
    const auto data = wt->data();

    // v[i] = 1 - 4*|i/length - 0.5|
    ASSERT_EQ(data.size(), 8u);
    EXPECT_NEAR(data[0], -1.0f, kEps);
    EXPECT_NEAR(data[1], -0.5f, kEps);
    EXPECT_NEAR(data[2], 0.0f, kEps);
    EXPECT_NEAR(data[3], 0.5f, kEps);
    EXPECT_NEAR(data[4], 1.0f, kEps);   // peak at the midpoint
    EXPECT_NEAR(data[5], 0.5f, kEps);
    EXPECT_NEAR(data[6], 0.0f, kEps);
    EXPECT_NEAR(data[7], -0.5f, kEps);
}

TEST(WavetableStoreTest, GenerateTriangleStaysWithinUnitRange) {
    WavetableStore store;
    const auto* wt = store.generateTriangle("t-range", 4096);
    for (const float sample : wt->data()) {
        EXPECT_LE(sample, 1.0f + kEps);
        EXPECT_GE(sample, -1.0f - kEps);
    }
}

// ---- id collisions --------------------------------------------------
//
// WavetableStore keys tables_ by id using unordered_map::emplace(), which is
// a no-op if the key already exists. Re-generating a wavetable under an id
// that is already taken silently keeps the FIRST table and ignores the new
// parameters. This test pins down that current (arguably surprising)
// behavior so a future change is a conscious decision, not a silent
// regression.
TEST(WavetableStoreTest, RegeneratingExistingIdKeepsOriginalTable) {
    WavetableStore store;
    const auto* first = store.generateSine("dup", 8);
    const auto* second = store.generateSine("dup", 16); // different length, same id

    EXPECT_EQ(first, second);
    EXPECT_EQ(second->data().size(), 8u); // NOT 16 - the second call was ignored
}

TEST(WavetableStoreTest, SineAndTriangleCanCoexistUnderDifferentIds) {
    WavetableStore store;
    store.generateSine("a", 16);
    store.generateTriangle("b", 16);

    EXPECT_TRUE(store.find("a").has_value());
    EXPECT_TRUE(store.find("b").has_value());
}
