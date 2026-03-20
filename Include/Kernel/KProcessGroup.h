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
// Created: 15.03.2026 20:00

#pragma once

#include <sys/types.h>
#include <Ptr/PtrTarget.h>

namespace kernel
{

class KProcess;
class KProcessSession;

class KProcessGroup : public PtrTarget
{
public:
    KProcessGroup(pid_t id, Ptr<KProcessSession> session);

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
