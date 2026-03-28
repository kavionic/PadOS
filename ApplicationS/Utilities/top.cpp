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
// Created: 12.03.2026 22:00


#include <termios.h>
#include <sys/pados_types.h>
#include <sys/pados_threads.h>

#include <System/AppDefinition.h>

#include <PadOS/Filesystem.h>
#include <Threads/Threads.h>
#include <Threads/Thread.h>


namespace shutil_top
{

#define ANSI_CLEAR_TO_END_OF_LINE   "\033[K"
#define ANSI_CLEAR_TO_END_OF_SCREEN "\033[J"
#define ANSI_CLEAR_SCREEN           "\033[2J"
#define ANSI_CURSOR_TOP_LEFT        "\033[H"
#define ANSI_HIDE_CURSOR            "\033[?25l"
#define ANSI_SHOW_CURSOR            "\033[?25h"
#define ANSI_ENABLE_ALT_SCR_BUFFER  "\033[?1049h"
#define ANSI_DISABLE_ALT_SCR_BUFFER "\033[?1049l"


struct TopThreadInfo : public PtrTarget
{
    thread_id       ThreadID;
    pid_t           ProcessID;
    TimeValNanos    ThisTime;
    TimeValNanos    LastTime;
    PString         ThreadName;
    PString         ProcName;
    int             Priority;
    uint32_t        RunNumber;
};


class CmdTop : public PtrTarget
{
public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    int Run(TimeValNanos period)
    {
        printf(ANSI_CLEAR_SCREEN ANSI_CURSOR_TOP_LEFT "Wait for initial update\n");
        update_list();

        for(;;)
        {
            snooze(period);
            update_list();
            print_list();
        }
        return 0;
    }

private:

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void insert_thread(ThreadInfo* psInfo)
    {
        for (size_t i = 0; i < m_ThreadList.size(); ++i)
        {
            if (psInfo->ThreadID == m_ThreadList[i]->ThreadID)
            {
                m_ThreadList[i]->LastTime = m_ThreadList[i]->ThisTime;
                m_ThreadList[i]->ThisTime = TimeValNanos::FromNanoseconds(psInfo->UserTimeNano);
                m_ThreadList[i]->RunNumber = m_RunNumber;

                m_ThreadList[i]->Priority = psInfo->Priority;
                m_ThreadList[i]->ThreadName = psInfo->ThreadName;
                m_ThreadList[i]->ProcName = psInfo->ProcessName;
                return;
            }
        }
        Ptr<TopThreadInfo> threadInfo = ptr_new<TopThreadInfo>();
        
        threadInfo->ThreadID    = psInfo->ThreadID;
        threadInfo->ProcessID   = psInfo->ProcessID;
        threadInfo->ThisTime    = TimeValNanos::FromNanoseconds(psInfo->UserTimeNano);
        threadInfo->LastTime    = TimeValNanos::FromNanoseconds(psInfo->UserTimeNano);
        threadInfo->Priority    = psInfo->Priority;
        threadInfo->RunNumber   = m_RunNumber;

        threadInfo->ThreadName  = psInfo->ThreadName;
        threadInfo->ProcName    = psInfo->ProcessName;

        m_ThreadList.push_back(threadInfo);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void print_list()
    {
        struct winsize winSize;
        TimeValNanos totalTime;

        device_control(0, TIOCGWINSZ, nullptr, 0, &winSize, sizeof(winSize));

        const size_t lineCount = std::min<size_t>(m_ThreadList.size(), winSize.ws_row);

        for (size_t i = 0; i < lineCount; ++i)
        {
            const Ptr<TopThreadInfo>& info = m_ThreadList[i];
            totalTime += info->ThisTime - info->LastTime;
        }

        printf(ANSI_CURSOR_TOP_LEFT);

        double totalTimeInverse = 1.0f / totalTime.AsSeconds();
        for (size_t i = 0; i < lineCount; ++i)
        {
            const Ptr<TopThreadInfo>& psInfo = m_ThreadList[i];
            TimeValNanos deltaTime = psInfo->ThisTime - psInfo->LastTime;

            PString text = PString::format_string("{:<15.15} {:<20.20} ({:04}:{:04}) {:4}, {:.3f}ms {:5.1f}%",
                psInfo->ProcName, psInfo->ThreadName,
                psInfo->ProcessID, psInfo->ThreadID,
                psInfo->Priority,
                deltaTime.AsSeconds() * 0.001, deltaTime.AsSeconds() * 100.0 * totalTimeInverse
            );
            if (text.size() > winSize.ws_col) text.resize(winSize.ws_col);

            if (i == 0) {
                printf("%s", text.c_str());
            } else {
                printf("\n%s", text.c_str());
            }
            if (text.size() < winSize.ws_col) {
                printf(ANSI_CLEAR_TO_END_OF_LINE);
            }
        }
        if (lineCount < winSize.ws_row) {
            printf("\n" ANSI_CLEAR_TO_END_OF_SCREEN);
        }
        fflush(stdout);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void update_list()
    {
        ThreadInfo threadInfo;

        for (PErrorCode result = get_thread_info(-1, &threadInfo); result == PErrorCode::Success; result = get_next_thread_info(&threadInfo))
        {
            insert_thread(&threadInfo);
        }
        std::sort(m_ThreadList.begin(), m_ThreadList.end(), [](const Ptr<TopThreadInfo>& psI1, const Ptr<TopThreadInfo>& psI2)
            {
                if (psI1->RunNumber != psI2->RunNumber) {
                    return psI2->RunNumber < psI1->RunNumber;
                }
                return (psI2->ThisTime - psI2->LastTime) < (psI1->ThisTime - psI1->LastTime);
            }
        );

        for (ssize_t i = m_ThreadList.size() - 1; i >= 0; --i)
        {
            if (m_ThreadList[i]->RunNumber == m_RunNumber)
            {
                m_ThreadList.resize(i + 1);
                break;
            }
        }
        m_RunNumber++;
    }

    std::vector<Ptr<TopThreadInfo>> m_ThreadList;
    uint32_t m_RunNumber = 1;
};

int top_main(int argc, char** argv)
{
    TimeValNanos period = TimeValNanos::FromSeconds(5.0);

    if (argc == 2) {
        period = TimeValNanos::FromSeconds(atof(argv[1]));
    }


    try
    {
        Ptr<CmdTop> top = ptr_new<CmdTop>();

        printf(ANSI_ENABLE_ALT_SCR_BUFFER ANSI_HIDE_CURSOR ANSI_CURSOR_TOP_LEFT);

        PScopeExit scopeExit([]()
            {
                printf("\n" ANSI_SHOW_CURSOR ANSI_DISABLE_ALT_SCR_BUFFER);
                fflush(stdout);
            }
        );

        return top->Run(period);
    }
    catch(const std::exception& exc)
    {
        return 0;
    }
}

static PAppDefinition g_TopAppDef("top", "Show process/thread information.", top_main);

} //namespace shutil_top
