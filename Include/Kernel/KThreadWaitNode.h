// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 12.08.2022 20:00

#pragma once

#include <System/TimeValue.h>
#include "Utils/IntrusiveList.h"

namespace kernel
{
class KThreadCB;

struct KThreadWaitNode : PIntrusiveListNode<KThreadWaitNode>
{
    bool Detatch()
    {
        PIntrusiveList<KThreadWaitNode>* list = GetList();
        if (list != nullptr)
        {
            list->Remove(this);
            return true;
        }
        else
        {
            return false;
        }
    }

    TimeValNanos                    m_ResumeTime;
    KThreadCB*                      m_Thread = nullptr;
    int                             m_ReturnCode = 0;
    bool                            m_TargetDeleted = false;
};

typedef PIntrusiveList<KThreadWaitNode> KThreadWaitList;

} // namespace kernel
