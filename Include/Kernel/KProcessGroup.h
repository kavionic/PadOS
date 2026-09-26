// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 15.03.2026 20:00

#pragma once

#include <sys/types.h>
#include <vector>

#include <Ptr/Ptr.h>
#include <Ptr/PtrTarget.h>

namespace kernel
{

class KProcess;
class KProcessSession;

class KProcessGroup : public PtrTarget
{
public:
    KProcessGroup(pid_t id, Ptr<KProcessSession> session);
    ~KProcessGroup();

    pid_t GetID() const noexcept { return m_GroupID; }

    bool IsOrphaned() const noexcept;

    Ptr<KProcessSession> GetSession() const noexcept;

    void ReserveSpace();
    void AddProcess(KProcess* process);
    void RemoveProcess(KProcess* process);

    const std::vector<KProcess*>& GetProcessList() const noexcept;

private:
    pid_t                   m_GroupID = -1;
    Ptr<KProcessSession>    m_Session;
    std::vector<KProcess*>  m_Processes;
};


} // namespace kernel
