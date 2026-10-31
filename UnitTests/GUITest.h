// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <functional>
#include <utility>

#include <gtest/gtest.h>
#include <App/Application.h>
#include <Threads/Threads.h>

// These integration tests require the running appserver and bundled fonts.
class PGUITestApplication : public PApplication
{
public:
    explicit PGUITestApplication(std::function<void(PApplication&)> test)
        : PApplication("gui_test")
        , m_Test(std::move(test))
    {
        SetDeleteOnExit(false);
    }

    virtual void* Run() override
    {
        CRITICAL_SCOPE(GetMutex());
        EXPECT_NO_THROW(m_Test(*this));
        Flush();
        return nullptr;
    }

private:
    std::function<void(PApplication&)> m_Test;
};

inline void RunGUITest(std::function<void(PApplication&)> test)
{
    Ptr<PGUITestApplication> application = ptr_new<PGUITestApplication>(std::move(test));
    ASSERT_EQ(application->Start(PThreadDetachState_Joinable, 0, 16384), PErrorCode::Success);
    ASSERT_EQ(application->Join(nullptr), PErrorCode::Success);
}
