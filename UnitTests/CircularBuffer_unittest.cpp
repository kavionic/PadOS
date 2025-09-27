#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <random>
#include <Utils/CircularBuffer.h>

// --- helper: size tag so we can do typed tests over sizes ---
template <size_t N>
struct SizeTag { static constexpr size_t value = N; };

// The fixture takes a *type* parameter, whose ::value is the queue size.
template <class SizeT>
class CircularBufferTest : public ::testing::Test {
protected:
    static constexpr size_t N = SizeT::value;
    CircularBuffer<uint8_t, N> buf;
};

using TestSizes = ::testing::Types<SizeTag<4>, SizeTag<8>, SizeTag<16>, SizeTag<64>>;
TYPED_TEST_SUITE(CircularBufferTest, TestSizes);

static void FillPattern(uint8_t* buf, size_t count, uint8_t seed) {
    for (size_t i = 0; i < count; ++i) buf[i] = static_cast<uint8_t>(seed + i);
}

// Initially empty
TYPED_TEST(CircularBufferTest, InitiallyEmpty) {
    EXPECT_TRUE(this->buf.IsEmpty());
    EXPECT_EQ(this->buf.GetLength(), 0u);
}

// Simple write/read (no wrap)
TYPED_TEST(CircularBufferTest, WriteAndReadSimple) {
    uint8_t in[3]; FillPattern(in, 3, 0x10);
    EXPECT_EQ(this->buf.Write(in, 3), 3u);
    EXPECT_EQ(this->buf.GetLength(), 3u);

    uint8_t out[3] = {};
    EXPECT_EQ(this->buf.Read(out, 3), 3u);
    EXPECT_EQ(this->buf.GetLength(), 0u);
    EXPECT_EQ(0, std::memcmp(in, out, 3));
}

// Fill exactly N and drain
TYPED_TEST(CircularBufferTest, FillAndDrain) {
    constexpr size_t N = TestFixture::N;
    uint8_t in[N]; FillPattern(in, N, 0x20);

    EXPECT_EQ(this->buf.Write(in, N), N);
    EXPECT_EQ(this->buf.GetLength(), N);

    uint8_t out[N] = {};
    EXPECT_EQ(this->buf.Read(out, N), N);
    EXPECT_EQ(this->buf.GetLength(), 0u);
    EXPECT_EQ(0, std::memcmp(in, out, N));
}

// Force a wrap on write, then read back
TYPED_TEST(CircularBufferTest, WrapWriteAndRead) {
    constexpr size_t N = TestFixture::N;

    // Prime positions so in%N == N-2 and out%N == N-2 (empty but in near end)
    uint8_t prime[N - 2]; FillPattern(prime, N - 2, 0x30);
    ASSERT_EQ(this->buf.Write(prime, N - 2), N - 2);
    uint8_t dump[N - 2] = {};
    ASSERT_EQ(this->buf.Read(dump, N - 2), N - 2);
    ASSERT_TRUE(this->buf.IsEmpty());

    // Now write 4 bytes -> guaranteed to wrap when N>=4 (our sizes are)
    uint8_t in[4]; FillPattern(in, 4, 0x40);
    EXPECT_EQ(this->buf.Write(in, 4), 4u);

    uint8_t out[4] = {};
    EXPECT_EQ(this->buf.Read(out, 4), 4u);
    EXPECT_EQ(0, std::memcmp(in, out, 4));
}

// Overflow: write 2N, expect last N retained
TYPED_TEST(CircularBufferTest, DropOldestOnOverflow) {
    constexpr size_t N = TestFixture::N;
    uint8_t in[2 * N]; FillPattern(in, 2 * N, 0x50);

    EXPECT_EQ(this->buf.Write(in, 2 * N), N);  // expect to report bytes actually retained
    EXPECT_EQ(this->buf.GetLength(), N);

    uint8_t out[N] = {};
    EXPECT_EQ(this->buf.Read(out, N), N);
    EXPECT_EQ(0, std::memcmp(in + N, out, N));
}

// Many small interleaved
