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


#include <Kernel/KProcessGroup.h>
#include <Kernel/KPIDNode.h>
#include <Kernel/KProcess.h>
#include <Kernel/KProcessSession.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcessGroup::KProcessGroup(pid_t id, Ptr<KProcessSession> session)
    : m_GroupID(id)
    , m_Session(session)
{
    session->AddGroup(ptr_tmp_cast(this));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcessGroup::~KProcessGroup() = default;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcessGroup::ReserveSpace()
{
    kassert(g_PIDMapMutex.IsLocked());

    if (m_Processes.capacity() == m_Processes.size()) {
        m_Processes.reserve((m_Processes.capacity() == 0) ? 1 : m_Processes.capacity() * 2);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KProcessGroup::IsOrphaned() const noexcept
{
    kassert(g_PIDMapMutex.IsLocked());

    for (KProcess* process : m_Processes)
    {
        Ptr<KProcess> parent = process->GetParent_pl();
        if (parent != nullptr && parent->GetSession() == m_Session && parent->GetGroup() != this) {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KProcessSession> KProcessGroup::GetSession() const noexcept
{
    return m_Session;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcessGroup::AddProcess(KProcess* process)
{
    kassert(gk_CurrentThread == nullptr || g_PIDMapMutex.IsLocked());

    kassert(process->GetGroup() == nullptr);

    process->SetGroup(ptr_tmp_cast(this));
    m_Processes.push_back(process);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcessGroup::RemoveProcess(KProcess* process)
{
    kassert(g_PIDMapMutex.IsLocked());

    kassert(process->GetGroup() == this);

    auto it = std::find(m_Processes.begin(), m_Processes.end(), process);
    if (kensure(it != m_Processes.end()))
    {
        process->SetGroup(nullptr);
        m_Processes.erase(it);

        if (m_Processes.empty() && m_Session != nullptr) {
            m_Session->RemoveGroup(ptr_tmp_cast(this));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const std::vector<KProcess*>& KProcessGroup::GetProcessList() const noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    
    return m_Processes;
}

} // namespace kernel
