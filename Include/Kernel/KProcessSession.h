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
