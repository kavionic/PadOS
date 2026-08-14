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

#pragma once

#include <vector>

#include <sys/pados_threads.h>
#include <sys/pados_types.h>

#include <Ptr/Ptr.h>
#include <System/TimeValue.h>
#include <Utils/String.h>


namespace shutil_top
{

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
    int Run(TimeValNanos period);

private:
    void insert_thread(ThreadInfo* threadInfo);
    void print_list();
    void update_list();

    std::vector<Ptr<TopThreadInfo>> m_ThreadList;
    uint32_t                        m_RunNumber = 1;
};

int top_main(int argc, char** argv);

} // namespace shutil_top
