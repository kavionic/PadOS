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


#include <Kernel/KWaitableObject.h>
#include <Kernel/KObjectWaitGroup.h>

namespace kernel
{

KWaitableObject::~KWaitableObject()
{
    while (!m_WaitGroups.empty())
    {
        m_WaitGroups.back().first->RemoveObject(this, m_WaitGroups.back().second);
    }
}

bool KWaitableObject::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    m_WaitQueue.Append(waitNode);
    return true;
}


} // namespace kernel
