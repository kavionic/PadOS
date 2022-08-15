// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 12.08.2022 20:00

#pragma once

#include <vector>
#include <Kernel/KThreadWaitNode.h>

enum class ObjectWaitMode : uint8_t
{
    Read,
    Write,
    ReadWrite
};

namespace kernel
{
class KObjectWaitGroup;

class KWaitableObject
{
public:
    virtual ~KWaitableObject();

    KThreadWaitList& GetWaitQueue() { return m_WaitQueue; }

    // If access would block, add to wait-list and return true. If not, don't add to any list and return false.
    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode);

protected:
    KThreadWaitList m_WaitQueue;

private:
    friend class KObjectWaitGroup;

    std::vector<std::pair<KObjectWaitGroup*, ObjectWaitMode>> m_WaitGroups;
};


} // namespace kernel
