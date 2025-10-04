// std_chrono_clocks_tests.cpp
// Comprehensive on-device tests for <chrono> clocks + timing behavior.
// - Exercises system_clock, steady_clock, high_resolution_clock always.
// - If your lib has C++20 chrono (calendars/clocks), also tests utc/tai/gps/file_clock via clock_cast.
// - Bounded waits only; no unbounded blocking.
//
// Build-time knobs (override with -D…):
//   -DCHRONO_BASE_MS=20         // nominal short wait
//   -DCHRONO_SLACK_MS=200       // tolerated overshoot on sleeps/timeouts
//   -DCHRONO_RESOLUTION_PROBES=128 // resolution sampling iterations

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <type_traits>
#include <ratio>
#include <ctime>
#include <mutex>
#include <condition_variable>
#include <System/TimeValue.h>
#include <System/SysTime.h>

using namespace std::chrono;
// ---------- Tunables ----------
#ifndef CHRONO_BASE_MS
#define CHRONO_BASE_MS 20
#endif
#ifndef CHRONO_SLACK_MS
#define CHRONO_SLACK_MS 200
#endif
#ifndef CHRONO_RESOLUTION_PROBES
#define CHRONO_RESOLUTION_PROBES 128
#endif

static constexpr milliseconds kShort(CHRONO_BASE_MS);
static constexpr milliseconds kLong(CHRONO_BASE_MS * 5);
static constexpr milliseconds kSlack(CHRONO_SLACK_MS);

// Helper: absolute difference for durations/time_points
template<class T>
static auto AbsDiff(const T& a, const T& b) -> decltype(a - b) {
    auto d = a - b;
    return d < typename decltype(d)::zero() ? -d : d;
}

// Helper: measure a rough lower bound for steady_clock resolution.
static nanoseconds MeasureSteadyResolution() {
    nanoseconds best = nanoseconds::max();
    for (int i = 0; i < CHRONO_RESOLUTION_PROBES; ++i) {
        auto t0 = steady_clock::now();
        auto t1 = t0;
        while ((t1 = steady_clock::now()) == t0) { /* spin very briefly */ }
        auto diff = t1 - t0;
        if (diff < best) best = diff;
    }
    return best;
}

// ---------- Duration / rounding basics ----------
TEST(ChronoDurations, CastsAndRounding)
{
    static_assert(std::ratio_equal<milliseconds::period, std::milli>::value, "ms period");
    EXPECT_EQ(duration_cast<milliseconds>(3s).count(), 3000);
    EXPECT_EQ(duration_cast<seconds>(1999ms).count(), 1); // truncation
    EXPECT_EQ(floor<seconds>(1499ms), 1s);
    EXPECT_EQ(ceil <seconds>(1500ms), 2s);
    EXPECT_EQ(round<seconds>(1499ms), 1s);
    EXPECT_EQ(round<seconds>(1501ms), 2s);
}

// ---------- system_clock ----------
TEST(ChronoSystemClock, TimeTRoundTripWithinOneSecond)
{
    auto now = system_clock::now();
    std::time_t tt = system_clock::to_time_t(now);
    auto back = system_clock::from_time_t(tt);
    auto diff = now - back;
    EXPECT_GE(diff, 0s);
    EXPECT_LT(diff, 1s + kSlack);
}

TEST(ChronoSystemClock, SleepUntilReachesTarget)
{
    // system_clock is not required to be steady but should respect sleep_until deadlines.
    auto target = system_clock::now() + kShort;
    std::this_thread::sleep_until(target);
    auto now = system_clock::now();
    EXPECT_GE(now, target);
    EXPECT_LT(now - target, kSlack);
}

// ---------- steady_clock ----------
TEST(ChronoSteadyClock, IsSteadyAndMonotonic)
{
    EXPECT_TRUE(steady_clock::is_steady);
    auto prev = steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        auto cur = steady_clock::now();
        EXPECT_LE(prev, cur);
        prev = cur;
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(ChronoSteadyClock, ResolutionReasonable)
{
    auto res = MeasureSteadyResolution();
    EXPECT_GT(res, nanoseconds::zero());
    EXPECT_LT(res, seconds(1));
}

TEST(ChronoSteadyClock, SleepForAtLeastRequested)
{
    auto t0 = steady_clock::now();
    std::this_thread::sleep_for(kShort);
    auto elapsed = steady_clock::now() - t0;
    EXPECT_GE(elapsed, kShort);
    EXPECT_LT(elapsed, kShort + kSlack);
}

TEST(ChronoSteadyClock, SleepUntilAtLeastTarget)
{
    auto target = steady_clock::now() + kShort;
    std::this_thread::sleep_until(target);
    auto now = steady_clock::now();
    EXPECT_GE(now, target);
    EXPECT_LT(now - target, kSlack);
}

// ---------- high_resolution_clock ----------
TEST(ChronoHighResClock, NowAndNonDecreasingInTightLoop)
{
    // high_resolution_clock may alias steady_clock or system_clock.
    auto a = high_resolution_clock::now();
    auto b = high_resolution_clock::now();
    EXPECT_LE(a, b);
}

// ---------- Condition variable timeouts with BOTH clocks ----------
TEST(ChronoCV, WaitForTimesOutNearTarget)
{
    std::mutex m; std::condition_variable cv;
    std::unique_lock<std::mutex> lk(m);
    auto t0 = steady_clock::now();
    auto status = cv.wait_for(lk, kShort);
    auto elapsed = steady_clock::now() - t0;
    EXPECT_EQ(status, std::cv_status::timeout);
    EXPECT_GE(elapsed, kShort);
    EXPECT_LT(elapsed, kShort + kSlack);
}

TEST(ChronoCV, WaitUntilSystemClockTimesOutNearTarget)
{
    std::mutex m; std::condition_variable cv;
    std::unique_lock<std::mutex> lk(m);
    auto target = system_clock::now() + kShort;
    auto status = cv.wait_until(lk, target);
    auto now = system_clock::now();
    EXPECT_EQ(status, std::cv_status::timeout);
    EXPECT_GE(now, target);
    EXPECT_LT(now - target, kSlack);
}

// ---------- time_point arithmetic across durations ----------
TEST(ChronoTimePoint, MixedDurationMath)
{
    time_point<steady_clock, milliseconds> t_ms(1000ms);
    time_point<steady_clock, microseconds> t_us = t_ms + 250ms;
    auto diff = t_us - t_ms; // microseconds
    EXPECT_EQ(diff, 250ms);
}

// ---------- C++20 chrono: extra clocks & clock_cast ----------

TEST(ChronoCxx20Clocks, AliasesCompile)
{
    // Compile-time checks for alias types
    static_assert(std::is_same_v<sys_time<nanoseconds>, time_point<system_clock, nanoseconds>>);
    static_assert(std::is_same_v<utc_time<nanoseconds>, time_point<utc_clock, nanoseconds>>);
    static_assert(std::is_same_v<tai_time<nanoseconds>, time_point<tai_clock, nanoseconds>>);
    static_assert(std::is_same_v<gps_time<nanoseconds>, time_point<gps_clock, nanoseconds>>);
    static_assert(std::is_same_v<file_time<nanoseconds>, time_point<file_clock, nanoseconds>>);
}

TEST(ChronoCxx20Clocks, SysUtcRoundTripExact)
{
    auto sys_now = time_point_cast<nanoseconds>(system_clock::now());
    auto utc_tp = clock_cast<utc_clock>(sys_now);
    auto sys_back = clock_cast<system_clock>(utc_tp);
    EXPECT_EQ(sys_back, sys_now); // round-trip should be exact for representable instants
}

TEST(ChronoCxx20Clocks, UtcTaiGpsRoundTrips)
{
    auto utc_now = clock_cast<utc_clock>(time_point_cast<nanoseconds>(system_clock::now()));
    auto tai_back = clock_cast<tai_clock>(utc_now);
    auto utc_back = clock_cast<utc_clock>(tai_back);
    EXPECT_EQ(utc_back, utc_now);

    auto gps_back = clock_cast<gps_clock>(utc_now);
    auto utc_back2 = clock_cast<utc_clock>(gps_back);
    EXPECT_EQ(utc_back2, utc_now);
}


TEST(ChronoCxx20Clocks, FileClockRoundTripWithResolution)
{
    // file_clock can have coarser resolution; compare at file_clock::duration granularity.
    auto sys_now = time_point_cast<nanoseconds>(system_clock::now());
    auto file_tp = clock_cast<file_clock>(sys_now);
    auto sys_back = clock_cast<system_clock>(file_tp);

    // Round both to file_clock's duration and compare
    auto sys_now_rounded = time_point_cast<file_clock::duration>(sys_now);
    auto sys_back_rounded = time_point_cast<file_clock::duration>(sys_back);
    EXPECT_EQ(sys_back_rounded.time_since_epoch(), sys_now_rounded.time_since_epoch());
}

TEST(ChronoCxx20Clocks, CompareClocks)
{
    auto nativeSteady = get_monotonic_clock();
    auto nativeSteadyHires = get_monotonic_clock_hires();
    auto chronoSteady = steady_clock::now();

    EXPECT_TRUE(std::chrono::abs(nativeSteady.time_since_epoch() - nativeSteadyHires.time_since_epoch()) < milliseconds(10));
    EXPECT_TRUE(std::chrono::abs(nativeSteady.time_since_epoch() - chronoSteady.time_since_epoch()) < milliseconds(10));
}

TEST(ChronoCxx20Clocks, NativeSteadyClock)
{
    bool sawNoStep = false;
    auto prevTime = get_real_time();
    for (int i = 0; i < 10; ++i)
    {
        const auto curTime = get_real_time();
        if (curTime == prevTime) sawNoStep = true;
        prevTime = curTime;
    }
    for (int i = 0; i < 100000; ++i)
    {
        get_real_time();
    }
    EXPECT_TRUE(sawNoStep);
}

TEST(ChronoCxx20Clocks, NativeHighresSteadyClock)
{
    bool sawNoStep = false;
    auto prevTime = get_real_time_hires();
    for (int i = 0; i < 10; ++i)
    {
        const auto curTime = get_real_time_hires();
        if (curTime == prevTime) sawNoStep = true;
        prevTime = curTime;
    }
    for (int i = 0; i < 100000; ++i)
    {
        get_real_time_hires();
    }
    EXPECT_FALSE(sawNoStep);
}

TEST(ChronoCxx20Clocks, ChronoHighresSteadyClock)
{
    bool sawNoStep = false;
    auto prevTime = high_resolution_clock::now();
    for (int i = 0; i < 10; ++i)
    {
        const auto curTime = high_resolution_clock::now();
        if (curTime == prevTime) sawNoStep = true;
        prevTime = curTime;
    }
    for (int i = 0; i < 100000; ++i)
    {
        high_resolution_clock::now();
    }
    EXPECT_FALSE(sawNoStep);
}
