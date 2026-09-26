// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
