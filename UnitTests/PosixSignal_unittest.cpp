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
// Created: 16.04.2026 20:30

#include <gtest/gtest.h>

#include <atomic>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>

#include <PadOS/Threads.h>

#include <System/AppDefinition.h>

namespace posix_signal_tests
{

///////////////////////////////////////////////////////////////////////////////
// Shared globals
//
// PadOS has no MMU — all spawned child processes and worker threads share
// these globals directly, so they serve as cross-process/cross-thread probes
// without any file I/O.
///////////////////////////////////////////////////////////////////////////////

// In-process signal delivery observation
static std::atomic<int>       g_handler_count{0};
static std::atomic<int>       g_last_signo{0};
static std::atomic<int>       g_si_value_int{0};
static std::atomic<int>       g_si_code{0};

// Cross-thread coordination
static std::atomic<bool>      g_worker_ready{false};
static std::atomic<bool>      g_main_proceed{false};
static std::atomic<bool>      g_worker_done{false};
static thread_id              g_main_thread_id{0};
static thread_id              g_worker_thread_id{0};

// Cross-process probes
static std::atomic<bool>      g_child_running{false};
static std::atomic<bool>      g_child_ign_ready{false};

// Fault-signal SA_SIGINFO probe — written by child, read by parent after exit
static std::atomic<int>       g_fault_si_code{0};
static std::atomic<uintptr_t> g_fault_si_addr{0};

///////////////////////////////////////////////////////////////////////////////
// Signal handlers
//
// Must be plain C-linkage functions (no lambdas) so they can be stored in
// sa_handler / sa_sigaction function pointer fields.
///////////////////////////////////////////////////////////////////////////////

static void handler_record(int signo)
{
    g_last_signo.store(signo, std::memory_order_release);
    g_handler_count.fetch_add(1, std::memory_order_release);
}

static void handler_siginfo(int /*signo*/, siginfo_t* info, void*)
{
    g_last_signo.store(info->si_signo, std::memory_order_release);
    g_si_code.store(info->si_code, std::memory_order_release);
    g_si_value_int.store(info->si_value.sival_int, std::memory_order_release);
    g_handler_count.fetch_add(1, std::memory_order_release);
}

// Used by fault-induced signal tests.
// Stores si_code and si_addr then calls exit(42) — never returns, because
// returning from a fault signal handler re-executes the faulting instruction.
static void handler_fault_siginfo(int /*signo*/, siginfo_t* info, void*)
{
    g_fault_si_code.store(info->si_code, std::memory_order_release);
    g_fault_si_addr.store(reinterpret_cast<uintptr_t>(info->si_addr), std::memory_order_release);
    _exit(42);
}

///////////////////////////////////////////////////////////////////////////////
// Mini-app: spins forever (target for kill() cross-process tests)
///////////////////////////////////////////////////////////////////////////////

static int app_spin_forever(int, char*[])
{
    g_child_running.store(true, std::memory_order_release);
    while (true) { snooze_ms(10); }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Mini-app: installs SIG_IGN for SIGUSR1 then exits normally
///////////////////////////////////////////////////////////////////////////////

static int app_ignore_usr1(int, char*[])
{
    signal(SIGUSR1, SIG_IGN);
    g_child_ign_ready.store(true, std::memory_order_release);
    snooze_ms(200);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Mini-apps: fault-induced signals
//
// Each installs an SA_SIGINFO handler for the relevant signal, then
// deliberately triggers the fault. The handler stores si_code / si_addr and
// calls exit(42). The parent verifies WIFEXITED with code 42, plus the probes.
///////////////////////////////////////////////////////////////////////////////

// SIGSEGV — null-pointer write
static int app_sigsegv_handled(int, char*[])
{
    struct sigaction sa{};
    sa.sa_sigaction = handler_fault_siginfo;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, nullptr);

    volatile uint32_t* p = nullptr;
    *p = 0x12345678u;  // SIGSEGV
    return 1;          // unreachable
}

// SIGBUS — unaligned 4-byte write through a 1-byte-offset pointer
static uint8_t s_unaligned_buf[8];

static int app_sigbus_handled(int, char*[])
{
    struct sigaction sa{};
    sa.sa_sigaction = handler_fault_siginfo;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGBUS, &sa, nullptr);

    // -mno-unaligned-access causes the compiler to emit 4 byte stores, which
    // never trigger UNALIGN_TRP. Force a single unaligned STR via inline asm.
    uint32_t* p = reinterpret_cast<uint32_t*>(&s_unaligned_buf[1]);
    __asm volatile("str %1, [%0]" : : "r"(p), "r"(uint32_t(0x12345678u)) : "memory");  // SIGBUS (BUS_ADRALN)
    return 1;
}

// SIGFPE — integer divide-by-zero
// Requires the CPU's CCR.DIV_0_TRP bit to be set (PadOS enables this on
// Cortex-M7 so that SDIV by zero raises a UsageFault → SIGFPE FPE_INTDIV).
static int app_sigfpe_handled(int, char*[])
{
    struct sigaction sa{};
    sa.sa_sigaction = handler_fault_siginfo;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGFPE, &sa, nullptr);

    volatile int divisor = 0;
    volatile int result  = 100 / divisor;  // SIGFPE (FPE_INTDIV)
    (void)result;
    return 1;
}

// SIGILL — permanently-undefined instruction
static int app_sigill_handled(int, char*[])
{
    struct sigaction sa{};
    sa.sa_sigaction = handler_fault_siginfo;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGILL, &sa, nullptr);

    __asm volatile("udf\n" ::: "memory");  // SIGILL
    return 1;
}

// SIGSEGV with no handler — default action (TerminateCoreDump) kills the child
static int app_sigsegv_default(int, char*[])
{
    volatile uint32_t* p = nullptr;
    *p = 0x12345678u;  // SIGSEGV, default action
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// App registrations — automatically visible as /bin/<name>
///////////////////////////////////////////////////////////////////////////////

static PAppDefinition s_app_spin_forever   ("sig_spin_forever",    "", app_spin_forever);
static PAppDefinition s_app_ignore_usr1    ("sig_ignore_usr1",     "", app_ignore_usr1);
static PAppDefinition s_app_sigsegv_handled("sig_sigsegv_handled", "", app_sigsegv_handled);
static PAppDefinition s_app_sigbus_handled ("sig_sigbus_handled",  "", app_sigbus_handled);
static PAppDefinition s_app_sigfpe_handled ("sig_sigfpe_handled",  "", app_sigfpe_handled);
static PAppDefinition s_app_sigill_handled ("sig_sigill_handled",  "", app_sigill_handled);
static PAppDefinition s_app_sigsegv_default("sig_sigsegv_default", "", app_sigsegv_default);

///////////////////////////////////////////////////////////////////////////////
// Worker thread entry points (multi-thread B-category tests)
///////////////////////////////////////////////////////////////////////////////

// B1: delivers SIGUSR1 to the main thread once g_main_proceed is set
static void* worker_kill_main(void*)
{
    while (!g_main_proceed.load(std::memory_order_acquire)) { snooze_ms(1); }
    thread_kill(g_main_thread_id, SIGUSR1);
    g_worker_done.store(true, std::memory_order_release);
    return nullptr;
}

// B2: installs handler_record for SIGUSR1, announces readiness, spins until told to stop
static void* worker_wait_for_signal(void*)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);
    g_worker_thread_id = get_thread_id();
    g_worker_ready.store(true, std::memory_order_release);
    while (!g_worker_done.load(std::memory_order_acquire)) { snooze_ms(1); }
    return nullptr;
}

// B3: installs SA_SIGINFO handler_siginfo for SIGUSR1, same lifecycle as B2
static void* worker_wait_siginfo(void*)
{
    struct sigaction sa{};
    sa.sa_sigaction = handler_siginfo;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, nullptr);
    g_worker_thread_id = get_thread_id();
    g_worker_ready.store(true, std::memory_order_release);
    while (!g_worker_done.load(std::memory_order_acquire)) { snooze_ms(1); }
    return nullptr;
}


///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

struct WaitResult
{
    bool valid      = false;
    bool exited     = false;
    bool signaled   = false;
    bool stopped    = false;
    int  exitStatus = -1;
    int  termSig    = -1;
    int  stopSig    = -1;
};

static WaitResult WaitFor(pid_t pid, int options = 0)
{
    int status = 0;
    WaitResult r;
    if (waitpid(pid, &status, options) != pid) return r;
    r.valid = true;
    if      (WIFEXITED(status))   { r.exited   = true; r.exitStatus = WEXITSTATUS(status); }
    else if (WIFSIGNALED(status)) { r.signaled  = true; r.termSig   = WTERMSIG(status);    }
    else if (WIFSTOPPED(status))  { r.stopped   = true; r.stopSig   = WSTOPSIG(status);    }
    return r;
}

static pid_t SpawnApp(const char* path)
{
    char* argv[] = { const_cast<char*>(path), nullptr };
    pid_t pid = -1;
    return (posix_spawn(&pid, path, nullptr, nullptr, argv, environ) == 0) ? pid : -1;
}

// Polls flag every 1 ms until it becomes true or timeoutMs expires.
static bool SpinUntil(const std::atomic<bool>& flag, int timeoutMs = 500)
{
    for (int i = 0; i < timeoutMs; ++i)
    {
        if (flag.load(std::memory_order_acquire)) return true;
        snooze_ms(1);
    }
    return false;
}

// Polls counter every 1 ms until it reaches at least `target` or timeoutMs expires.
static bool SpinUntilCount(const std::atomic<int>& counter, int target = 1, int timeoutMs = 500)
{
    for (int i = 0; i < timeoutMs; ++i)
    {
        if (counter.load(std::memory_order_acquire) >= target) return true;
        snooze_ms(1);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Fixture — resets all shared globals before each test so that one test's
// process or worker cannot contaminate the next.
///////////////////////////////////////////////////////////////////////////////

class PosixSignalTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_handler_count.store(0, std::memory_order_relaxed);
        g_last_signo.store(0, std::memory_order_relaxed);
        g_si_value_int.store(0, std::memory_order_relaxed);
        g_si_code.store(0, std::memory_order_relaxed);
        g_worker_ready.store(false, std::memory_order_relaxed);
        g_main_proceed.store(false, std::memory_order_relaxed);
        g_worker_done.store(false, std::memory_order_relaxed);
        g_main_thread_id   = 0;
        g_worker_thread_id = 0;
        g_child_running.store(false, std::memory_order_relaxed);
        g_child_ign_ready.store(false, std::memory_order_relaxed);
        g_fault_si_code.store(0, std::memory_order_relaxed);
        g_fault_si_addr.store(0, std::memory_order_relaxed);

        // Restore SIGUSR1 and SIGUSR2 to SIG_DFL.
        struct sigaction sa{};
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGUSR1, &sa, nullptr);
        sigaction(SIGUSR2, &sa, nullptr);

        // Clear the current thread's signal mask entirely.
        sigset_t empty;
        sigemptyset(&empty);
        thread_sigmask(SIG_SETMASK, &empty, nullptr);
    }

    void TearDown() override
    {
        // Set SIG_IGN before clearing the mask so that any signal left pending by
        // the test is silently discarded rather than taking the default action
        // (which for SIGUSR1/SIGUSR2 would terminate the process).
        struct sigaction ign{};
        ign.sa_handler = SIG_IGN;
        sigemptyset(&ign.sa_mask);
        ign.sa_flags = 0;
        sigaction(SIGUSR1, &ign, nullptr);
        sigaction(SIGUSR2, &ign, nullptr);

        sigset_t empty;
        sigemptyset(&empty);
        thread_sigmask(SIG_SETMASK, &empty, nullptr);  // unblock; pending signals silently discarded

        struct sigaction dfl{};
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        dfl.sa_flags = 0;
        sigaction(SIGUSR1, &dfl, nullptr);
        sigaction(SIGUSR2, &dfl, nullptr);
    }
};

///////////////////////////////////////////////////////////////////////////////
// A — Single-thread, in-process tests
///////////////////////////////////////////////////////////////////////////////

// sigaction install/retrieve roundtrip — handler pointer survives storage.
TEST_F(PosixSignalTest, SigAction_InstallAndRetrieve)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    ASSERT_EQ(sigaction(SIGUSR1, &sa, nullptr), 0);

    struct sigaction old{};
    ASSERT_EQ(sigaction(SIGUSR1, nullptr, &old), 0);
    EXPECT_EQ(old.sa_handler, handler_record);
}

// SA_SIGINFO flag and sa_sigaction pointer both roundtrip through sigaction.
TEST_F(PosixSignalTest, SigAction_SAFlags_SASiginfo_Roundtrip)
{
    struct sigaction sa{};
    sa.sa_sigaction = handler_siginfo;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    ASSERT_EQ(sigaction(SIGUSR1, &sa, nullptr), 0);

    struct sigaction old{};
    ASSERT_EQ(sigaction(SIGUSR1, nullptr, &old), 0);
    EXPECT_NE(old.sa_flags & SA_SIGINFO, 0);
    EXPECT_EQ(old.sa_sigaction, handler_siginfo);
}

// The out-parameter of sigaction carries the previous handler.
TEST_F(PosixSignalTest, SigAction_ReturnsPreviousHandler)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);

    struct sigaction sa2{};
    sa2.sa_sigaction = handler_siginfo;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = SA_SIGINFO;

    struct sigaction prev{};
    ASSERT_EQ(sigaction(SIGUSR1, &sa2, &prev), 0);
    EXPECT_EQ(prev.sa_handler, handler_record);
}

// Passing null new-action reads the current disposition without changing it.
TEST_F(PosixSignalTest, SigAction_NullNewAction_ReadsWithoutChanging)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    struct sigaction read1{};
    sigaction(SIGUSR1, nullptr, &read1);
    EXPECT_EQ(read1.sa_handler, handler_record);

    // Change the handler, verify the second read reflects the change.
    struct sigaction sa2{};
    sa2.sa_sigaction = handler_siginfo;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa2, nullptr);

    struct sigaction read2{};
    sigaction(SIGUSR1, nullptr, &read2);
    EXPECT_EQ(read2.sa_sigaction, handler_siginfo);
}

// sigaction with SIG_IGN sets disposition to SIG_IGN, visible via retrieve.
// Note: POSIX signal() in PadOS's newlib port is a userspace-only simulation
// (SIGNAL_PROVIDED is not defined), so it does not touch the kernel sigaction
// table. All tests that need kernel-level disposition use sigaction() directly.
TEST_F(PosixSignalTest, Signal_SigIgn_InstallAndRetrieve)
{
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    ASSERT_EQ(sigaction(SIGUSR1, &sa, nullptr), 0);

    struct sigaction old{};
    ASSERT_EQ(sigaction(SIGUSR1, nullptr, &old), 0);
    EXPECT_EQ(old.sa_handler, SIG_IGN);
}

// sigaction with SIG_DFL resets a previously installed custom handler.
TEST_F(PosixSignalTest, Signal_SigDfl_ResetsDisposition)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);

    struct sigaction dfl{};
    dfl.sa_handler = SIG_DFL;
    sigemptyset(&dfl.sa_mask);
    dfl.sa_flags = 0;
    sigaction(SIGUSR1, &dfl, nullptr);

    struct sigaction old{};
    sigaction(SIGUSR1, nullptr, &old);
    EXPECT_EQ(old.sa_handler, SIG_DFL);
}

// raise() delivers the signal synchronously — handler has run before raise() returns.
// Handler is installed via signal() (not sigaction) so that newlib's raise()
// invokes it directly through the userspace simulation table.
TEST_F(PosixSignalTest, Raise_DeliversToCurrentThread)
{
    signal(SIGUSR1, handler_record);

    EXPECT_EQ(raise(SIGUSR1), 0);
    EXPECT_EQ(g_handler_count.load(std::memory_order_acquire), 1);
    EXPECT_EQ(g_last_signo.load(std::memory_order_acquire), SIGUSR1);
}

// SIG_IGN suppresses delivery — the handler counter stays at zero.
TEST_F(PosixSignalTest, Raise_SigIgn_DoesNotInvokeHandler)
{
    signal(SIGUSR1, SIG_IGN);
    raise(SIGUSR1);
    EXPECT_EQ(g_handler_count.load(std::memory_order_acquire), 0);
}

// raise() returns 0 on success.
TEST_F(PosixSignalTest, Raise_Returns_Zero_OnSuccess)
{
    signal(SIGUSR1, handler_record);
    EXPECT_EQ(raise(SIGUSR1), 0);
}

// Blocking a signal prevents its handler from running during the block.
TEST_F(PosixSignalTest, ThreadSigmask_Block_PreventsDelivery)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    thread_sigmask(SIG_BLOCK, &block, nullptr);

    thread_kill(get_thread_id(), SIGUSR1);  // marks signal pending on this thread
    EXPECT_EQ(g_handler_count.load(std::memory_order_acquire), 0);  // still blocked

    // Drain the pending signal explicitly so TearDown can safely clear the mask.
    thread_sigmask(SIG_UNBLOCK, &block, nullptr);
    // Handler fires on syscall return; count is now 1 but assertion is not the point here.
}

// Unblocking a signal delivers any pending instance synchronously on syscall return.
TEST_F(PosixSignalTest, ThreadSigmask_Unblock_DeliversPending)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    thread_sigmask(SIG_BLOCK, &block, nullptr);

    thread_kill(get_thread_id(), SIGUSR1);  // mark pending; blocked so not delivered yet
    EXPECT_EQ(g_handler_count.load(std::memory_order_acquire), 0);

    // thread_sigmask itself is a syscall; the pending signal fires on its return.
    thread_sigmask(SIG_UNBLOCK, &block, nullptr);
    EXPECT_EQ(g_handler_count.load(std::memory_order_acquire), 1);
    EXPECT_EQ(g_last_signo.load(std::memory_order_acquire), SIGUSR1);
}

// SIG_SETMASK completely replaces the mask; clearing SIGUSR1's block delivers the pending signal.
TEST_F(PosixSignalTest, ThreadSigmask_SetMask_ReplacesEntireMask)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    thread_sigmask(SIG_BLOCK, &block, nullptr);

    thread_kill(get_thread_id(), SIGUSR1);  // mark pending; blocked so not delivered yet
    EXPECT_EQ(g_handler_count.load(std::memory_order_acquire), 0);

    sigset_t empty;
    sigemptyset(&empty);
    thread_sigmask(SIG_SETMASK, &empty, nullptr);  // unblocks SIGUSR1; handler fires on return
    EXPECT_EQ(g_handler_count.load(std::memory_order_acquire), 1);
}

// Querying the old mask (null new-set) returns the active mask accurately.
TEST_F(PosixSignalTest, ThreadSigmask_GetOldMask_ReturnsCurrentMask)
{
    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    thread_sigmask(SIG_BLOCK, &block, nullptr);

    sigset_t old;
    sigemptyset(&old);
    thread_sigmask(SIG_BLOCK, nullptr, &old);

    EXPECT_NE(sigismember(&old, SIGUSR1), 0);
    EXPECT_EQ(sigismember(&old, SIGUSR2), 0);
}

// sigpending() shows signals that are both blocked and pending.
TEST_F(PosixSignalTest, SigPending_BlockedAndRaised_ShowsInSet)
{
    // Install a handler so the signal does not terminate if accidentally unblocked.
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    thread_sigmask(SIG_BLOCK, &block, nullptr);
    thread_kill(get_thread_id(), SIGUSR1);  // mark pending; blocked so not delivered yet

    sigset_t pending;
    sigemptyset(&pending);
    sigpending(&pending);
    EXPECT_NE(sigismember(&pending, SIGUSR1), 0);

    // Drain the pending signal so TearDown can safely clear the mask.
    thread_sigmask(SIG_UNBLOCK, &block, nullptr);
}

// A signal that was already delivered (unblocked) is absent from the pending set.
TEST_F(PosixSignalTest, SigPending_UnblockedSignal_NotInPendingSet)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    // SIGUSR1 is unblocked (SetUp ensures this); thread_kill delivers it on syscall return.
    thread_kill(get_thread_id(), SIGUSR1);
    EXPECT_EQ(g_handler_count.load(std::memory_order_acquire), 1);

    sigset_t pending;
    sigemptyset(&pending);
    sigpending(&pending);
    EXPECT_EQ(sigismember(&pending, SIGUSR1), 0);
}

// SA_RESETHAND: after first delivery, the disposition is reset to SIG_DFL and
// the SA_RESETHAND flag itself is cleared.
TEST_F(PosixSignalTest, SigAction_SAResethand_ResetsAfterDelivery)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;
    ASSERT_EQ(sigaction(SIGUSR1, &sa, nullptr), 0);

    thread_kill(get_thread_id(), SIGUSR1);  // delivered synchronously on syscall return
    EXPECT_EQ(g_handler_count.load(std::memory_order_acquire), 1);

    struct sigaction old{};
    sigaction(SIGUSR1, nullptr, &old);
    EXPECT_EQ(old.sa_handler, SIG_DFL);
    EXPECT_EQ(old.sa_flags & SA_RESETHAND, 0);
}

// sa_mask is stored and retrieved accurately — SIGUSR2 in the mask roundtrips.
TEST_F(PosixSignalTest, SigAction_SAMask_BlocksDuringHandler_Roundtrip)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGUSR2);
    sa.sa_flags = 0;
    ASSERT_EQ(sigaction(SIGUSR1, &sa, nullptr), 0);

    struct sigaction old{};
    sigaction(SIGUSR1, nullptr, &old);
    EXPECT_NE(sigismember(&old.sa_mask, SIGUSR2), 0);
    EXPECT_EQ(sigismember(&old.sa_mask, SIGUSR1), 0);
}

// Installing SIG_IGN for SIGKILL must not crash (the kernel ignores the install
// at delivery time). Behavioral verification is in Kill_SigKill_TerminatesChild.
TEST_F(PosixSignalTest, SigAction_SigKill_InstallDoesNotCrash)
{
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGKILL, &sa, nullptr);
    // Passes as long as the call does not crash or hang.
}

// Same for SIGSTOP.
TEST_F(PosixSignalTest, SigAction_SigStop_InstallDoesNotCrash)
{
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSTOP, &sa, nullptr);
}

// sigset_t arithmetic: empty/add/ismember/del all produce the correct membership.
TEST_F(PosixSignalTest, SigSet_EmptyAddDelIsmember)
{
    sigset_t s;
    sigemptyset(&s);
    EXPECT_EQ(sigismember(&s, SIGUSR1), 0);

    sigaddset(&s, SIGUSR1);
    EXPECT_NE(sigismember(&s, SIGUSR1), 0);
    EXPECT_EQ(sigismember(&s, SIGUSR2), 0);

    sigdelset(&s, SIGUSR1);
    EXPECT_EQ(sigismember(&s, SIGUSR1), 0);
}

// sigfillset produces a set that contains every standard signal.
TEST_F(PosixSignalTest, SigSet_Fillset_AllSignalsPresent)
{
    sigset_t s;
    sigfillset(&s);
    EXPECT_NE(sigismember(&s, SIGUSR1), 0);
    EXPECT_NE(sigismember(&s, SIGUSR2), 0);
    EXPECT_NE(sigismember(&s, SIGTERM), 0);
    EXPECT_NE(sigismember(&s, SIGHUP),  0);
    EXPECT_NE(sigismember(&s, SIGINT),  0);
}

// SA_NODEFER flag roundtrips through sigaction storage.
TEST_F(PosixSignalTest, SigAction_SANodefer_FlagRoundtrip)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NODEFER;
    ASSERT_EQ(sigaction(SIGUSR1, &sa, nullptr), 0);

    struct sigaction old{};
    sigaction(SIGUSR1, nullptr, &old);
    EXPECT_NE(old.sa_flags & SA_NODEFER, 0);
}

///////////////////////////////////////////////////////////////////////////////
// B — Multi-thread tests
///////////////////////////////////////////////////////////////////////////////

// thread_kill can target the main thread from a worker.
TEST_F(PosixSignalTest, ThreadKill_FromWorkerToMain_DeliversSignal)
{
    g_main_thread_id = get_thread_id();

    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    thread_id worker;
    thread_spawn(&worker, nullptr, worker_kill_main, nullptr);

    g_main_proceed.store(true, std::memory_order_release);

    EXPECT_TRUE(SpinUntilCount(g_handler_count));
    EXPECT_EQ(g_handler_count.load(), 1);
    EXPECT_EQ(g_last_signo.load(), SIGUSR1);
    EXPECT_TRUE(SpinUntil(g_worker_done));
}

// thread_kill can target a worker thread from the main thread.
TEST_F(PosixSignalTest, ThreadKill_FromMainToWorker_DeliversSignal)
{
    thread_id worker;
    thread_spawn(&worker, nullptr, worker_wait_for_signal, nullptr);

    ASSERT_TRUE(SpinUntil(g_worker_ready));

    thread_kill(g_worker_thread_id, SIGUSR1);

    EXPECT_TRUE(SpinUntilCount(g_handler_count));
    EXPECT_EQ(g_handler_count.load(), 1);
    EXPECT_EQ(g_last_signo.load(), SIGUSR1);

    g_worker_done.store(true, std::memory_order_release);
}

// thread_sigqueue delivers si_value.sival_int and sets si_code to SI_QUEUE.
TEST_F(PosixSignalTest, ThreadSigqueue_DeliversSivalAndSICode)
{
    thread_id worker;
    thread_spawn(&worker, nullptr, worker_wait_siginfo, nullptr);

    ASSERT_TRUE(SpinUntil(g_worker_ready));

    sigval_t val;
    val.sival_int = 42;
    thread_sigqueue(g_worker_thread_id, SIGUSR1, val);

    EXPECT_TRUE(SpinUntilCount(g_handler_count));
    EXPECT_EQ(g_si_value_int.load(), 42);
    EXPECT_EQ(g_si_code.load(), SI_QUEUE);

    g_worker_done.store(true, std::memory_order_release);
}

// sigsuspend() with a pre-pending signal: block SIGUSR1, raise it (makes it
// pending), then call sigsuspend with an empty mask.  The signal is already
// pending and unblocked under the sigsuspend mask, so the call returns EINTR
// immediately without sleeping.  After return the original mask (SIGUSR1
// blocked) must be restored.  SIGUSR1 is still pending at that point; drain it
// by unblocking so the handler fires and clears the pending bit.
TEST_F(PosixSignalTest, Sigsuspend_PendingSignal_ReturnsEINTR)
{
    struct sigaction sa{};
    sa.sa_handler = handler_record;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, nullptr);

    // Block SIGUSR1 and make it pending without delivering it.
    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    thread_sigmask(SIG_BLOCK, &block, nullptr);
    ASSERT_EQ(thread_kill(get_thread_id(), SIGUSR1), PErrorCode::Success);  // mark pending
    ASSERT_EQ(g_handler_count.load(), 0);  // still blocked, not delivered yet

    // sigsuspend with an empty mask unblocks SIGUSR1.  Since a matching signal
    // is already pending, sigsuspend must return EINTR immediately.
    sigset_t empty;
    sigemptyset(&empty);
    const int ret = sigsuspend(&empty);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EINTR);

    // sigsuspend must have restored the original mask (SIGUSR1 blocked again).
    sigset_t cur;
    sigemptyset(&cur);
    thread_sigmask(SIG_BLOCK, nullptr, &cur);
    EXPECT_NE(sigismember(&cur, SIGUSR1), 0);

    // SIGUSR1 is still pending; unblock it so the handler fires and clears the
    // pending bit before TearDown (which would otherwise deliver it with SIG_DFL
    // and terminate the process).
    thread_sigmask(SIG_UNBLOCK, &block, nullptr);
    EXPECT_EQ(g_handler_count.load(), 1);
}


// Sending a signal to a non-existent thread ID returns an error.
TEST_F(PosixSignalTest, ThreadKill_InvalidId_ReturnsError)
{
    const PErrorCode result = thread_kill(0x7FFFFFFF, SIGUSR1);
    EXPECT_NE(result, PErrorCode::Success);
}

///////////////////////////////////////////////////////////////////////////////
// C — Cross-process, kill() tests
///////////////////////////////////////////////////////////////////////////////

// kill(SIGKILL) terminates the child unconditionally.
TEST_F(PosixSignalTest, Kill_SigKill_TerminatesChild_WIFSIGNALED)
{
    const pid_t pid = SpawnApp("/bin/sig_spin_forever");
    ASSERT_GT(pid, 0);
    ASSERT_TRUE(SpinUntil(g_child_running));

    EXPECT_EQ(kill(pid, SIGKILL), 0);

    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.signaled);
    EXPECT_EQ(r.termSig, SIGKILL);
}

// SIGUSR1 default action is Terminate — child with no handler is killed.
TEST_F(PosixSignalTest, Kill_SigUsr1_DefaultTerminatesChild_WIFSIGNALED)
{
    const pid_t pid = SpawnApp("/bin/sig_spin_forever");
    ASSERT_GT(pid, 0);
    ASSERT_TRUE(SpinUntil(g_child_running));

    EXPECT_EQ(kill(pid, SIGUSR1), 0);

    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.signaled);
    EXPECT_EQ(r.termSig, SIGUSR1);
}

// SIGTERM default action is Terminate.
TEST_F(PosixSignalTest, Kill_SigTerm_DefaultTerminatesChild_WIFSIGNALED)
{
    const pid_t pid = SpawnApp("/bin/sig_spin_forever");
    ASSERT_GT(pid, 0);
    ASSERT_TRUE(SpinUntil(g_child_running));

    EXPECT_EQ(kill(pid, SIGTERM), 0);

    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.signaled);
    EXPECT_EQ(r.termSig, SIGTERM);
}

// A child that installs SIG_IGN survives SIGUSR1 and exits normally.
TEST_F(PosixSignalTest, Kill_SigIgn_ChildSurvivesSigusr1)
{
    const pid_t pid = SpawnApp("/bin/sig_ignore_usr1");
    ASSERT_GT(pid, 0);
    ASSERT_TRUE(SpinUntil(g_child_ign_ready));

    EXPECT_EQ(kill(pid, SIGUSR1), 0);

    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.exited);
    EXPECT_EQ(r.exitStatus, 0);
}

// SIGSTOP stops the child (waitpid with WUNTRACED returns WIFSTOPPED);
// SIGCONT resumes it so it can be cleaned up with SIGKILL.
TEST_F(PosixSignalTest, Kill_SigStop_WifStopped_SigCont_Resumes)
{
    const pid_t pid = SpawnApp("/bin/sig_spin_forever");
    ASSERT_GT(pid, 0);
    ASSERT_TRUE(SpinUntil(g_child_running));

    EXPECT_EQ(kill(pid, SIGSTOP), 0);

    int status = 0;
    EXPECT_EQ(waitpid(pid, &status, WUNTRACED), pid);
    EXPECT_TRUE(WIFSTOPPED(status));
    EXPECT_EQ(WSTOPSIG(status), SIGSTOP);

    EXPECT_EQ(kill(pid, SIGCONT), 0);
    EXPECT_EQ(kill(pid, SIGKILL), 0);

    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.signaled);
    EXPECT_EQ(r.termSig, SIGKILL);
}

///////////////////////////////////////////////////////////////////////////////
// D — Fault-induced signals (cross-process)
//
// Each test spawns a child that deliberately triggers a hardware fault.
// The child has an SA_SIGINFO handler installed; the handler stores
// si_code/si_addr in shared globals and calls exit(42), which performs
// proper stack unwinding before terminating.
///////////////////////////////////////////////////////////////////////////////

// Null-pointer write generates SIGSEGV; the child exits cleanly via exit(42).
TEST_F(PosixSignalTest, Fault_SigSegv_HandlerInvoked_ChildExitsCleanly)
{
    const pid_t pid = SpawnApp("/bin/sig_sigsegv_handled");
    ASSERT_GT(pid, 0);
    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.exited);
    EXPECT_EQ(r.exitStatus, 42);
}

// The SIGSEGV handler receives si_code == SEGV_MAPERR and si_addr == nullptr.
TEST_F(PosixSignalTest, Fault_SigSegv_SASigninfo_SiAddrAndSiCode)
{
    const pid_t pid = SpawnApp("/bin/sig_sigsegv_handled");
    ASSERT_GT(pid, 0);
    WaitFor(pid);
    EXPECT_EQ(g_fault_si_code.load(), SEGV_ACCERR);
    EXPECT_EQ(g_fault_si_addr.load(), uintptr_t(0));
}

// Unaligned 4-byte write generates SIGBUS; the child exits cleanly.
TEST_F(PosixSignalTest, Fault_SigBus_HandlerInvoked_ChildExitsCleanly)
{
    const pid_t pid = SpawnApp("/bin/sig_sigbus_handled");
    ASSERT_GT(pid, 0);
    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.exited);
    EXPECT_EQ(r.exitStatus, 42);
}

// The SIGBUS handler receives si_code == BUS_ADRALN.
TEST_F(PosixSignalTest, Fault_SigBus_SASigninfo_SiCodeIsBusAdraln)
{
    const pid_t pid = SpawnApp("/bin/sig_sigbus_handled");
    ASSERT_GT(pid, 0);
    WaitFor(pid);
    EXPECT_EQ(g_fault_si_code.load(), BUS_ADRALN);
}

// Integer divide-by-zero generates SIGFPE; the child exits cleanly.
TEST_F(PosixSignalTest, Fault_SigFpe_HandlerInvoked_ChildExitsCleanly)
{
    const pid_t pid = SpawnApp("/bin/sig_sigfpe_handled");
    ASSERT_GT(pid, 0);
    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.exited);
    EXPECT_EQ(r.exitStatus, 42);
}

// The SIGFPE handler receives si_code == FPE_INTDIV.
TEST_F(PosixSignalTest, Fault_SigFpe_SASigninfo_SiCodeIsFpeIntdiv)
{
    const pid_t pid = SpawnApp("/bin/sig_sigfpe_handled");
    ASSERT_GT(pid, 0);
    WaitFor(pid);
    EXPECT_EQ(g_fault_si_code.load(), FPE_INTDIV);
}

// Permanently-undefined instruction generates SIGILL; the child exits cleanly.
TEST_F(PosixSignalTest, Fault_SigIll_HandlerInvoked_ChildExitsCleanly)
{
    const pid_t pid = SpawnApp("/bin/sig_sigill_handled");
    ASSERT_GT(pid, 0);
    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.exited);
    EXPECT_EQ(r.exitStatus, 42);
}

// SIGSEGV with no installed handler takes the default action (TerminateCoreDump).
TEST_F(PosixSignalTest, Fault_SigSegv_DefaultAction_TerminatesChild_WIFSIGNALED)
{
    const pid_t pid = SpawnApp("/bin/sig_sigsegv_default");
    ASSERT_GT(pid, 0);
    const WaitResult r = WaitFor(pid);
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.signaled);
    EXPECT_EQ(r.termSig, SIGSEGV);
}

} // namespace posix_signal_tests
