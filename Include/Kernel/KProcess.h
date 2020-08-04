// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 11.03.2018 01:23:13

#pragma once

#include "Ptr/PtrTarget.h"
#include "Threads/Threads.h"
#include "KThreadCB.h"
#include "VFS/KIOContext.h"

namespace kernel
{

struct KTLSNode
{
    TLSDestructor_t m_Destructor;
};

class KProcess : public PtrTarget
{
public:
    KProcess();
    ~KProcess();

    void ThreadQuit(KThreadCB* thread);

    int  AllocTLSSlot(TLSDestructor_t destructor);
    bool FreeTLSSlot(int slot);

    KIOContext* GetIOContext() { return &m_IOContext; }

private:
    TLSDestructor_t m_TLSDestructors[THREAD_MAX_TLS_SLOTS];
    uint32_t        m_TLSAllocationMap[(THREAD_MAX_TLS_SLOTS + 31) / 32];

    KIOContext m_IOContext;

    KProcess(const KProcess &) = delete;
    KProcess& operator=(const KProcess &) = delete;
};

} // namespace
