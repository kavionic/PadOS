// pthread_rwlock_tests.cpp
// GoogleTest coverage for POSIX pthread read-write locks.
// Focus: correctness invariants, no fairness assumptions.
// Build with -pthread and link against your pthreads port.
//
// Tunables (override with -D…):
//   -DRWTEST_BASE_MS=20      // nominal short wait
//   -DRWTEST_TIMEOUT_MS=500  // "eventually" bound for blocked ops
//
// If your port supports timed rwlock ops, define:
//   -DHAVE_PTHREAD_RWLOCK_TIMED=1

#include <gtest/gtest.h>

#include <pthread.h>
#include <errno.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono;

#ifndef RWTEST_BASE_MS
#  define RWTEST_BASE_MS 20
#endif
#ifndef RWTEST_TIMEOUT_MS
#  define RWTEST_TIMEOUT_MS 500
#endif

static constexpr milliseconds kShort(RWTEST_BASE_MS);
static constexpr milliseconds kLong(RWTEST_BASE_MS * 5);
static constexpr milliseconds kTO(RWTEST_TIMEOUT_MS);

// Small start gate so threads begin attempts together.
struct StartGate {
    std::mutex m;
    std::condition_variable cv;
    std::atomic<int> ready{0};
    bool open = false;

    void arrived() {
        ready.fetch_add(1, std::memory_order_acq_rel);
    }
    void wait_open() {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return open; });
    }
    void wait_ready(int n) {
        auto deadline = steady_clock::now() + kTO;
        while (ready.load(std::memory_order_acquire) < n &&
            steady_clock::now() < deadline) {
            std::this_thread::sleep_for(milliseconds(1));
        }
    }
    void open_all() {
        std::lock_guard<std::mutex> g(m);
        open = true;
        cv.notify_all();
    }
};

// Helper: absolute CLOCK_REALTIME timespec for timed* APIs.
static timespec MakeAbsTimespec(milliseconds rel)
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto then = now + rel;
    auto sec = time_point_cast<seconds>(then);
    auto nsec = duration_cast<nanoseconds>(then - sec);
    timespec ts;
    ts.tv_sec = static_cast<time_t>(sec.time_since_epoch().count());
    ts.tv_nsec = static_cast<long>(nsec.count());
    return ts;
}

// ---------------------- Init / Destroy ----------------------
TEST(PthreadRwlock, InitDestroy)
{
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, nullptr), 0);
    EXPECT_EQ(pthread_rwlock_rdlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_wrlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}

#ifdef PTHREAD_RWLOCK_INITIALIZER
TEST(PthreadRwlock, StaticInitializer)
{
    pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;
    EXPECT_EQ(pthread_rwlock_rdlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}
#endif

TEST(PthreadRwlock, AttrInitDestroy)
{
    pthread_rwlockattr_t attr;
    ASSERT_EQ(pthread_rwlockattr_init(&attr), 0);
    // pshared default is implementation-defined; just ensure set/get compiles if present.
#if defined(PTHREAD_PROCESS_PRIVATE) && defined(PTHREAD_PROCESS_SHARED)
    int pshared = 0;
    EXPECT_EQ(pthread_rwlockattr_getpshared(&attr, &pshared), 0);
    EXPECT_TRUE(pshared == PTHREAD_PROCESS_PRIVATE || pshared == PTHREAD_PROCESS_SHARED);
    EXPECT_EQ(pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE), 0);
#endif
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, &attr), 0);
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
    EXPECT_EQ(pthread_rwlockattr_destroy(&attr), 0);
}

// ---------------------- Shared read access ----------------------
TEST(PthreadRwlock, MultipleReadersConcurrently)
{
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, nullptr), 0);

    constexpr int N = 4;
    StartGate gate;
    std::atomic<int> in_read{0};
    std::atomic<int> max_in_read{0};

    auto reader = [&](int) {
        gate.arrived();
        gate.wait_open();
        EXPECT_EQ(pthread_rwlock_rdlock(&rw), 0);
        int cur = in_read.fetch_add(1, std::memory_order_acq_rel) + 1;
        int prev = max_in_read.load(std::memory_order_acquire);
        while (cur > prev && !max_in_read.compare_exchange_weak(prev, cur)) { /* retry */ }
        std::this_thread::sleep_for(kShort);
        in_read.fetch_sub(1, std::memory_order_acq_rel);
        EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
    };

    std::vector<std::thread> ts;
    ts.reserve(N);
    for (int i = 0; i < N; ++i) ts.emplace_back(reader, i);
    gate.wait_ready(N);
    gate.open_all();
    for (auto& t : ts) t.join();

    EXPECT_GE(max_in_read.load(std::memory_order_acquire), 2); // at least 2 readers overlapped
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}

// ---------------------- Writer exclusivity ----------------------
TEST(PthreadRwlock, WriterBlocksReadersAndIsExclusive)
{
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, nullptr), 0);

    // Hold write lock.
    ASSERT_EQ(pthread_rwlock_wrlock(&rw), 0);

    std::atomic<bool> reader_entered{false};
    StartGate gate;

    std::thread r([&] noexcept {
        gate.arrived();
        gate.wait_open();
        // This should block until writer releases.
        EXPECT_EQ(pthread_rwlock_rdlock(&rw), 0);
        reader_entered.store(true, std::memory_order_release);
        EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
        });

    gate.wait_ready(1);
    gate.open_all();

    // Give the reader a chance to attempt.
    std::this_thread::sleep_for(kShort);

    // While writer holds, tryrdlock should fail with EBUSY.
    errno = 0;
    int rc = pthread_rwlock_tryrdlock(&rw);
    EXPECT_NE(rc, 0);
    EXPECT_EQ(rc, EBUSY);

    // Reader must not have entered yet.
    EXPECT_FALSE(reader_entered.load(std::memory_order_acquire));

    // Release writer; reader should proceed soon.
    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);

    auto deadline = steady_clock::now() + kTO;
    while (!reader_entered.load(std::memory_order_acquire) && steady_clock::now() < deadline) {
        std::this_thread::sleep_for(milliseconds(1));
    }
    EXPECT_TRUE(reader_entered.load(std::memory_order_acquire));

    r.join();
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}

// ---------------------- Trylock behavior ----------------------
TEST(PthreadRwlock, TryWrLockWhenReadHeldFails)
{
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, nullptr), 0);

    // Take a read lock first.
    ASSERT_EQ(pthread_rwlock_rdlock(&rw), 0);

    // Writer from another thread should fail trywrlock while read held.
    int rc = pthread_rwlock_trywrlock(&rw);
    EXPECT_NE(rc, 0);
    EXPECT_EQ(rc, EBUSY);

    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}

TEST(PthreadRwlock, TryWrLockWhileWriteHeldFailsAndNotRecursive)
{
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, nullptr), 0);
    ASSERT_EQ(pthread_rwlock_wrlock(&rw), 0);

    // Self-recursive write must not succeed (should be EDEADLK or EBUSY).
    int rc = pthread_rwlock_trywrlock(&rw);
    EXPECT_NE(rc, 0);
    // Accept either common outcome.
    EXPECT_TRUE(rc == EDEADLK || rc == EBUSY);

    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}

// Upgrading from RD->WR is not supported; trywrlock must not succeed.
TEST(PthreadRwlock, UpgradeFromReadTryWriteMustNotSucceed)
{
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, nullptr), 0);

    ASSERT_EQ(pthread_rwlock_rdlock(&rw), 0);
    int rc = pthread_rwlock_trywrlock(&rw);
    EXPECT_NE(rc, 0); // either EDEADLK or EBUSY, but never 0
    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);

    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}

// ---------------------- Timed lock behavior (optional) ----------------------
//#if defined(HAVE_PTHREAD_RWLOCK_TIMED) && HAVE_PTHREAD_RWLOCK_TIMED
TEST(PthreadRwlock, TimedRdLockTimesOutUnderWriter)
{
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, nullptr), 0);
    ASSERT_EQ(pthread_rwlock_wrlock(&rw), 0);

    timespec abst = MakeAbsTimespec(kShort);
    int rc = pthread_rwlock_timedrdlock(&rw, &abst);
    EXPECT_EQ(rc, ETIMEDOUT);

    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}

TEST(PthreadRwlock, TimedWrLockTimesOutUnderWriter)
{
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, nullptr), 0);
    ASSERT_EQ(pthread_rwlock_wrlock(&rw), 0);

    timespec abst = MakeAbsTimespec(kShort);
    int rc = pthread_rwlock_timedwrlock(&rw, &abst);
    EXPECT_EQ(rc, ETIMEDOUT);

    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}
//#endif

// ---------------------- Readers don't enter while writer holds ----------------------
TEST(PthreadRwlock, ReadersBlockedWhileWriterHolds)
{
    pthread_rwlock_t rw;
    ASSERT_EQ(pthread_rwlock_init(&rw, nullptr), 0);
    ASSERT_EQ(pthread_rwlock_wrlock(&rw), 0);

    StartGate gate;
    std::atomic<bool> any_reader_entered{false};
    constexpr int N = 3;

    auto reader = [&](int) {
        gate.arrived();
        gate.wait_open();
        // Block here until writer releases.
        int rc = pthread_rwlock_rdlock(&rw);
        if (rc == 0) {
            any_reader_entered.store(true, std::memory_order_release);
            pthread_rwlock_unlock(&rw);
        }
    };

    std::vector<std::thread> ts;
    for (int i = 0; i < N; ++i) ts.emplace_back(reader, i);
    gate.wait_ready(N);
    gate.open_all();

    std::this_thread::sleep_for(kShort);
    EXPECT_FALSE(any_reader_entered.load(std::memory_order_acquire));

    EXPECT_EQ(pthread_rwlock_unlock(&rw), 0);
    for (auto& t : ts) t.join();
    EXPECT_TRUE(any_reader_entered.load(std::memory_order_acquire));
    EXPECT_EQ(pthread_rwlock_destroy(&rw), 0);
}
