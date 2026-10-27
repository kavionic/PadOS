// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <cerrno>
#include <cstdint>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include <Threads/Mutex.h>
#include <Threads/ObjectWaitGroup.h>
#include <Utils/MessagePort.h>

class ObjectWaitGroupReadyFlagsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_NE(m_WaitGroup.GetHandle(), INVALID_HANDLE);
        m_Ports.reserve(9);
        for (size_t i = 0; i < 9; ++i)
        {
            m_Ports.emplace_back("wait_flags_test", 1);
            ASSERT_NE(m_Ports.back().GetHandle(), INVALID_HANDLE);
            ASSERT_TRUE(m_WaitGroup.AddObject(m_Ports.back()));
        }
        m_Buffer.fill(GUARD_VALUE);
    }

    void ExpectBuffer(uint8_t firstFlags, uint8_t secondFlags)
    {
        EXPECT_EQ(m_Buffer[0], GUARD_VALUE);
        EXPECT_EQ(m_Buffer[1], firstFlags);
        EXPECT_EQ(m_Buffer[2], secondFlags);
        for (size_t i = 3; i < m_Buffer.size(); ++i) {
            EXPECT_EQ(m_Buffer[i], GUARD_VALUE) << i;
        }
    }

    static constexpr uint8_t GUARD_VALUE = 0xa5;

    std::vector<PMessagePort> m_Ports;
    PObjectWaitGroup m_WaitGroup{"wait_flags_test"};
    alignas(uint32_t) std::array<uint8_t, 24> m_Buffer;
};

TEST_F(ObjectWaitGroupReadyFlagsTest, ReportsAllReadyObjectsInUnalignedByteBuffer)
{
    ASSERT_TRUE(m_Ports[0].SendMessage(INVALID_HANDLE, 1, nullptr, 0));
    ASSERT_TRUE(m_Ports[7].SendMessage(INVALID_HANDLE, 1, nullptr, 0));
    ASSERT_TRUE(m_Ports[8].SendMessage(INVALID_HANDLE, 1, nullptr, 0));

    ASSERT_TRUE(m_WaitGroup.WaitTimeout(TimeValNanos::zero, m_Buffer.data() + 1, 2));
    ExpectBuffer(0x81, 0x01);
}

TEST_F(ObjectWaitGroupReadyFlagsTest, ClearsFlagsOnTimeoutWithoutTouchingAdjacentBytes)
{
    EXPECT_FALSE(m_WaitGroup.WaitTimeout(TimeValNanos::zero, m_Buffer.data() + 1, 2));
    EXPECT_EQ(get_last_error(), ETIMEDOUT);
    ExpectBuffer(0, 0);
}

TEST_F(ObjectWaitGroupReadyFlagsTest, TruncatedBufferStillWakesForAnUnreportedObject)
{
    ASSERT_TRUE(m_Ports[8].SendMessage(INVALID_HANDLE, 1, nullptr, 0));

    ASSERT_TRUE(m_WaitGroup.WaitTimeout(TimeValNanos::zero, m_Buffer.data() + 1, 1));
    ExpectBuffer(0, GUARD_VALUE);
}

TEST_F(ObjectWaitGroupReadyFlagsTest, ZeroLengthBufferIsNotModified)
{
    ASSERT_TRUE(m_Ports[0].SendMessage(INVALID_HANDLE, 1, nullptr, 0));

    ASSERT_TRUE(m_WaitGroup.WaitTimeout(TimeValNanos::zero, m_Buffer.data() + 1, 0));
    ExpectBuffer(GUARD_VALUE, GUARD_VALUE);
}

TEST_F(ObjectWaitGroupReadyFlagsTest, ReportsReadinessAfterBlocking)
{
    PMutex mutex("wait_flags_test", PEMutexRecursionMode_RaiseError);
    ASSERT_NE(mutex.GetHandle(), INVALID_HANDLE);
    PMessagePort& readyPort = m_Ports[8];
    std::thread sender;
    bool sent = false;
    bool ready = false;
    {
        CRITICAL_SCOPE(mutex);
        sender = std::thread([&mutex, &readyPort, &sent]
        {
            CRITICAL_SCOPE(mutex);
            sent = readyPort.SendMessage(INVALID_HANDLE, 1, nullptr, 0);
        });
        ready = m_WaitGroup.WaitTimeout(mutex, TimeValNanos::FromSeconds(1.0), m_Buffer.data() + 1, 2);
    }
    sender.join();

    EXPECT_TRUE(sent);
    EXPECT_TRUE(ready);
    ExpectBuffer(0, 0x01);
}
