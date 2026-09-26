// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 12.08.2022 20:00


#include <Kernel/KWaitableObject.h>
#include <Kernel/KObjectWaitGroup.h>

namespace kernel
{

KWaitableObject::~KWaitableObject()
{
    try
    {
        while (!m_WaitGroups.empty())
        {
            m_WaitGroups.back().first->RemoveObject_trw(this, m_WaitGroups.back().second);
        }
    }
    catch(...)
    {
        return;
    }
}

bool KWaitableObject::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    m_WaitQueue.Append(waitNode);
    return true;
}


} // namespace kernel
