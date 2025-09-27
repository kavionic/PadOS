// std_concurrency_tests.cpp
// A pragmatic, on-device GoogleTest suite for C++ concurrency primitives.
// Focus: correctness + bounded waits. No hanging operations.
//
// Build notes:
//  - Link with gtest & gtest_main OR call RunStdConcurrencyTests() yourself.
//  - Adjust TEST_BASE_MS below if your RTOS tick is coarse/slow.
//  - All waits use steady_clock and timeouts to avoid hangs.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
#include <vector>
#include <deque>
#include <numeric>
#if 1
#if __has_include(<shared_mutex>)
#include <shared_mutex>
#endif

#if __has_include(<semaphore>)
#include <semaphore>
#endif

#if __has_include(<latch>)
#include <latch>
#endif

#if __has_include(<barrier>)
#include <barrier>
#endif

#if __has_include(<stop_token>)
#include <stop_token>
#include <thread> // jthread also lives here
#endif

// Feature-test convenience (guard newer headers on older libstdc++)
#ifndef __cpp_lib_shared_mutex
#define __cpp_lib_shared_mutex 0
#endif
#ifndef __cpp_lib_semaphore
#define __cpp_lib_semaphore 0
#endif
#ifndef __cpp_lib_latch
#define __cpp_lib_latch 0
#endif
#ifndef __cpp_lib_barrier
#define __cpp_lib_barrier 0
#endif
#ifndef __cpp_lib_jthread
#define __cpp_lib_jthread 0
#endif
#ifndef __cpp_lib_atomic_wait
#define __cpp_lib_atomic_wait 0
#endif

using namespace std::chrono;

// ---------- Tunables ----------
#ifndef TEST_BASE_MS
#define TEST_BASE_MS 10 // 20  // Bump if your platform is slow or tick >= 10ms
#endif
static constexpr auto kShort = milliseconds(TEST_BASE_MS);
static constexpr auto kLong = milliseconds(TEST_BASE_MS * 5);
static constexpr auto kTimeOut = milliseconds(TEST_BASE_MS * 10);

// Small helper: wait for predicate with timeout (no busy spin).
template<class Pred>
static bool WaitPred(std::mutex& m, std::condition_variable& cv, Pred&& pred, milliseconds to)
{
    std::unique_lock<std::mutex> lk(m);
    return cv.wait_for(lk, to, std::forward<Pred>(pred));
}

// ---------- Basic threading ----------
TEST(StdConcurrency, ThreadJoinAndIdUniqueness)
{
    std::thread::id main_id = std::this_thread::get_id();
    std::thread::id t_id;
    std::thread t([&] noexcept { t_id = std::this_thread::get_id(); });
    t.join();
    EXPECT_NE(main_id, t_id);
}

TEST(StdConcurrency, SleepYieldNotFail)
{
    auto t0 = steady_clock::now();
    std::this_thread::sleep_for(kShort);
    EXPECT_GE(steady_clock::now() - t0, kShort);
    std::this_thread::yield(); // should not deadlock or crash
    SUCCEED();
}
// ---------- Mutex family ----------
TEST(StdConcurrency, MutexProtectsIncrement)
{
    std::mutex m;
    int shared = 0;
    constexpr int threads = 4;
    constexpr int per = 1000;
    std::vector<std::thread> ts;
    ts.reserve(threads);
    for (int i = 0; i < threads; ++i) {
        ts.emplace_back([&] {
            for (int j = 0; j < per; ++j) {
                std::lock_guard<std::mutex> g(m);
                ++shared;
            }
            });
    }
    for (auto& th : ts) th.join();
    EXPECT_EQ(shared, threads * per);
}

TEST(StdConcurrency, MutexTryLockContendedThenSucceeds)
{
    std::mutex m;
    std::atomic<bool> locked{false};
    std::thread t([&] {
        m.lock();
        locked.store(true, std::memory_order_release);
        std::this_thread::sleep_for(kShort);
        m.unlock();
        });

    // Spin with bounded time to observe contention.
    auto deadline = steady_clock::now() + kTimeOut;
    bool saw_contention = false;
    while (steady_clock::now() < deadline)
    {
        if (m.try_lock()) {
            m.unlock();
            continue; // success without contention (rare)
        }
        else {
            if (locked.load(std::memory_order_acquire)) {
                saw_contention = true;
                // wait until t unlocks
                std::this_thread::sleep_for(milliseconds(1));
            }
        }
    }
    t.join();
    // After t joined, lock must succeed quickly.
    EXPECT_TRUE(m.try_lock());
    m.unlock();
    EXPECT_TRUE(saw_contention);
}

TEST(StdConcurrency, UniqueLockBasicsAndDefer)
{
    std::mutex m;
    std::unique_lock<std::mutex> lk(m, std::defer_lock);
    EXPECT_FALSE(lk.owns_lock());
    lk.lock();
    EXPECT_TRUE(lk.owns_lock());
    lk.unlock();
    EXPECT_FALSE(lk.owns_lock());
}

/////////////////////////////////////////////////////

TEST(StdConcurrency, RecursiveMutexReentrancy)
{
    std::recursive_mutex rm;
    rm.lock();
    rm.lock();
    rm.unlock();
    rm.unlock();
    SUCCEED();
}

TEST(StdConcurrency, TimedMutexTryLockFor)
{
    std::timed_mutex tm;
    std::atomic<bool> locked{false};
    std::thread t([&] {
        tm.lock();
        locked = true;
        std::this_thread::sleep_for(kShort);
        tm.unlock();
        locked = false;
        });
    while (!locked) { std::this_thread::yield(); }
    // Should fail quickly while held
    bool result = tm.try_lock_for(kShort / 2);
    if (result) {
        tm.unlock();
    }
    EXPECT_FALSE(result);
    // Then succeed later
    while (locked) { std::this_thread::yield(); }
    result = tm.try_lock_for(kLong);
    EXPECT_TRUE(result);
    if (result) {
        tm.unlock();
    }
    t.join();
}

#if __cpp_lib_shared_mutex
TEST(StdConcurrency, SharedMutexReadersThenWriter)
{
    std::shared_mutex sm;
    std::atomic<int> readers_inside{0};
    std::atomic<bool> readers_done{false};

    std::thread r1([&] { std::shared_lock l(sm); ++readers_inside; std::this_thread::sleep_for(kShort); --readers_inside; });
    std::thread r2([&] { std::shared_lock l(sm); ++readers_inside; std::this_thread::sleep_for(kShort); --readers_inside; });

    // Give readers a head start
    std::this_thread::sleep_for(milliseconds(5));

    bool writer_acquired = false;
    std::thread w([&] {
        std::unique_lock wl(sm); // should block until readers exit
        writer_acquired = true;
        });

    r1.join(); r2.join();
    // Once readers done, writer should acquire within kLong
    auto start = steady_clock::now();
    while (!writer_acquired && steady_clock::now() - start < kLong) {
        std::this_thread::sleep_for(milliseconds(1));
    }
    EXPECT_TRUE(writer_acquired);
    w.join();
}
#endif

// ---------- Condition variables ----------
TEST(StdConcurrency, ConditionVariableProducerConsumer)
{
    std::mutex m;
    std::condition_variable cv;
    std::deque<int> q;
    const int N = 100;

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) {
            {
                std::lock_guard<std::mutex> g(m);
                q.push_back(i);
            }
            cv.notify_one();
        }
        });

    int sum = 0;
    int got = 0;
    auto deadline = steady_clock::now() + kTimeOut;
    while (got < N && steady_clock::now() < deadline) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait_for(lk, kLong, [&] { return !q.empty(); });
        while (!q.empty()) {
            sum += q.front(); q.pop_front(); ++got;
        }
    }
    producer.join();
    EXPECT_EQ(got, N);
    EXPECT_EQ(sum, (N - 1) * N / 2);
}


TEST(StdConcurrency, ConditionVariableNotifyAll)
{
    std::mutex m; std::condition_variable cv;
    bool go = false;
    const int T = 4;
    int woke = 0;

    std::vector<std::thread> ts;
    for (int i = 0; i < T; ++i) {
        ts.emplace_back([&] {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&] { return go; });
            ++woke;
            });
    }

    {
        std::lock_guard<std::mutex> g(m);
        go = true;
    }
    cv.notify_all();
    for (auto& th : ts) th.join();
    EXPECT_EQ(woke, T);
}

// ---------- Futures/Promises/Async ----------
TEST(StdConcurrency, PromiseFutureRoundtrip)
{
    std::promise<int> p;
    auto f = p.get_future();
    std::thread t([p = std::move(p)]() mutable {
        std::this_thread::sleep_for(kShort);
        p.set_value(123);
        });
    EXPECT_EQ(f.wait_for(kTimeOut), std::future_status::ready);
    EXPECT_EQ(f.get(), 123);
    t.join();
}

TEST(StdConcurrency, PackagedTaskWorks)
{
    std::packaged_task<int(int)> task([](int n) noexcept {
        int s = 0; for (int i = 1; i <= n; ++i) s += i; return s;
        });
    auto fut = task.get_future();
    std::thread t(std::move(task), 100);
    EXPECT_EQ(fut.wait_for(kTimeOut), std::future_status::ready);
    EXPECT_EQ(fut.get(), 5050);
    t.join();
}

TEST(StdConcurrency, AsyncLaunchAsync)
{
    auto fut = std::async(std::launch::async, [] {
        std::this_thread::sleep_for(kShort);
        return 7;
        });
    EXPECT_EQ(fut.wait_for(kTimeOut), std::future_status::ready);
    EXPECT_EQ(fut.get(), 7);
}

 // ---------- once_flag / call_once ----------
TEST(StdConcurrency, CallOnceInitializesExactlyOnce)
{
    std::once_flag flag;
    std::atomic<int> init_calls{0};
    auto init = [&] noexcept { ++init_calls; };

    const int T = 8;
    std::vector<std::thread> ts;
    for (int i = 0; i < T; ++i) {
        ts.emplace_back([&] noexcept { std::call_once(flag, init); });
    }
    for (auto& th : ts) th.join();
    EXPECT_EQ(init_calls.load(), 1);
}

// ---------- Atomics ----------
TEST(StdConcurrency, AtomicMessagePassingAcquireRelease)
{
    std::atomic<int> data{0};
    std::atomic<bool> ready{false};

    int observed = 0;
    std::thread consumer([&] {
        // Wait until producer signals ready
        while (!ready.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(milliseconds(1));
        }
        observed = data.load(std::memory_order_relaxed);
        });

    data.store(42, std::memory_order_relaxed);
    ready.store(true, std::memory_order_release);
    consumer.join();
    EXPECT_EQ(observed, 42);
}

#if __cpp_lib_atomic_wait
TEST(StdConcurrency, AtomicWaitNotifyOne)
{
    std::atomic<int> val{0};
    std::atomic<bool> woke{false};

    std::thread waiter([&] noexcept {
        int expected = 0;
        val.wait(expected); // blocks until val != expected
        woke.store(true, std::memory_order_release);
        });

    std::this_thread::sleep_for(kShort);
    val.store(1, std::memory_order_release);
    val.notify_one();

    auto start = steady_clock::now();
    while (!woke.load(std::memory_order_acquire) && steady_clock::now() - start < kTimeOut) {
        std::this_thread::sleep_for(milliseconds(1));
    }
    waiter.join();
    EXPECT_TRUE(woke.load());
}
#endif

// ---------- C++20 syncs ----------
#if __cpp_lib_semaphore
TEST(StdConcurrency, CountingSemaphore)
{
    std::counting_semaphore<4> sem(0);
    std::atomic<int> count{0};
    std::thread prod([&] { for (int i = 0; i < 3; ++i) { std::this_thread::sleep_for(milliseconds(2)); sem.release(); } });

    std::thread cons([&] noexcept {
        for (int i = 0; i < 3; ++i) { sem.acquire(); ++count; }
        });

    prod.join(); cons.join();
    EXPECT_EQ(count.load(), 3);
}
#endif

#if __cpp_lib_latch
TEST(StdConcurrency, LatchArrivesAndWaits)
{
    const int T = 4;
    std::latch lat(T);
    std::atomic<int> passed{0};

    std::vector<std::thread> ts;
    for (int i = 0; i < T; ++i) {
        ts.emplace_back([&] { std::this_thread::sleep_for(kShort); lat.count_down(); lat.wait(); ++passed; });
    }
    for (auto& th : ts) th.join();
    EXPECT_EQ(passed.load(), T);
}
#endif

#if __cpp_lib_barrier
TEST(StdConcurrency, BarrierPhases)
{
    const int T = 4;
    std::atomic<int> phase0{0}, phase1{ 0 };
    std::barrier bar(T, [&] { ++phase0; }); // completion increments phase0

    std::vector<std::thread> ts;
    for (int i = 0; i < T; ++i) {
        ts.emplace_back([&] {
            bar.arrive_and_wait();
            ++phase1;
            bar.arrive_and_wait();
            });
    }
    for (auto& th : ts) th.join();
    EXPECT_EQ(phase0.load(), 2);   // completion ran twice
    EXPECT_EQ(phase1.load(), T);   // all passed mid-point
}
#endif

//=========================

#if __cpp_lib_jthread
TEST(StdConcurrency, JThreadStopToken)
{
    std::atomic<bool> observed_stop{false};
    std::jthread jt([&](std::stop_token st) {
        while (!st.stop_requested()) {
            std::this_thread::sleep_for(milliseconds(1));
        }
        observed_stop.store(true, std::memory_order_release);
        });
    std::this_thread::sleep_for(kShort);
    jt.request_stop();
    jt.join();
    EXPECT_TRUE(observed_stop.load(std::memory_order_acquire));
}
#endif

// ---------- Thread-local cleanup (optional) ----------
TEST(StdConcurrency, ThreadLocalDestructorRunsOnExit)
{
    struct T {
        std::atomic<int>* p;
        ~T() { if (p) p->fetch_add(1, std::memory_order_relaxed); }
    };
    std::atomic<int> dtor_count{0};

    std::thread t([&] noexcept {
        thread_local T tl{&dtor_count};
        (void)tl;
        });
    t.join();

    // Destructor should have run once as thread exited.
    EXPECT_EQ(dtor_count.load(), 1);
}
#if 0
#endif
// ---------- Optional entry point you can call from firmware ----------
//extern "C" int RunStdConcurrencyTests()
//{
//    // If you already link gtest_main and call RUN_ALL_TESTS elsewhere, you don't need this.
//    int argc = 1;
//    char arg0[] = "std_concurrency_tests";
//    char* argv[] = { arg0, nullptr };
//    ::testing::InitGoogleTest(&argc, argv);
//    return RUN_ALL_TESTS();
//}
#endif