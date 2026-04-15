// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 14.04.2026


#include <gtest/gtest.h>

#include <atomic>

#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>


#include <PadOS/Threads.h>

#include <System/AppDefinition.h>

namespace exit_tests
{

///////////////////////////////////////////////////////////////////////////////
// Shared globals
//
// PadOS has no MMU, so all processes share the same physical address space.
// Globals declared here are visible to every thread in every spawned process,
// allowing destructors to signal the test runner without any file I/O.
///////////////////////////////////////////////////////////////////////////////

static std::atomic<bool> g_probe_main{false};
static std::atomic<bool> g_probe_worker{false};
static std::atomic<bool> g_worker_ready{false};
static std::atomic<bool> g_main_ready{false};

///////////////////////////////////////////////////////////////////////////////
// RAII probe — sets a flag on destruction to prove the stack was unwound
///////////////////////////////////////////////////////////////////////////////

struct StackUnwindProbe
{
    std::atomic<bool>& m_flag;

    explicit StackUnwindProbe(std::atomic<bool>& flag) : m_flag(flag) {}
    ~StackUnwindProbe() { m_flag.store(true, std::memory_order_release); }

    StackUnwindProbe(const StackUnwindProbe&) = delete;
    StackUnwindProbe& operator=(const StackUnwindProbe&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
// Cancellation-point spin loop used by threads that must stay alive until
// exit() is called from another thread.
//
// Marked [[noreturn]]: the loop either runs forever or the thread is
// terminated via forced unwind through thread_testcancel().
///////////////////////////////////////////////////////////////////////////////

[[noreturn]] static void cancellation_spin()
{
    while (true)
    {
        thread_testcancel();
        snooze_ms(1);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Worker thread entry points (ThreadEntryPoint_t = void* (*)(void*))
//
// Workers use thread_spawn() so they go through PadOS's thread_entry_trampoline
// rather than libstdc++'s execute_native_thread_routine.  This matters because
// _Unwind_ForcedUnwind must not propagate through execute_native_thread_routine
// (it would crash trying to delete the std::thread _State during unwind).
///////////////////////////////////////////////////////////////////////////////

// Worker for test 3: spins at a cancellation point with a probe on its stack.
static void* worker_spin_with_probe(void*)
{
    StackUnwindProbe probe(g_probe_worker);
    g_worker_ready.store(true, std::memory_order_release);
    cancellation_spin();
}

// Worker for test 4: calls exit() with a probe on its stack.
static void* worker_calls_exit(void*)
{
    StackUnwindProbe probe(g_probe_worker);
    exit(0);
    return nullptr;
}

// Worker for test 5: waits until the main thread is ready, then calls exit().
static void* worker_waits_and_exits(void*)
{
    while (!g_main_ready.load(std::memory_order_acquire)) {
        snooze_ms(1);
    }
    StackUnwindProbe probe(g_probe_worker);
    exit(0);
    return nullptr;
}

// Worker for test 6: returns naturally from its entry function.
static void* worker_natural_return(void*)
{
    StackUnwindProbe probe(g_probe_worker);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// Test application implementations
///////////////////////////////////////////////////////////////////////////////

// exit() called from the main thread with a probe on the stack.
static int app_exit_main_unwind(int /*argc*/, char* /*argv*/[])
{
    StackUnwindProbe probe(g_probe_main);
    exit(0);
    return 0;
}

// main() returns normally — probe destructor runs via normal C++ scope exit.
static int app_return_main_unwind(int /*argc*/, char* /*argv*/[])
{
    StackUnwindProbe probe(g_probe_main);
    return 0;
}

// Main thread starts a worker (worker_spin_with_probe) then calls exit().
// Both stacks should be unwound: main via exit()'s forced unwind, worker via
// the cancellation mechanism (CancelThreads -> thread_testcancel).
static int app_exit_main_unwinds_worker(int /*argc*/, char* /*argv*/[])
{
    pid_t worker_id;
    thread_spawn(&worker_id, nullptr, worker_spin_with_probe, nullptr);

    while (!g_worker_ready.load(std::memory_order_acquire)) {
        snooze_ms(1);
    }

    StackUnwindProbe probe(g_probe_main);
    exit(0);
    return 0;
}

// A worker thread calls exit() — worker stack is unwound by exit()'s forced
// unwind. Main thread spins at cancellation points and is cancelled by kexit().
static int app_exit_worker_unwinds_worker(int /*argc*/, char* /*argv*/[])
{
    pid_t worker_id;
    thread_spawn(&worker_id, nullptr, worker_calls_exit, nullptr);
    cancellation_spin();
}

// Main thread has a probe and spins at a cancellation point. Worker calls
// exit() once the main thread is ready. Main stack is unwound via cancellation.
static int app_exit_worker_unwinds_main(int /*argc*/, char* /*argv*/[])
{
    pid_t worker_id;
    thread_spawn(&worker_id, nullptr, worker_waits_and_exits, nullptr);

    StackUnwindProbe probe(g_probe_main);
    g_main_ready.store(true, std::memory_order_release);
    cancellation_spin();
}

// Worker returns naturally — objects on its stack are destroyed by normal
// C++ scope exit before thread_entry_trampoline calls thread_exit().
// Main waits for the probe to be set, then returns.
static int app_worker_natural_return(int /*argc*/, char* /*argv*/[])
{
    pid_t worker_id;
    thread_spawn(&worker_id, nullptr, worker_natural_return, nullptr);

    while (!g_probe_worker.load(std::memory_order_acquire)) {
        snooze_ms(1);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// App registrations — each entry is automatically visible as /bin/<name>
///////////////////////////////////////////////////////////////////////////////

static PAppDefinition s_app_exit_main_unwind          ("xt_exit_main_unwind",           "", app_exit_main_unwind);
static PAppDefinition s_app_return_main_unwind        ("xt_return_main_unwind",         "", app_return_main_unwind);
static PAppDefinition s_app_exit_main_unwinds_worker  ("xt_exit_main_unwinds_worker",   "", app_exit_main_unwinds_worker);
static PAppDefinition s_app_exit_worker_unwinds_worker("xt_exit_worker_unwinds_worker", "", app_exit_worker_unwinds_worker);
static PAppDefinition s_app_exit_worker_unwinds_main  ("xt_exit_worker_unwinds_main",   "", app_exit_worker_unwinds_main);
static PAppDefinition s_app_worker_natural_return     ("xt_worker_natural_return",      "", app_worker_natural_return);

///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

static int WaitChild(pid_t pid)
{
    int status = 0;
    return (waitpid(pid, &status, 0) == pid && WIFEXITED(status))
        ? WEXITSTATUS(status) : -1;
}

static pid_t SpawnApp(const char* path)
{
    char* argv[] = { const_cast<char*>(path), nullptr };
    pid_t pid = -1;
    return (posix_spawn(&pid, path, nullptr, nullptr, argv, environ) == 0) ? pid : -1;
}

///////////////////////////////////////////////////////////////////////////////
// Test fixture — resets all shared globals before each test so that one
// test's process cannot contaminate the next.
///////////////////////////////////////////////////////////////////////////////

class ExitTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_probe_main.store(false, std::memory_order_relaxed);
        g_probe_worker.store(false, std::memory_order_relaxed);
        g_worker_ready.store(false, std::memory_order_relaxed);
        g_main_ready.store(false, std::memory_order_relaxed);
    }
};

///////////////////////////////////////////////////////////////////////////////
// Tests
///////////////////////////////////////////////////////////////////////////////

// exit() unwinds the calling (main) thread's stack.
TEST_F(ExitTest, ExitFromMain_UnwindsMainStack)
{
    const pid_t pid = SpawnApp("/bin/xt_exit_main_unwind");
    ASSERT_GT(pid, 0);
    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_TRUE(g_probe_main.load(std::memory_order_acquire));
}

// Returning from main() unwinds the stack via normal C++ scope exit.
TEST_F(ExitTest, ReturnFromMain_UnwindsMainStack)
{
    const pid_t pid = SpawnApp("/bin/xt_return_main_unwind");
    ASSERT_GT(pid, 0);
    WaitChild(pid);
    EXPECT_TRUE(g_probe_main.load(std::memory_order_acquire));
}

// exit() from the main thread cancels and unwinds all running worker threads.
TEST_F(ExitTest, ExitFromMain_UnwindsWorkerStack)
{
    const pid_t pid = SpawnApp("/bin/xt_exit_main_unwinds_worker");
    ASSERT_GT(pid, 0);
    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_TRUE(g_probe_main.load(std::memory_order_acquire));
    EXPECT_TRUE(g_probe_worker.load(std::memory_order_acquire));
}

// exit() called from a worker thread unwinds the worker's own stack.
TEST_F(ExitTest, ExitFromWorker_UnwindsWorkerStack)
{
    const pid_t pid = SpawnApp("/bin/xt_exit_worker_unwinds_worker");
    ASSERT_GT(pid, 0);
    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_TRUE(g_probe_worker.load(std::memory_order_acquire));
}

// exit() from a worker thread cancels and unwinds the main thread's stack.
TEST_F(ExitTest, ExitFromWorker_UnwindsMainStack)
{
    const pid_t pid = SpawnApp("/bin/xt_exit_worker_unwinds_main");
    ASSERT_GT(pid, 0);
    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_TRUE(g_probe_main.load(std::memory_order_acquire));
    EXPECT_TRUE(g_probe_worker.load(std::memory_order_acquire));
}

// A worker that returns from its entry function unwinds its stack normally
// before thread_entry_trampoline calls thread_exit().
TEST_F(ExitTest, WorkerNaturalReturn_UnwindsWorkerStack)
{
    const pid_t pid = SpawnApp("/bin/xt_worker_natural_return");
    ASSERT_GT(pid, 0);
    WaitChild(pid);
    EXPECT_TRUE(g_probe_worker.load(std::memory_order_acquire));
}

} // namespace exit_tests
