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

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcessSession::AddGroup(Ptr<KProcessGroup> group)
{
    kassert(g_PIDMapMutex.IsLocked());
    kassert(group->GetSession() == this);

    m_Groups.push_back(group);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcessSession::RemoveGroup(Ptr<KProcessGroup> group)
{
    kassert(g_PIDMapMutex.IsLocked());
    kassert(group->GetSession() == this);

    if (group == m_ForegroundGroup) {
        m_ForegroundGroup = nullptr;
    }

    auto it = std::find(m_Groups.begin(), m_Groups.end(), group);
    if (kensure(it != m_Groups.end()))
    {
        m_Groups.erase(it);

        if (m_Groups.empty()) {
            kerase_pid_node_if_empty_pl(m_SessionID);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcessSession::SetForegroundGroup(Ptr<KProcessGroup> group) noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    kassert(group == nullptr || std::find(m_Groups.begin(), m_Groups.end(), group) != m_Groups.end());

    m_ForegroundGroup = group;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<kernel::KProcessGroup> KProcessSession::GetForegroundGroup() const noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    return m_ForegroundGroup;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcessSession::SetControllingTTY(Ptr<KInode> inode)
{
    kassert(g_PIDMapMutex.IsLocked());
    m_ControllingTTY = inode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<kernel::KInode> KProcessSession::GetControllingTTY() const
{
    kassert(g_PIDMapMutex.IsLocked());
    return m_ControllingTTY;
}


} // namespace kernel
