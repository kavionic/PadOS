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

#include <System/TimeValue.h>
#include "Utils/IntrusiveList.h"

namespace kernel
{
class KThreadCB;

struct KThreadWaitNode
{
    bool Detatch()
    {
        if (m_List != nullptr) {
            m_List->Remove(this);
            return true;
        } else {
            return false;
        }
    }

    TimeValNanos                    m_ResumeTime;
    KThreadCB*                      m_Thread = nullptr;
    int                             m_ReturnCode = 0;
    bool                            m_TargetDeleted = false;
    KThreadWaitNode*                m_Next = nullptr;
    KThreadWaitNode*                m_Prev = nullptr;
    PIntrusiveList<KThreadWaitNode>* m_List = nullptr;
};

typedef PIntrusiveList<KThreadWaitNode> KThreadWaitList;

} // namespace kernel
