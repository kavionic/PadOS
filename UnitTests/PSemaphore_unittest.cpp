// posix_semaphore_tests.cpp (fixed)
// Coverage: unnamed + named semaphores, sem_timedwait, sem_clockwait,
// and two stress tests. No clock-jump tests.
// Build: link with -pthread (or your port equivalent).

#include <gtest/gtest.h>

#include <semaphore.h>
#include <errno.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <time.h>     // clock_gettime, clockid_t
#include <fcntl.h>    // O_CREAT, O_EXCL
#include <sys/unistd.h>
#include <sys/stat.h> // mode_t

using namespace std::chrono;

#ifndef SEMTEST_BASE_MS
#  define SEMTEST_BASE_MS 20
#endif
#ifndef SEMTEST_TIMEOUT_MS
#  define SEMTEST_TIMEOUT_MS 1000
#endif

static constexpr milliseconds kShort(SEMTEST_BASE_MS);
static constexpr milliseconds kTO(SEMTEST_TIMEOUT_MS);

// -------- helpers --------
static timespec MakeAbsTimespecRealtime(milliseconds rel) noexcept {
    auto now = system_clock::now();
    auto then = now + rel;
    auto sec = time_point_cast<seconds>(then);
    auto nsec = duration_cast<nanoseconds>(then - sec);
    timespec ts;
    ts.tv_sec = static_cast<time_t>(sec.time_since_epoch().count());
    ts.tv_nsec = static_cast<long>(nsec.count());
    return ts;
}

// Returns true on success and writes *out; false if clock_gettime fails.
static bool MakeAbsTimespecForClock(clockid_t clk, milliseconds rel, timespec* out) noexcept {
    constexpr long NS_PER_S = 1000000000L;
    timespec now{};
    if (clock_gettime(clk, &now) != 0) return false;
    long add_ns = static_cast<long>(rel.count()) * 1000000L;
    timespec ts = now;
    ts.tv_sec += add_ns / NS_PER_S;
    ts.tv_nsec += add_ns % NS_PER_S;
    if (ts.tv_nsec >= NS_PER_S) { ++ts.tv_sec; ts.tv_nsec -= NS_PER_S; }
    *out = ts;
    return true;
}

// Robust wait wrappers that handle EINTR.
static int TimedWait(sem_t* s, const timespec* abs) noexcept {
    for (;;) {
        if (sem_timedwait(s, abs) == 0) return 0;
        if (errno == EINTR) continue;
        return -1;
    }
}
static int ClockWait(sem_t* s, clockid_t clk, const timespec* abs) noexcept {
    for (;;) {
        if (sem_clockwait(s, clk, abs) == 0) return 0;
        if (errno == EINTR) continue;
        return -1;
    }
}

struct StartGate {
    std::atomic<int>  ready{0};
    std::atomic<bool> open{false};
    void arrived() noexcept { ready.fetch_add(1, std::memory_order_acq_rel); }
    void wait_ready(int n) const noexcept {
        auto deadline = steady_clock::now() + kTO;
        while (ready.load(std::memory_order_acquire) < n &&
            steady_clock::now() < deadline) {
            std::this_thread::sleep_for(milliseconds(1));
        }
    }
    void open_all() noexcept { open.store(true, std::memory_order_release); }
    void wait_open() const noexcept {
        auto deadline = steady_clock::now() + kTO;
        while (!open.load(std::memory_order_acquire) &&
            steady_clock::now() < deadline) {
            std::this_thread::sleep_for(milliseconds(1));
        }
    }
};

// =======================================================
// Unnamed semaphore tests
// =======================================================

TEST(PosixSem, InitDestroyBasic) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, /*pshared=*/0, /*value=*/0), 0);
    EXPECT_EQ(sem_post(&s), 0);
    timespec ts = MakeAbsTimespecRealtime(kShort);
    errno = 0;
    EXPECT_EQ(TimedWait(&s, &ts), 0) << "errno=" << errno;
    EXPECT_EQ(sem_destroy(&s), 0);
}

TEST(PosixSem, TryWaitWhenZeroGivesEAGAIN) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);
    errno = 0;
    EXPECT_EQ(sem_trywait(&s), -1);
    EXPECT_EQ(errno, EAGAIN);
    EXPECT_EQ(sem_destroy(&s), 0);
}

TEST(PosixSem, PostThenWaitSingleThread) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);

    EXPECT_EQ(sem_post(&s), 0);
    timespec ts = MakeAbsTimespecRealtime(kShort);
    EXPECT_EQ(TimedWait(&s, &ts), 0) << "errno=" << errno;

    int v = -1;
    EXPECT_EQ(sem_getvalue(&s, &v), 0);
    EXPECT_EQ(v, 0);

    EXPECT_EQ(sem_destroy(&s), 0);
}

TEST(PosixSem, GetValueSingleThreadSequence) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 3), 0);

    int v = -1;
    ASSERT_EQ(sem_getvalue(&s, &v), 0);
    EXPECT_EQ(v, 3);

    ASSERT_EQ(sem_wait(&s), 0);
    ASSERT_EQ(sem_getvalue(&s, &v), 0);
    EXPECT_EQ(v, 2);

    ASSERT_EQ(sem_post(&s), 0);
    ASSERT_EQ(sem_post(&s), 0);
    ASSERT_EQ(sem_getvalue(&s, &v), 0);
    EXPECT_EQ(v, 4);

    EXPECT_EQ(sem_destroy(&s), 0);
}

TEST(PosixSem, PostOverflowIfAtMax) {
#ifdef SEM_VALUE_MAX
    sem_t s;
    int rc = sem_init(&s, 0, SEM_VALUE_MAX);
    if (rc != 0) {
        GTEST_SKIP() << "sem_init rejected SEM_VALUE_MAX (EINVAL likely).";
    }
    else {
        errno = 0;
        EXPECT_EQ(sem_post(&s), -1);
        EXPECT_EQ(errno, EOVERFLOW);
        EXPECT_EQ(sem_destroy(&s), 0);
    }
#else
    GTEST_SKIP() << "SEM_VALUE_MAX not defined.";
#endif
}

TEST(PosixSem, PostWakesExactlyOneWaiter) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);

    std::atomic<bool> w1_started{false}, w2_started{ false };
    std::atomic<bool> w1_done{false}, w2_done{ false };

    auto waiter = [&](std::atomic<bool>& started, std::atomic<bool>& done) noexcept {
        started.store(true, std::memory_order_release);
        timespec ts = MakeAbsTimespecRealtime(kTO);
        if (TimedWait(&s, &ts) == 0) {
            done.store(true, std::memory_order_release);
        }
    };

    std::thread t1(waiter, std::ref(w1_started), std::ref(w1_done));
    std::thread t2(waiter, std::ref(w2_started), std::ref(w2_done));

    auto deadline = steady_clock::now() + kTO;
    while (!(w1_started.load(std::memory_order_acquire) &&
        w2_started.load(std::memory_order_acquire)) &&
        steady_clock::now() < deadline) {
        std::this_thread::sleep_for(milliseconds(1));
    }
    ASSERT_TRUE(w1_started.load() && w2_started.load());

    ASSERT_EQ(sem_post(&s), 0);

    auto one_deadline = steady_clock::now() + kShort;
    while (!w1_done.load(std::memory_order_acquire) &&
        !w2_done.load(std::memory_order_acquire) &&
        steady_clock::now() < one_deadline) {
        std::this_thread::sleep_for(milliseconds(1));
    }
    bool one = w1_done.load(std::memory_order_acquire) ^ w2_done.load(std::memory_order_acquire);
    EXPECT_TRUE(one) << "Exactly one waiter should proceed after single post";

    ASSERT_EQ(sem_post(&s), 0);

    deadline = steady_clock::now() + kTO;
    while (!(w1_done.load(std::memory_order_acquire) &&
        w2_done.load(std::memory_order_acquire)) &&
        steady_clock::now() < deadline) {
        std::this_thread::sleep_for(milliseconds(1));
    }
    EXPECT_TRUE(w1_done.load() && w2_done.load());

    t1.join(); t2.join();
    EXPECT_EQ(sem_destroy(&s), 0);
}

TEST(PosixSem, TimedWaitTimesOutWhenZero) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);
    timespec ts = MakeAbsTimespecRealtime(kShort);
    errno = 0;
    EXPECT_EQ(TimedWait(&s, &ts), -1);
    EXPECT_EQ(errno, ETIMEDOUT);
    EXPECT_EQ(sem_destroy(&s), 0);
}

// =======================================================
// sem_clockwait() tests (no clock jumping)
// =======================================================

TEST(PosixSem, ClockWaitRealtimeTimesOutWhenZero) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);
    timespec abs{};
    ASSERT_TRUE(MakeAbsTimespecForClock(CLOCK_REALTIME, kShort, &abs)) << "clock_gettime failed";
    errno = 0;
    EXPECT_EQ(ClockWait(&s, CLOCK_REALTIME, &abs), -1);
    EXPECT_EQ(errno, ETIMEDOUT);
    EXPECT_EQ(sem_destroy(&s), 0);
}

TEST(PosixSem, ClockWaitMonotonicTimesOutWhenZero) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);
    timespec abs{};
    ASSERT_TRUE(MakeAbsTimespecForClock(CLOCK_MONOTONIC, kShort, &abs)) << "clock_gettime failed";
    errno = 0;
    EXPECT_EQ(ClockWait(&s, CLOCK_MONOTONIC, &abs), -1);
    EXPECT_EQ(errno, ETIMEDOUT);
    EXPECT_EQ(sem_destroy(&s), 0);
}

TEST(PosixSem, ClockWaitRealtimeWakesOnPost) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);

    std::atomic<bool> started{false};
    std::atomic<bool> done{false};

    std::thread w([&] noexcept {
        started.store(true, std::memory_order_release);
        timespec abs{};
        ASSERT_TRUE(MakeAbsTimespecForClock(CLOCK_REALTIME, kTO, &abs)) << "clock_gettime failed";
        ASSERT_EQ(ClockWait(&s, CLOCK_REALTIME, &abs), 0) << "errno=" << errno;
        done.store(true, std::memory_order_release);
        });

    auto deadline = steady_clock::now() + kTO;
    while (!started.load(std::memory_order_acquire) && steady_clock::now() < deadline)
        std::this_thread::sleep_for(milliseconds(1));

    ASSERT_EQ(sem_post(&s), 0);

    deadline = steady_clock::now() + kTO;
    while (!done.load(std::memory_order_acquire) && steady_clock::now() < deadline)
        std::this_thread::sleep_for(milliseconds(1));

    EXPECT_TRUE(done.load(std::memory_order_acquire));
    w.join();
    EXPECT_EQ(sem_destroy(&s), 0);
}

TEST(PosixSem, ClockWaitMonotonicWakesOnPost) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);

    std::atomic<bool> started{false};
    std::atomic<bool> done{false};

    std::thread w([&] noexcept {
        started.store(true, std::memory_order_release);
        timespec abs{};
        ASSERT_TRUE(MakeAbsTimespecForClock(CLOCK_MONOTONIC, kTO, &abs)) << "clock_gettime failed";
        ASSERT_EQ(ClockWait(&s, CLOCK_MONOTONIC, &abs), 0) << "errno=" << errno;
        done.store(true, std::memory_order_release);
        });

    auto deadline = steady_clock::now() + kTO;
    while (!started.load(std::memory_order_acquire) && steady_clock::now() < deadline)
        std::this_thread::sleep_for(milliseconds(1));

    ASSERT_EQ(sem_post(&s), 0);

    deadline = steady_clock::now() + kTO;
    while (!done.load(std::memory_order_acquire) && steady_clock::now() < deadline)
        std::this_thread::sleep_for(milliseconds(1));

    EXPECT_TRUE(done.load(std::memory_order_acquire));
    w.join();
    EXPECT_EQ(sem_destroy(&s), 0);
}

// =======================================================
// Named semaphore tests (unconditional)
// =======================================================

static std::string UniqueSemName() {
    static std::atomic<unsigned> ctr{0};
    unsigned id = ctr.fetch_add(1, std::memory_order_acq_rel);
    return "/gtest_sem_" + std::to_string(id); // POSIX requires leading '/'
}

TEST(PosixSemNamed, OpenExistingAndExclSemantics) {
    std::string name = UniqueSemName();
    sem_unlink(name.c_str()); // clean slate

    // First create: must succeed.
    sem_t* a = sem_open(name.c_str(), O_CREAT | O_EXCL, 0600, 0);
    ASSERT_NE(a, SEM_FAILED) << "errno=" << errno;

    // Re-open existing WITHOUT O_CREAT: must succeed (open existing).
    sem_t* h0 = sem_open(name.c_str(), 0);
    ASSERT_NE(h0, SEM_FAILED) << "errno=" << errno;

    // Re-open existing WITH O_CREAT but WITHOUT O_EXCL: must also succeed (open existing).
    sem_t* h1 = sem_open(name.c_str(), O_CREAT, 0600, 123 /*ignored*/);
    ASSERT_NE(h1, SEM_FAILED) << "errno=" << errno;

    // Attempt to create again WITH O_CREAT|O_EXCL: must fail with EEXIST.
    errno = 0;
    sem_t* dup = sem_open(name.c_str(), O_CREAT | O_EXCL, 0600, 0);
    EXPECT_EQ(dup, SEM_FAILED);
    EXPECT_EQ(errno, EEXIST);

    // Prove all successful handles refer to the *same* object.
    std::atomic<bool> done{false};
    std::thread w([&] noexcept {
        timespec ts = MakeAbsTimespecRealtime(kTO);
        ASSERT_EQ(TimedWait(h0, &ts), 0) << "errno=" << errno;
        done.store(true, std::memory_order_release);
        });

    ASSERT_EQ(sem_post(a), 0);
    auto deadline = std::chrono::steady_clock::now() + kTO;
    while (!done.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(kShort);
    EXPECT_TRUE(done.load());

    w.join();

    ASSERT_EQ(sem_close(h1), 0);
    ASSERT_EQ(sem_close(h0), 0);
    ASSERT_EQ(sem_close(a), 0);
    ASSERT_EQ(sem_unlink(name.c_str()), 0);
}

TEST(PosixSemNamed, RecreateAfterUnlinkCreatesFreshSemaphore) {
    std::string name = UniqueSemName();
    sem_unlink(name.c_str());

    sem_t* a = sem_open(name.c_str(), O_CREAT | O_EXCL, 0600, 0);
    ASSERT_NE(a, SEM_FAILED) << "errno=" << errno;
    ASSERT_EQ(sem_unlink(name.c_str()), 0); // remove name while 'a' is open

    // Create a new semaphore with the same name (distinct object).
    sem_t* b = sem_open(name.c_str(), O_CREAT | O_EXCL, 0600, 0);
    ASSERT_NE(b, SEM_FAILED) << "errno=" << errno;

    // Post on 'a' should NOT satisfy waits on 'b'.
    std::atomic<bool> b_done{false};
    std::thread w([&] noexcept {
        timespec ts = MakeAbsTimespecRealtime(kShort);
        int rc = TimedWait(b, &ts);
        if (rc == 0) b_done.store(true, std::memory_order_release);
        else if (errno != ETIMEDOUT) FAIL() << "unexpected errno=" << errno;
        });

    ASSERT_EQ(sem_post(a), 0);
    std::this_thread::sleep_for(kShort);
    EXPECT_FALSE(b_done.load(std::memory_order_acquire));

    w.join();
    EXPECT_EQ(sem_close(b), 0);
    EXPECT_EQ(sem_close(a), 0);
    // Name is still present for 'b'; unlink it now.
    EXPECT_EQ(sem_unlink(name.c_str()), 0);
}

// =======================================================
// Stress tests (no clock manipulation)
// =======================================================

TEST(PosixSem, StressMixedProducersConsumers) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);

    constexpr int PRODUCERS = 6;
    constexpr int CONSUMERS = 10;
    constexpr int TOKENS_PER_PRODUCER = 150;   // total 900
    constexpr milliseconds PRODUCER_PAUSE(1);
    constexpr milliseconds WAIT_SLICE(20);

    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    std::atomic<bool> start{false};

    auto lcg = [](uint32_t& x) noexcept -> uint32_t { x = x * 1664525u + 1013904223u; return x; };

    // Consumers: alternate between sem_clockwait(MONOTONIC) and sem_timedwait.
    std::vector<std::thread> cs;
    cs.reserve(CONSUMERS);
    for (int i = 0; i < CONSUMERS; ++i) {
        cs.emplace_back([&, i]() noexcept {
            uint32_t rng = 0x9E3779B9u + i;
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();

            for (;;) {
                int before = consumed.load(std::memory_order_acquire);
                if (before >= PRODUCERS * TOKENS_PER_PRODUCER) break;

                int rc = -1;
                if (i & 1) {
                    timespec abs{};
                    ASSERT_TRUE(MakeAbsTimespecForClock(CLOCK_MONOTONIC, WAIT_SLICE, &abs));
                    rc = ClockWait(&s, CLOCK_MONOTONIC, &abs);
                }
                else {
                    timespec abs = MakeAbsTimespecRealtime(WAIT_SLICE);
                    rc = TimedWait(&s, &abs);
                }

                if (rc == 0) {
                    consumed.fetch_add(1, std::memory_order_acq_rel);
                }
                else if (errno != ETIMEDOUT) {
                    ADD_FAILURE() << "Unexpected wait error, errno=" << errno;
                    return;
                }

                if ((lcg(rng) & 3u) == 0u)
                    std::this_thread::sleep_for(milliseconds(1));
            }
            });
    }

    // Producers: randomized small batches.
    std::vector<std::thread> ps;
    ps.reserve(PRODUCERS);
    for (int p = 0; p < PRODUCERS; ++p) {
        ps.emplace_back([&, p]() noexcept {
            uint32_t rng = 0xB5297A4Du + p;
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();

            for (int k = 0; k < TOKENS_PER_PRODUCER; ++k) {
                int batch = 1 + int(lcg(rng) % 3);
                for (int b = 0; b < batch && k < TOKENS_PER_PRODUCER; ++b, ++k) {
                    ASSERT_EQ(sem_post(&s), 0);
                    produced.fetch_add(1, std::memory_order_acq_rel);
                }
                --k; // adjust for outer ++k
                std::this_thread::sleep_for(PRODUCER_PAUSE);
            }
            });
    }

    start.store(true, std::memory_order_release);

    for (auto& t : ps) t.join();

    // Allow consumers to drain with bounded time.
    auto deadline = steady_clock::now() + seconds(5);
    while (consumed.load(std::memory_order_acquire) < produced.load(std::memory_order_acquire) &&
        steady_clock::now() < deadline) {
        std::this_thread::sleep_for(milliseconds(5));
    }

    for (auto& t : cs) t.join();

    EXPECT_EQ(consumed.load(std::memory_order_acquire), produced.load(std::memory_order_acquire));
    EXPECT_EQ(produced.load(std::memory_order_acquire), PRODUCERS * TOKENS_PER_PRODUCER);

    int v = -1;
    ASSERT_EQ(sem_getvalue(&s, &v), 0);
    EXPECT_EQ(v, 0);

    EXPECT_EQ(sem_destroy(&s), 0);
}

TEST(PosixSem, StressTryWaitAndTimedWaitMix) {
    sem_t s;
    ASSERT_EQ(sem_init(&s, 0, 0), 0);

    constexpr int PRODUCERS = 4;
    constexpr int CONSUMERS = 8;
    constexpr int TOKENS_PER_PRODUCER = 200;   // total 800
    constexpr milliseconds WAIT_SLICE(15);

    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    std::atomic<bool> start{false};

    // Consumers: try fast-path first, then fall back to timed waits.
    std::vector<std::thread> cs;
    cs.reserve(CONSUMERS);
    for (int i = 0; i < CONSUMERS; ++i) {
        cs.emplace_back([&]() noexcept {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (;;) {
                int done = consumed.load(std::memory_order_acquire);
                if (done >= PRODUCERS * TOKENS_PER_PRODUCER) break;

                if (sem_trywait(&s) == 0) {
                    consumed.fetch_add(1, std::memory_order_acq_rel);
                    continue;
                }
                if (errno != EAGAIN) { ADD_FAILURE() << "sem_trywait unexpected errno=" << errno; return; }

                timespec abs = MakeAbsTimespecRealtime(WAIT_SLICE);
                int rc = TimedWait(&s, &abs);
                if (rc == 0) {
                    consumed.fetch_add(1, std::memory_order_acq_rel);
                }
                else if (errno != ETIMEDOUT) {
                    ADD_FAILURE() << "sem_timedwait unexpected errno=" << errno;
                    return;
                }
            }
            });
    }

    // Producers: steady posting to increase contention.
    std::vector<std::thread> ps;
    ps.reserve(PRODUCERS);
    for (int p = 0; p < PRODUCERS; ++p) {
        ps.emplace_back([&]() noexcept {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (int k = 0; k < TOKENS_PER_PRODUCER; ++k) {
                ASSERT_EQ(sem_post(&s), 0);
                produced.fetch_add(1, std::memory_order_acq_rel);
            }
            });
    }

    start.store(true, std::memory_order_release);

    for (auto& t : ps) t.join();

    // Drain with deadline.
    auto deadline = steady_clock::now() + seconds(5);
    while (consumed.load(std::memory_order_acquire) < produced.load(std::memory_order_acquire) &&
        steady_clock::now() < deadline) {
        std::this_thread::sleep_for(milliseconds(2));
    }

    for (auto& t : cs) t.join();

    EXPECT_EQ(consumed.load(std::memory_order_acquire), produced.load(std::memory_order_acquire));
    EXPECT_EQ(produced.load(std::memory_order_acquire), PRODUCERS * TOKENS_PER_PRODUCER);

    int v = -1;
    ASSERT_EQ(sem_getvalue(&s, &v), 0);
    EXPECT_EQ(v, 0);

    EXPECT_EQ(sem_destroy(&s), 0);
}
