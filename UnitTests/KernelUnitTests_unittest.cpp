// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <utility>

#include <gtest/gtest.h>
#include <sys/pados_syscalls.h>

TEST(KernelTests, All)
{
    const PErrorCode kernelUnitTestResult = run_kernel_unit_tests();
    EXPECT_EQ(std::to_underlying(kernelUnitTestResult), std::to_underlying(PErrorCode::Success));
}
