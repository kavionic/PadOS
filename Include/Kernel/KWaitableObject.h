// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 12.08.2022 20:00

#pragma once

#include <vector>
#include <PadOS/ObjectWaitGroup.h>
#include <Kernel/KThreadWaitNode.h>


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
