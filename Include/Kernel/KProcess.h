// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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

#include <sys/pados_syscalls.h>

#include <Ptr/PtrTarget.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/KMutex.h>
#include <Kernel/VFS/KIOContext.h>
#include <Threads/Threads.h>

namespace kernel
{

class KProcess : public PtrTarget
{
public:
    KProcess();
    ~KProcess();

    KIOContext* GetIOContext() { return &m_IOContext; }

private:
    KIOContext m_IOContext;

    KProcess(const KProcess &) = delete;
    KProcess& operator=(const KProcess &) = delete;
};

} // namespace
