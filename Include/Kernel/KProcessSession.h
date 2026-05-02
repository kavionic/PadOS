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
#include <vector>

#include <Ptr/Ptr.h>
#include <Ptr/PtrTarget.h>


namespace kernel
{

class KInode;
class KProcessGroup;

class KProcessSession : public PtrTarget
{
public:
    KProcessSession(pid_t id);
    ~KProcessSession();

    pid_t GetID() const noexcept { return m_SessionID; }

    void AddGroup(Ptr<KProcessGroup> group);
    void RemoveGroup(Ptr<KProcessGroup> group);

    void                SetForegroundGroup(Ptr<KProcessGroup> group) noexcept;
    Ptr<KProcessGroup>  GetForegroundGroup() const noexcept;

    void SetControllingTTY(Ptr<KInode> inode);
    Ptr<KInode> GetControllingTTY() const;

    const std::vector<Ptr<KProcessGroup>>& GetGroupList() const noexcept;
private:
    pid_t                           m_SessionID = -1;
    Ptr<KInode>                     m_ControllingTTY;
    Ptr<KProcessGroup>              m_ForegroundGroup;
    std::vector<Ptr<KProcessGroup>> m_Groups;
};


} // namespace kernel
