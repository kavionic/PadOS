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
// Created: 12.04.2026

#include <gtest/gtest.h>

#include <spawn.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <chrono>
#include <string>

#include <System/AppDefinition.h>

// All registered apps become entries in the /bin/ virtual filesystem
// automatically - no executable files need to be created manually.

namespace posix_spawn_tests
{

///////////////////////////////////////////////////////////////////////////////
// Test application implementations
///////////////////////////////////////////////////////////////////////////////

// Exits with the integer value from argv[1], or 0 if not given.
static int app_exit_code(int argc, char* argv[])
{
    return (argc >= 2) ? atoi(argv[1]) : 0;
}

// Writes argv[1..argc-1] to stdout, space-separated, then a newline.
static int app_echo_args(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1) write(STDOUT_FILENO, " ", 1);
        write(STDOUT_FILENO, argv[i], strlen(argv[i]));
    }
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}

// Checks whether the fd given in argv[1] is open; writes "open" or "closed".
static int app_check_fd(int argc, char* argv[])
{
    const int fd = (argc >= 2) ? atoi(argv[1]) : 3;
    stat_t statBuf;
    const int flags = fstat(fd, &statBuf);
    const char* msg = (flags != -1) ? "open" : "closed";
    write(STDOUT_FILENO, msg, strlen(msg));
    return 0;
}

// Writes the current working directory to stdout.
static int app_print_cwd(int argc, char* argv[])
{
    char cwd[512] = {};
    if (getcwd(cwd, sizeof(cwd)))
    {
        write(STDOUT_FILENO, cwd, strlen(cwd));
        return 0;
    }
    return 1;
}

// Writes "masked" if SIGUSR1 is in the process signal mask, otherwise "unmasked".
static int app_check_sigmask(int argc, char* argv[])
{
    sigset_t mask;
    sigemptyset(&mask);
    thread_sigmask(SIG_BLOCK, nullptr, &mask);
    const char* msg = sigismember(&mask, SIGUSR1) ? "masked" : "unmasked";
    write(STDOUT_FILENO, msg, strlen(msg));
    return 0;
}

// Writes the process group ID to stdout.
static int app_print_pgrp(int argc, char* argv[])
{
    char buf[32];
    const int n = snprintf(buf, sizeof(buf), "%d", static_cast<int>(getpgrp()));
    write(STDOUT_FILENO, buf, n);
    return 0;
}

// Reads up to 512 bytes from stdin and writes them verbatim to stdout.
static int app_stdin_to_stdout(int argc, char* argv[])
{
    char buf[512];
    const ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n > 0) write(STDOUT_FILENO, buf, n);
    return 0;
}

// Writes "default" if SIGUSR1 disposition is SIG_DFL, otherwise "custom".
static int app_check_sigdef(int argc, char* argv[])
{
    struct sigaction sa{};
    sigaction(SIGUSR1, nullptr, &sa);
    const char* msg = (sa.sa_handler == SIG_DFL) ? "default" : "custom";
    write(STDOUT_FILENO, msg, strlen(msg));
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// App registrations — each entry is automatically visible as /bin/<name>
///////////////////////////////////////////////////////////////////////////////

static PAppDefinition s_app_exit_code    ("ps_test_exit_code",     "", app_exit_code);
static PAppDefinition s_app_echo_args    ("ps_test_echo_args",     "", app_echo_args);
static PAppDefinition s_app_check_fd     ("ps_test_check_fd",      "", app_check_fd);
static PAppDefinition s_app_print_cwd    ("ps_test_print_cwd",     "", app_print_cwd);
static PAppDefinition s_app_check_sigmask("ps_test_check_sigmask", "", app_check_sigmask);
static PAppDefinition s_app_print_pgrp   ("ps_test_print_pgrp",    "", app_print_pgrp);
static PAppDefinition s_app_stdin_stdout ("ps_test_stdin_stdout",  "", app_stdin_to_stdout);
static PAppDefinition s_app_check_sigdef ("ps_test_check_sigdef",  "", app_check_sigdef);

///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

// Waits for child 'pid' and returns its exit status code, or -1 on error.
static int WaitChild(pid_t pid)
{
    int status = 0;
    return (waitpid(pid, &status, 0) == pid && WIFEXITED(status))
        ? WEXITSTATUS(status) : -1;
}

// Reads the entire content of 'path' into a std::string.
static std::string ReadFile(const char* path)
{
    const int fd = open(path, O_RDONLY);
    if (fd == -1) return {};
    char buf[4096];
    const ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n > 0) { buf[n] = '\0'; return buf; }
    return {};
}

// Spawns 'path' with 'argv' and 'attr', redirecting child stdout to a temp
// capture file in /tmp (FAT — no permission bits needed, just data).
// Returns the child's exit code, captured output text, and spawned PID.
struct SpawnResult { int exitCode = -1; std::string output; pid_t pid = -1; };

static SpawnResult SpawnCapture(const char* path, char* const argv[],
                                posix_spawnattr_t* attr = nullptr)
{
    static constexpr const char* kCapFile = "/tmp/.ps_cap";

    const int capFD = open(kCapFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (capFD == -1) return {};

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, capFD, STDOUT_FILENO);

    pid_t pid = -1;
    const int ret = posix_spawn(&pid, path, &actions, attr, argv, environ);
    close(capFD);
    posix_spawn_file_actions_destroy(&actions);

    if (ret != 0) return { ret, {}, -1 };

    SpawnResult result;
    result.pid      = pid;
    result.exitCode = WaitChild(pid);
    result.output   = ReadFile(kCapFile);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// Test fixture — only responsible for cleaning up /tmp data files
///////////////////////////////////////////////////////////////////////////////

class PosixSpawnTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        unlink("/tmp/.ps_cap");
        unlink("/tmp/.ps_addclose_dummy");
        unlink("/tmp/.ps_addopen_in");

        // Resolve the canonical path of /tmp, which may differ from "/tmp" if
        // it is a symlink or virtual alias (getcwd returns the real path).
        const int savedFD = open(".", O_RDONLY);
        if (savedFD != -1 && chdir("/tmp") == 0)
        {
            char buf[512] = {};
            if (getcwd(buf, sizeof(buf))) {
                m_TmpPath = buf;
            }
            fchdir(savedFD);
            close(savedFD);
        }
        if (m_TmpPath.empty()) {
            m_TmpPath = "/tmp";
        }
    }

    void TearDown() override
    {
        unlink("/tmp/.ps_cap");
        unlink("/tmp/.ps_addclose_dummy");
        unlink("/tmp/.ps_addopen_in");
    }

    std::string m_TmpPath;
};

///////////////////////////////////////////////////////////////////////////////
// Basic spawn tests
///////////////////////////////////////////////////////////////////////////////

TEST_F(PosixSpawnTest, BasicSpawn_ExitZero)
{
    char arg0[] = "ps_exit_code", arg1[] = "0";
    char* argv[] = { arg0, arg1, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_exit_code", nullptr, nullptr, argv, environ), 0);
    EXPECT_EQ(WaitChild(pid), 0);
}

TEST_F(PosixSpawnTest, BasicSpawn_NonZeroExitCode)
{
    char arg0[] = "ps_exit_code", arg1[] = "73";
    char* argv[] = { arg0, arg1, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_exit_code", nullptr, nullptr, argv, environ), 0);
    EXPECT_EQ(WaitChild(pid), 73);
}

TEST_F(PosixSpawnTest, BasicSpawn_ReturnedPidIsPositive)
{
    char arg0[] = "ps_exit_code";
    char* argv[] = { arg0, nullptr };
    pid_t pid = -1;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_exit_code", nullptr, nullptr, argv, environ), 0);
    EXPECT_GT(pid, 0);
    WaitChild(pid);
}

TEST_F(PosixSpawnTest, BasicSpawn_ArgvPassing_SingleArg)
{
    char arg0[] = "ps_echo_args", arg1[] = "hello";
    char* argv[] = { arg0, arg1, nullptr };
    auto r = SpawnCapture("/bin/ps_test_echo_args", argv);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_EQ(r.output, "hello\n");
}

TEST_F(PosixSpawnTest, BasicSpawn_ArgvPassing_MultipleArgs)
{
    char arg0[] = "ps_echo_args", arg1[] = "foo", arg2[] = "bar", arg3[] = "baz";
    char* argv[] = { arg0, arg1, arg2, arg3, nullptr };
    auto r = SpawnCapture("/bin/ps_test_echo_args", argv);
    EXPECT_EQ(r.exitCode, 0);
    EXPECT_EQ(r.output, "foo bar baz\n");
}

///////////////////////////////////////////////////////////////////////////////
// Error cases
///////////////////////////////////////////////////////////////////////////////

TEST_F(PosixSpawnTest, Error_AppNotFound)
{
    // No PAppDefinition is registered under this name, so /bin lookup fails.
    char arg0[] = "ps_test_app_that_does_not_exist";
    char* argv[] = { arg0, nullptr };
    pid_t pid;
    EXPECT_NE(posix_spawn(&pid, "/bin/ps_test_app_that_does_not_exist", nullptr, nullptr, argv, environ), 0);
}

///////////////////////////////////////////////////////////////////////////////
// posix_spawn_file_actions — lifecycle
///////////////////////////////////////////////////////////////////////////////

TEST_F(PosixSpawnTest, FileActions_InitDestroy)
{
    posix_spawn_file_actions_t actions;
    EXPECT_EQ(posix_spawn_file_actions_init(&actions), 0);
    EXPECT_NE(actions, nullptr);
    EXPECT_EQ(posix_spawn_file_actions_destroy(&actions), 0);
}

TEST_F(PosixSpawnTest, FileActions_AddMultipleOps_NoError)
{
    posix_spawn_file_actions_t actions;
    ASSERT_EQ(posix_spawn_file_actions_init(&actions), 0);
    EXPECT_EQ(posix_spawn_file_actions_addclose(&actions, 42), 0);
    EXPECT_EQ(posix_spawn_file_actions_addclose(&actions, 43), 0);
    EXPECT_EQ(posix_spawn_file_actions_adddup2(&actions, 5, 6), 0);
    EXPECT_EQ(posix_spawn_file_actions_addopen(&actions, 7, "/tmp", O_RDONLY, 0), 0);
    EXPECT_EQ(posix_spawn_file_actions_addchdir(&actions, "/tmp"), 0);
    posix_spawn_file_actions_destroy(&actions);
}

///////////////////////////////////////////////////////////////////////////////
// posix_spawn_file_actions — behavioural
///////////////////////////////////////////////////////////////////////////////

TEST_F(PosixSpawnTest, FileActions_AddDup2_RedirectsChildStdout)
{
    const char* capPath = "/tmp/.ps_cap";
    const int capFD = open(capPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ASSERT_NE(capFD, -1);

    posix_spawn_file_actions_t actions;
    ASSERT_EQ(posix_spawn_file_actions_init(&actions), 0);
    ASSERT_EQ(posix_spawn_file_actions_adddup2(&actions, capFD, STDOUT_FILENO), 0);

    char arg0[] = "ps_echo_args", arg1[] = "dup2_works";
    char* argv[] = { arg0, arg1, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_echo_args", &actions, nullptr, argv, environ), 0);
    close(capFD);
    posix_spawn_file_actions_destroy(&actions);

    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_EQ(ReadFile(capPath), "dup2_works\n");
}

TEST_F(PosixSpawnTest, FileActions_AddClose_FdIsClosedInChild)
{
    // Open a dummy file in the parent; the child should see it as closed.
    const int extraFD = open("/tmp/.ps_addclose_dummy", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ASSERT_NE(extraFD, -1);

    const char* capPath = "/tmp/.ps_cap";
    const int capFD = open(capPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ASSERT_NE(capFD, -1);

    posix_spawn_file_actions_t actions;
    ASSERT_EQ(posix_spawn_file_actions_init(&actions), 0);
    ASSERT_EQ(posix_spawn_file_actions_addclose(&actions, extraFD), 0);
    ASSERT_EQ(posix_spawn_file_actions_adddup2(&actions, capFD, STDOUT_FILENO), 0);

    char arg0[] = "ps_check_fd";
    char fdArg[16];
    snprintf(fdArg, sizeof(fdArg), "%d", extraFD);
    char* argv[] = { arg0, fdArg, nullptr };

    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_check_fd", &actions, nullptr, argv, environ), 0);
    close(extraFD);
    close(capFD);
    posix_spawn_file_actions_destroy(&actions);

    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_EQ(ReadFile(capPath), "closed");
}

TEST_F(PosixSpawnTest, FileActions_AddOpen_ChildCanReadFile)
{
    // Write content to a file, then use addopen to attach it as stdin in child.
    const char* inputPath = "/tmp/.ps_addopen_in";
    {
        const int fd = open(inputPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        ASSERT_NE(fd, -1);
        write(fd, "addopen_content", 15);
        close(fd);
    }

    const char* capPath = "/tmp/.ps_cap";
    const int capFD = open(capPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ASSERT_NE(capFD, -1);

    posix_spawn_file_actions_t actions;
    ASSERT_EQ(posix_spawn_file_actions_init(&actions), 0);
    ASSERT_EQ(posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, inputPath, O_RDONLY, 0), 0);
    ASSERT_EQ(posix_spawn_file_actions_adddup2(&actions, capFD, STDOUT_FILENO), 0);

    char arg0[] = "ps_stdin_stdout";
    char* argv[] = { arg0, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_stdin_stdout", &actions, nullptr, argv, environ), 0);
    close(capFD);
    posix_spawn_file_actions_destroy(&actions);

    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_EQ(ReadFile(capPath), "addopen_content");
}

TEST_F(PosixSpawnTest, FileActions_AddChdir_ChildHasNewCwd)
{
    const char* capPath = "/tmp/.ps_cap";
    const int capFD = open(capPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ASSERT_NE(capFD, -1);

    posix_spawn_file_actions_t actions;
    ASSERT_EQ(posix_spawn_file_actions_init(&actions), 0);
    ASSERT_EQ(posix_spawn_file_actions_addchdir(&actions, "/tmp"), 0);
    ASSERT_EQ(posix_spawn_file_actions_adddup2(&actions, capFD, STDOUT_FILENO), 0);

    char arg0[] = "ps_print_cwd";
    char* argv[] = { arg0, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_print_cwd", &actions, nullptr, argv, environ), 0);
    close(capFD);
    posix_spawn_file_actions_destroy(&actions);

    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_EQ(ReadFile(capPath), m_TmpPath);
}

TEST_F(PosixSpawnTest, FileActions_AddChdirNp_ChildHasNewCwd)
{
    const char* capPath = "/tmp/.ps_cap";
    const int capFD = open(capPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ASSERT_NE(capFD, -1);

    posix_spawn_file_actions_t actions;
    ASSERT_EQ(posix_spawn_file_actions_init(&actions), 0);
    ASSERT_EQ(posix_spawn_file_actions_addchdir_np(&actions, "/tmp"), 0);
    ASSERT_EQ(posix_spawn_file_actions_adddup2(&actions, capFD, STDOUT_FILENO), 0);

    char arg0[] = "ps_print_cwd";
    char* argv[] = { arg0, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_print_cwd", &actions, nullptr, argv, environ), 0);
    close(capFD);
    posix_spawn_file_actions_destroy(&actions);

    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_EQ(ReadFile(capPath), m_TmpPath);
}

TEST_F(PosixSpawnTest, FileActions_AddFchdir_ChildHasNewCwd)
{
    const int dirFD = open("/tmp", O_RDONLY);
    ASSERT_NE(dirFD, -1);

    const char* capPath = "/tmp/.ps_cap";
    const int capFD = open(capPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ASSERT_NE(capFD, -1);

    posix_spawn_file_actions_t actions;
    ASSERT_EQ(posix_spawn_file_actions_init(&actions), 0);
    ASSERT_EQ(posix_spawn_file_actions_addfchdir(&actions, dirFD), 0);
    ASSERT_EQ(posix_spawn_file_actions_adddup2(&actions, capFD, STDOUT_FILENO), 0);

    char arg0[] = "ps_print_cwd";
    char* argv[] = { arg0, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_print_cwd", &actions, nullptr, argv, environ), 0);
    close(dirFD);
    close(capFD);
    posix_spawn_file_actions_destroy(&actions);

    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_EQ(ReadFile(capPath), m_TmpPath);
}

TEST_F(PosixSpawnTest, FileActions_AddFchdirNp_ChildHasNewCwd)
{
    const int dirFD = open("/tmp", O_RDONLY);
    ASSERT_NE(dirFD, -1);

    const char* capPath = "/tmp/.ps_cap";
    const int capFD = open(capPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ASSERT_NE(capFD, -1);

    posix_spawn_file_actions_t actions;
    ASSERT_EQ(posix_spawn_file_actions_init(&actions), 0);
    ASSERT_EQ(posix_spawn_file_actions_addfchdir_np(&actions, dirFD), 0);
    ASSERT_EQ(posix_spawn_file_actions_adddup2(&actions, capFD, STDOUT_FILENO), 0);

    char arg0[] = "ps_print_cwd";
    char* argv[] = { arg0, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_print_cwd", &actions, nullptr, argv, environ), 0);
    close(dirFD);
    close(capFD);
    posix_spawn_file_actions_destroy(&actions);

    EXPECT_EQ(WaitChild(pid), 0);
    EXPECT_EQ(ReadFile(capPath), m_TmpPath);
}

///////////////////////////////////////////////////////////////////////////////
// posix_spawnattr — getter/setter roundtrips
///////////////////////////////////////////////////////////////////////////////

TEST_F(PosixSpawnTest, SpawnAttr_InitDestroy)
{
    posix_spawnattr_t attr;
    EXPECT_EQ(posix_spawnattr_init(&attr), 0);
    EXPECT_NE(attr, nullptr);
    EXPECT_EQ(posix_spawnattr_destroy(&attr), 0);
}

TEST_F(PosixSpawnTest, SpawnAttr_GetSetFlags)
{
    posix_spawnattr_t attr;
    ASSERT_EQ(posix_spawnattr_init(&attr), 0);

    EXPECT_EQ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF), 0);
    short flags = 0;
    EXPECT_EQ(posix_spawnattr_getflags(&attr, &flags), 0);
    EXPECT_EQ(flags, short(POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF));

    posix_spawnattr_destroy(&attr);
}

TEST_F(PosixSpawnTest, SpawnAttr_GetSetPGroup)
{
    posix_spawnattr_t attr;
    ASSERT_EQ(posix_spawnattr_init(&attr), 0);

    EXPECT_EQ(posix_spawnattr_setpgroup(&attr, 1234), 0);
    pid_t pgrp = 0;
    EXPECT_EQ(posix_spawnattr_getpgroup(&attr, &pgrp), 0);
    EXPECT_EQ(pgrp, 1234);

    posix_spawnattr_destroy(&attr);
}

TEST_F(PosixSpawnTest, SpawnAttr_GetSetSigMask)
{
    posix_spawnattr_t attr;
    ASSERT_EQ(posix_spawnattr_init(&attr), 0);

    sigset_t set, out;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    EXPECT_EQ(posix_spawnattr_setsigmask(&attr, &set), 0);
    sigemptyset(&out);
    EXPECT_EQ(posix_spawnattr_getsigmask(&attr, &out), 0);
    EXPECT_TRUE(sigismember(&out, SIGUSR1));
    EXPECT_TRUE(sigismember(&out, SIGUSR2));
    EXPECT_FALSE(sigismember(&out, SIGTERM));

    posix_spawnattr_destroy(&attr);
}

TEST_F(PosixSpawnTest, SpawnAttr_GetSetSigDefault)
{
    posix_spawnattr_t attr;
    ASSERT_EQ(posix_spawnattr_init(&attr), 0);

    sigset_t set, out;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    EXPECT_EQ(posix_spawnattr_setsigdefault(&attr, &set), 0);
    sigemptyset(&out);
    EXPECT_EQ(posix_spawnattr_getsigdefault(&attr, &out), 0);
    EXPECT_TRUE(sigismember(&out, SIGUSR1));
    EXPECT_FALSE(sigismember(&out, SIGTERM));

    posix_spawnattr_destroy(&attr);
}

TEST_F(PosixSpawnTest, SpawnAttr_GetSetSchedParam)
{
    posix_spawnattr_t attr;
    ASSERT_EQ(posix_spawnattr_init(&attr), 0);

    struct sched_param sp{};
    sp.sched_priority = 5;
    EXPECT_EQ(posix_spawnattr_setschedparam(&attr, &sp), 0);
    struct sched_param out{};
    EXPECT_EQ(posix_spawnattr_getschedparam(&attr, &out), 0);
    EXPECT_EQ(out.sched_priority, 5);

    posix_spawnattr_destroy(&attr);
}

TEST_F(PosixSpawnTest, SpawnAttr_GetSetSchedPolicy)
{
    posix_spawnattr_t attr;
    ASSERT_EQ(posix_spawnattr_init(&attr), 0);

    EXPECT_EQ(posix_spawnattr_setschedpolicy(&attr, SCHED_RR), 0);
    int policy = 0;
    EXPECT_EQ(posix_spawnattr_getschedpolicy(&attr, &policy), 0);
    EXPECT_EQ(policy, SCHED_RR);

    posix_spawnattr_destroy(&attr);
}

///////////////////////////////////////////////////////////////////////////////
// posix_spawnattr — behavioural
///////////////////////////////////////////////////////////////////////////////

TEST_F(PosixSpawnTest, SpawnAttr_SetPGroup_ChildGetsOwnProcessGroup)
{
    posix_spawnattr_t attr;
    ASSERT_EQ(posix_spawnattr_init(&attr), 0);
    ASSERT_EQ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP), 0);
    ASSERT_EQ(posix_spawnattr_setpgroup(&attr, 0), 0);  // 0 = use child's own PID

    char arg0[] = "ps_print_pgrp";
    char* argv[] = { arg0, nullptr };
    auto r = SpawnCapture("/bin/ps_test_print_pgrp", argv, &attr);
    posix_spawnattr_destroy(&attr);

    ASSERT_EQ(r.exitCode, 0);
    const pid_t childPgrp = static_cast<pid_t>(atoi(r.output.c_str()));
    // With pgroup=0 the child becomes its own group leader (pgrp == child PID).
    EXPECT_EQ(childPgrp, r.pid);
    EXPECT_NE(childPgrp, getpgrp());
}

TEST_F(PosixSpawnTest, SpawnAttr_SetSigMask_ChildHasMaskedSignal)
{
    // Ensure SIGUSR1 is NOT blocked in the parent so the result is unambiguous.
    sigset_t unblock;
    sigemptyset(&unblock);
    sigaddset(&unblock, SIGUSR1);
    thread_sigmask(SIG_UNBLOCK, &unblock, nullptr);

    posix_spawnattr_t attr;
    ASSERT_EQ(posix_spawnattr_init(&attr), 0);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    ASSERT_EQ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGMASK), 0);
    ASSERT_EQ(posix_spawnattr_setsigmask(&attr, &mask), 0);

    char arg0[] = "ps_check_sigmask";
    char* argv[] = { arg0, nullptr };
    auto r = SpawnCapture("/bin/ps_test_check_sigmask", argv, &attr);
    posix_spawnattr_destroy(&attr);

    EXPECT_EQ(r.exitCode, 0);
    EXPECT_EQ(r.output, "masked");
}

TEST_F(PosixSpawnTest, SpawnAttr_SetSigMask_NotSet_ChildInheritsParentMask)
{
    // Block SIGUSR1 in parent; child should inherit it when SETSIGMASK is not set.
    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    sigset_t old;
    thread_sigmask(SIG_BLOCK, &block, &old);

    char arg0[] = "ps_check_sigmask";
    char* argv[] = { arg0, nullptr };
    auto r = SpawnCapture("/bin/ps_test_check_sigmask", argv);

    thread_sigmask(SIG_SETMASK, &old, nullptr);  // Restore parent mask.

    EXPECT_EQ(r.exitCode, 0);
    EXPECT_EQ(r.output, "masked");
}

TEST_F(PosixSpawnTest, SpawnAttr_SetSigDef_ResetsHandlerToDefault)
{
    // Install a no-op handler for SIGUSR1; the child should reset it to SIG_DFL.
    struct sigaction sa{};
    sa.sa_handler = [](int) {};
    sigaction(SIGUSR1, &sa, nullptr);

    posix_spawnattr_t attr;
    ASSERT_EQ(posix_spawnattr_init(&attr), 0);
    sigset_t def;
    sigemptyset(&def);
    sigaddset(&def, SIGUSR1);
    ASSERT_EQ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGDEF), 0);
    ASSERT_EQ(posix_spawnattr_setsigdefault(&attr, &def), 0);

    char arg0[] = "ps_check_sigdef";
    char* argv[] = { arg0, nullptr };
    auto r = SpawnCapture("/bin/ps_test_check_sigdef", argv, &attr);
    posix_spawnattr_destroy(&attr);

    sa.sa_handler = SIG_DFL;
    sigaction(SIGUSR1, &sa, nullptr);  // Restore parent handler.

    EXPECT_EQ(r.exitCode, 0);
    EXPECT_EQ(r.output, "default");
}

TEST_F(PosixSpawnTest, SpawnAttr_SigDefNotSet_ChildInheritsCustomHandler)
{
    // Without SETSIGDEF the child inherits the custom handler from the parent.
    struct sigaction sa{};
    sa.sa_handler = [](int) {};
    sigaction(SIGUSR1, &sa, nullptr);

    char arg0[] = "ps_check_sigdef";
    char* argv[] = { arg0, nullptr };
    auto r = SpawnCapture("/bin/ps_test_check_sigdef", argv);

    sa.sa_handler = SIG_DFL;
    sigaction(SIGUSR1, &sa, nullptr);

    EXPECT_EQ(r.exitCode, 0);
    EXPECT_EQ(r.output, "custom");
}

///////////////////////////////////////////////////////////////////////////////
// posix_spawnp
///////////////////////////////////////////////////////////////////////////////

TEST_F(PosixSpawnTest, SpawnP_WorksLikeSpawn)
{
    // On PadOS posix_spawnp delegates directly to posix_spawn (no PATH search).
    char arg0[] = "ps_exit_code", arg1[] = "7";
    char* argv[] = { arg0, arg1, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawnp(&pid, "/bin/ps_test_exit_code", nullptr, nullptr, argv, environ), 0);
    EXPECT_EQ(WaitChild(pid), 7);
}

///////////////////////////////////////////////////////////////////////////////
// PTY — child output captured via PTY master
///////////////////////////////////////////////////////////////////////////////

TEST_F(PosixSpawnTest, PTY_ChildStdoutViaMasterPTY)
{
    // Find an unused PTY slot.
    int masterFD = -1;
    int ptyIndex = -1;
    char masterPath[64], slavePath[64];

    for (int i = 0; i < 256 && masterFD == -1; ++i)
    {
        snprintf(masterPath, sizeof(masterPath), "/dev/pty/master/pty%d", i);
        struct stat st;
        if (stat(masterPath, &st) == -1)
        {
            masterFD = open(masterPath, O_RDWR | O_CREAT | O_NONBLOCK);
            if (masterFD >= 0) ptyIndex = i;
        }
    }
    ASSERT_NE(masterFD, -1) << "No PTY slot available";

    snprintf(slavePath, sizeof(slavePath), "/dev/pty/slave/pty%d", ptyIndex);
    const int slaveFD = open(slavePath, O_RDWR);
    ASSERT_NE(slaveFD, -1);

    posix_spawn_file_actions_t actions;
    ASSERT_EQ(posix_spawn_file_actions_init(&actions), 0);
    ASSERT_EQ(posix_spawn_file_actions_adddup2(&actions, slaveFD, STDOUT_FILENO), 0);
    ASSERT_EQ(posix_spawn_file_actions_addclose(&actions, masterFD), 0);

    char arg0[] = "ps_echo_args", arg1[] = "via_pty";
    char* argv[] = { arg0, arg1, nullptr };
    pid_t pid;
    ASSERT_EQ(posix_spawn(&pid, "/bin/ps_test_echo_args", &actions, nullptr, argv, environ), 0);

    close(slaveFD);  // Parent no longer needs the slave end.
    posix_spawn_file_actions_destroy(&actions);

    EXPECT_EQ(WaitChild(pid), 0);

    // Data written by the child is now in the master buffer; poll briefly.
    char buf[128] = {};
    ssize_t total = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (total < static_cast<ssize_t>(sizeof(buf) - 1) &&
           std::chrono::steady_clock::now() < deadline)
    {
        const ssize_t n = read(masterFD, buf + total, sizeof(buf) - 1 - total);
        if (n > 0) { total += n; break; }
        if (n == 0 || (n < 0 && errno != EAGAIN)) break;
        usleep(1000);
    }
    close(masterFD);

    ASSERT_GT(total, 0);
    EXPECT_STREQ(buf, "via_pty\r\n");
}

} // namespace posix_spawn_tests
