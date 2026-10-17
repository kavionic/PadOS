// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <errno.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <System/AppDefinition.h>
#include <System/ExceptionHandling.h>

namespace credential_tests
{

static int CheckSecureEnvironment(int argumentCount, char* arguments[])
{
    if (argumentCount != 2) {
        return 1;
    }
    const bool expectSecure = strcmp(arguments[1], "secure") == 0;
    if ((issetugid() != 0) != expectSecure) {
        return 2;
    }
    if (getuid() != 0 || getgid() != 0 || geteuid() != (expectSecure ? 1001 : 0) || getegid() != (expectSecure ? 2001 : 0)) {
        return 3;
    }

    static constexpr char VARIABLE_NAME[] = "PADOS_TEST_SECURE_GETENV";
    if (setenv(VARIABLE_NAME, "present", 1) != 0) {
        return 4;
    }
    PScopeExit restoreEnvironment([]() { unsetenv(VARIABLE_NAME); });
    if (getenv(VARIABLE_NAME) == nullptr) {
        return 5;
    }
    const char* value = secure_getenv(VARIABLE_NAME);
    if ((value == nullptr) != expectSecure) {
        return 6;
    }
    if (value != nullptr && strcmp(value, "present") != 0) {
        return 7;
    }

    if (seteuid(0) != 0 || setegid(0) != 0) {
        return 8;
    }
    if ((issetugid() != 0) != expectSecure || (secure_getenv(VARIABLE_NAME) == nullptr) != expectSecure) {
        return 9;
    }
    return 0;
}

static PAppDefinition SECURE_ENVIRONMENT_APP("credentials_secure_environment", "", CheckSecureEnvironment);

static int SpawnAndWait(char* mode, const posix_spawnattr_t* attributes)
{
    char path[] = "/bin/credentials_secure_environment";
    char* arguments[] = { path, mode, nullptr };
    pid_t processID = -1;
    if (posix_spawn(&processID, path, nullptr, attributes, arguments, environ) != 0) {
        return -1;
    }
    int status = 0;
    if (waitpid(processID, &status, 0) != processID || !WIFEXITED(status)) {
        return -1;
    }
    return WEXITSTATUS(status);
}

TEST(Credentials, EffectiveIDsAndSecureEnvironmentUseTheCorrectCredentials)
{
    if (getuid() != 0 || getgid() != 0 || geteuid() != 0 || getegid() != 0 || issetugid() != 0) {
        GTEST_SKIP() << "Requires a process started with matching root credentials.";
    }
    PScopeExit restoreCredentials([]() { EXPECT_EQ(seteuid(0), 0); EXPECT_EQ(setegid(0), 0); });

    ASSERT_EQ(setegid(2001), 0);
    ASSERT_EQ(seteuid(1001), 0);
    EXPECT_EQ(getuid(), 0);
    EXPECT_EQ(getgid(), 0);
    EXPECT_EQ(geteuid(), 1001);
    EXPECT_EQ(getegid(), 2001);
    EXPECT_EQ(issetugid(), 0);

    EXPECT_EQ(seteuid(1002), -1);
    EXPECT_EQ(errno, EPERM);
    EXPECT_EQ(setegid(2002), -1);
    EXPECT_EQ(errno, EPERM);
    EXPECT_EQ(seteuid(uid_t(-1)), -1);
    EXPECT_EQ(errno, EINVAL);
    EXPECT_EQ(setegid(gid_t(-1)), -1);
    EXPECT_EQ(errno, EINVAL);

    char secureMode[] = "secure";
    EXPECT_EQ(SpawnAndWait(secureMode, nullptr), 0);

    posix_spawnattr_t attributes;
    ASSERT_EQ(posix_spawnattr_init(&attributes), 0);
    PScopeExit destroyAttributes([&attributes]() { posix_spawnattr_destroy(&attributes); });
    ASSERT_EQ(posix_spawnattr_setflags(&attributes, POSIX_SPAWN_RESETIDS), 0);
    char normalMode[] = "normal";
    EXPECT_EQ(SpawnAndWait(normalMode, &attributes), 0);
}

} // namespace credential_tests
