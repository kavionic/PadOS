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
// Created: 20.04.2026

#include <gtest/gtest.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <atomic>
#include <chrono>

#include <System/AppDefinition.h>
#include <PadOS/Threads.h>

namespace pipe_tests
{

///////////////////////////////////////////////////////////////////////////////
// Shared globals (PadOS has no MMU — shared across forked processes)
///////////////////////////////////////////////////////////////////////////////

static std::atomic<bool> g_child_ready{false};
static std::atomic<bool> g_sigpipe_received{false};
static std::atomic<int>  g_sigpipe_count{0};

///////////////////////////////////////////////////////////////////////////////
// Signal handlers
///////////////////////////////////////////////////////////////////////////////

static void handler_sigpipe(int /*signo*/)
{
    g_sigpipe_count.fetch_add(1, std::memory_order_release);
    g_sigpipe_received.store(true, std::memory_order_release);
}

///////////////////////////////////////////////////////////////////////////////
// Test apps (cross-process coordination via shared globals, no files needed)
///////////////////////////////////////////////////////////////////////////////

// Reads up to 4096 bytes from fd given in argv[1] and writes them to fd in argv[2].
static int app_pipe_relay(int argc, char* argv[])
{
    if (argc < 3) return 1;
    const int rfd = atoi(argv[1]);
    const int wfd = atoi(argv[2]);
    char buf[4096];
    const ssize_t n = read(rfd, buf, sizeof(buf));
    if (n > 0) write(wfd, buf, n);
    return 0;
}

// Writes PIPE_BUF (4096) bytes of 'X' into fd argv[1], signals ready via atomic, then exits.
static int app_pipe_writer(int argc, char* argv[])
{
    if (argc < 2) return 1;
    const int wfd = atoi(argv[1]);
    char buf[4096];
    memset(buf, 'X', sizeof(buf));
    g_child_ready.store(true, std::memory_order_release);
    write(wfd, buf, sizeof(buf));
    return 0;
}

// Two children: each writes PIPE_BUF bytes labelled with argv[2] ('A' or 'B').
static int app_pipe_labelled_writer(int argc, char* argv[])
{
    if (argc < 3) return 1;
    const int  wfd   = atoi(argv[1]);
    const char label = argv[2][0];
    char buf[4096];
    memset(buf, label, sizeof(buf));
    write(wfd, buf, sizeof(buf));
    return 0;
}

static PAppDefinition s_app_relay          ("pipe_relay",          "", app_pipe_relay);
static PAppDefinition s_app_writer         ("pipe_writer",         "", app_pipe_writer);
static PAppDefinition s_app_labelled_writer("pipe_labelled_writer","", app_pipe_labelled_writer);

///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

static int WaitChild(pid_t pid)
{
    int status = 0;
    return (waitpid(pid, &status, 0) == pid && WIFEXITED(status))
        ? WEXITSTATUS(status) : -1;
}

static char* FDStr(char* buf, int fd) { sprintf(buf, "%d", fd); return buf; }

///////////////////////////////////////////////////////////////////////////////
// Test fixture
///////////////////////////////////////////////////////////////////////////////

class PipeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_child_ready.store(false, std::memory_order_release);
        g_sigpipe_received.store(false, std::memory_order_release);
        g_sigpipe_count.store(0, std::memory_order_release);
        signal(SIGPIPE, SIG_DFL);
    }

    void TearDown() override
    {
        signal(SIGPIPE, SIG_DFL);
    }
};

///////////////////////////////////////////////////////////////////////////////
// Tests
///////////////////////////////////////////////////////////////////////////////

TEST_F(PipeTest, BasicPipeReadWrite)
{
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    const char msg[] = "hello pipe";
    ASSERT_EQ(write(fds[1], msg, sizeof(msg)), static_cast<ssize_t>(sizeof(msg)));

    char buf[64] = {};
    ASSERT_EQ(read(fds[0], buf, sizeof(buf)), static_cast<ssize_t>(sizeof(msg)));
    EXPECT_STREQ(buf, msg);

    close(fds[0]);
    close(fds[1]);
}

TEST_F(PipeTest, PipeEOFOnWriteEndClose)
{
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    const char msg[] = "eof-test";
    write(fds[1], msg, sizeof(msg));
    close(fds[1]);  // Close write end — subsequent read should return data then 0.

    char buf[64] = {};
    ssize_t n = read(fds[0], buf, sizeof(buf));
    EXPECT_EQ(n, static_cast<ssize_t>(sizeof(msg)));
    EXPECT_STREQ(buf, msg);

    // Buffer drained and no writers — next read must return 0 (EOF).
    n = read(fds[0], buf, sizeof(buf));
    EXPECT_EQ(n, 0);

    close(fds[0]);
}

TEST_F(PipeTest, PipeNonBlockingRead_EAGAIN)
{
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    // Set O_NONBLOCK on read end.
    const int flags = fcntl(fds[0], F_GETFL, 0);
    ASSERT_NE(flags, -1);
    ASSERT_EQ(fcntl(fds[0], F_SETFL, flags | O_NONBLOCK), 0);

    char buf[16];
    const ssize_t n = read(fds[0], buf, sizeof(buf));
    EXPECT_EQ(n, -1);
    EXPECT_EQ(errno, EAGAIN);

    close(fds[0]);
    close(fds[1]);
}

TEST_F(PipeTest, PipeNonBlockingWrite_EAGAIN)
{
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    // Set O_NONBLOCK on write end.
    const int flags = fcntl(fds[1], F_GETFL, 0);
    ASSERT_NE(flags, -1);
    ASSERT_EQ(fcntl(fds[1], F_SETFL, flags | O_NONBLOCK), 0);

    // Fill the pipe buffer (4096 bytes).
    char buf[4096];
    memset(buf, 0xAB, sizeof(buf));
    const ssize_t wrote = write(fds[1], buf, sizeof(buf));
    EXPECT_EQ(wrote, 4096);

    // One more write should immediately return EAGAIN (buffer full).
    const ssize_t n = write(fds[1], buf, 1);
    EXPECT_EQ(n, -1);
    EXPECT_EQ(errno, EAGAIN);

    close(fds[0]);
    close(fds[1]);
}

TEST_F(PipeTest, PipeBufAtomicity)
{
    // A write of exactly PIPE_BUF bytes must land atomically (uninterleaved).
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    char wbuf[PIPE_BUF];
    memset(wbuf, 'Z', sizeof(wbuf));

    const ssize_t wrote = write(fds[1], wbuf, PIPE_BUF);
    EXPECT_EQ(wrote, static_cast<ssize_t>(PIPE_BUF));

    char rbuf[PIPE_BUF];
    const ssize_t n = read(fds[0], rbuf, sizeof(rbuf));
    EXPECT_EQ(n, static_cast<ssize_t>(PIPE_BUF));

    // All bytes must be 'Z' — no interleaving possible with a single writer.
    for (size_t i = 0; i < PIPE_BUF; ++i) {
        ASSERT_EQ(rbuf[i], 'Z') << "Byte " << i << " corrupted";
    }

    close(fds[0]);
    close(fds[1]);
}

TEST_F(PipeTest, SIGPIPEOnReadEndClose)
{
    signal(SIGPIPE, handler_sigpipe);

    int fds[2];
    ASSERT_EQ(pipe(fds), 0);
    close(fds[0]);  // Close read end — write should get SIGPIPE + EPIPE.

    char buf[1] = {'X'};
    const ssize_t n = write(fds[1], buf, 1);
    EXPECT_EQ(n, -1);
    EXPECT_EQ(errno, EPIPE);
    EXPECT_TRUE(g_sigpipe_received.load(std::memory_order_acquire));

    close(fds[1]);
}

TEST_F(PipeTest, EPIPEOnSIGPIPEIgnored)
{
    signal(SIGPIPE, SIG_IGN);

    int fds[2];
    ASSERT_EQ(pipe(fds), 0);
    close(fds[0]);

    char buf[1] = {'X'};
    const ssize_t n = write(fds[1], buf, 1);
    EXPECT_EQ(n, -1);
    EXPECT_EQ(errno, EPIPE);
    EXPECT_FALSE(g_sigpipe_received.load(std::memory_order_acquire));

    close(fds[1]);
}

TEST_F(PipeTest, PipeLseekFails)
{
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    EXPECT_EQ(lseek(fds[0], 0, SEEK_SET), -1);
    EXPECT_EQ(errno, ESPIPE);

    EXPECT_EQ(lseek(fds[1], 0, SEEK_SET), -1);
    EXPECT_EQ(errno, ESPIPE);

    close(fds[0]);
    close(fds[1]);
}

TEST_F(PipeTest, PipeDupSharesBuffer)
{
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    // Dup write end; close original — buffer must still accept writes.
    const int wdup = dup(fds[1]);
    ASSERT_GE(wdup, 0);
    close(fds[1]);

    const char msg[] = "dup-test";
    ASSERT_EQ(write(wdup, msg, sizeof(msg)), static_cast<ssize_t>(sizeof(msg)));

    char buf[64] = {};
    ASSERT_EQ(read(fds[0], buf, sizeof(buf)), static_cast<ssize_t>(sizeof(msg)));
    EXPECT_STREQ(buf, msg);

    // Close the dup'd write end — now reader should see EOF.
    close(wdup);
    EXPECT_EQ(read(fds[0], buf, sizeof(buf)), 0);

    close(fds[0]);
}

TEST_F(PipeTest, PipeBlockingRead)
{
    // Child writes after signalling ready; parent blocks on read until data arrives.
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    char wfdStr[16];
    FDStr(wfdStr, fds[1]);

    char arg0[] = "pipe_writer";
    char* argv[] = { arg0, wfdStr, nullptr };

    pid_t pid = -1;
    ASSERT_EQ(posix_spawn(&pid, "/bin/pipe_writer", nullptr, nullptr, argv, environ), 0);

    char buf[4096] = {};
    const ssize_t n = read(fds[0], buf, sizeof(buf));
    EXPECT_EQ(n, 4096);

    // All bytes should be 'X'.
    bool allX = true;
    for (int i = 0; i < n; ++i) {
        if (buf[i] != 'X') { allX = false; break; }
    }
    EXPECT_TRUE(allX);

    close(fds[0]);
    close(fds[1]);
    EXPECT_EQ(WaitChild(pid), 0);
}

TEST_F(PipeTest, PipeAcrossPosixSpawn)
{
    // Parent writes to pipe; child reads via dup2'd stdin and writes to dup2'd stdout.
    int in_fds[2];   // parent→child
    int out_fds[2];  // child→parent
    ASSERT_EQ(pipe(in_fds), 0);
    ASSERT_EQ(pipe(out_fds), 0);

    char rfdStr[16], wfdStr[16];
    FDStr(rfdStr, in_fds[0]);
    FDStr(wfdStr, out_fds[1]);

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, in_fds[0],  STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&actions, out_fds[1], STDOUT_FILENO);

    char arg0[] = "pipe_relay";
    char a_rfd[16], a_wfd[16];
    FDStr(a_rfd, STDIN_FILENO);
    FDStr(a_wfd, STDOUT_FILENO);
    char* argv[] = { arg0, a_rfd, a_wfd, nullptr };

    pid_t pid = -1;
    ASSERT_EQ(posix_spawn(&pid, "/bin/pipe_relay", &actions, nullptr, argv, environ), 0);
    posix_spawn_file_actions_destroy(&actions);

    // Close child-side FDs in parent to avoid blocking.
    close(in_fds[0]);
    close(out_fds[1]);

    const char msg[] = "cross-spawn";
    write(in_fds[1], msg, sizeof(msg));
    close(in_fds[1]);  // Signal EOF to child.

    char buf[64] = {};
    const ssize_t n = read(out_fds[0], buf, sizeof(buf));
    EXPECT_EQ(n, static_cast<ssize_t>(sizeof(msg)));
    EXPECT_STREQ(buf, msg);

    close(out_fds[0]);
    EXPECT_EQ(WaitChild(pid), 0);
}

} // namespace pipe_tests
