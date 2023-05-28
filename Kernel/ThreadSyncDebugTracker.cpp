// This file is part of PadOS.
//
// Copyright (C) 2023 Kurt Skauen <http://kavionic.com/>
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
// Created: 02.02.2023 22:00

#include <string.h>

#include <Kernel/ThreadSyncDebugTracker.h>
#include <Kernel/KThreadCB.h>

namespace kernel
{

ThreadSyncDebugTracker ThreadSyncDebugTracker::s_Instance;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ThreadSyncDebugTracker::ThreadSyncDebugTracker()
{
    memset(m_BlockedThreads, 0, sizeof(m_BlockedThreads));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ThreadSyncDebugTracker& ThreadSyncDebugTracker::GetInstance()
{
    return s_Instance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ThreadSyncDebugTracker::AddThread(const KThreadCB* thread, const KNamedObject* waitObject)
{
    if (m_ThreadCount < MAX_TRACKED_THREADS)
    {
        m_BlockedThreads[m_ThreadCount].ThreadName = thread->GetName();
        if (waitObject != nullptr)
        {
            m_BlockedThreads[m_ThreadCount].WaitObjectName = waitObject->GetName();
            m_BlockedThreads[m_ThreadCount].WaitObjectType = waitObject->GetType();
        }
        else
        {
            m_BlockedThreads[m_ThreadCount].WaitObjectName = "sleeping";
            m_BlockedThreads[m_ThreadCount].WaitObjectType = KNamedObjectType::Generic;
        }
        m_BlockedThreads[m_ThreadCount].Thread = thread;
        m_BlockedThreads[m_ThreadCount].WaitObject = waitObject;
        m_ThreadCount++;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ThreadSyncDebugTracker::RemoveThread(const KThreadCB* thread)
{
    for (size_t i = 0; i < m_ThreadCount; ++i)
    {
        if (m_BlockedThreads[i].Thread == thread)
        {
            if (i != m_ThreadCount - 1) {
                memmove(&m_BlockedThreads[i], &m_BlockedThreads[i + 1], sizeof(m_BlockedThreads[0]) * (m_ThreadCount - i - 1));
            }
            m_ThreadCount--;
            m_BlockedThreads[m_ThreadCount].WaitObjectName = nullptr;
            m_BlockedThreads[m_ThreadCount].WaitObjectType = KNamedObjectType::Generic;
            m_BlockedThreads[m_ThreadCount].Thread = nullptr;
            m_BlockedThreads[m_ThreadCount].WaitObject = nullptr;
            break;
        }
    }
}

//void ThreadSyncDebugTracker::DetectDeadlock(const KThreadCB* thread, const KNamedObject* waitObject)
//{
//    for (const KThreadWaitNode* waitNode = waitObject->GetWaitQueue().m_First; waitObject != nullptr; waitNode = waitNode->m_Next)
//    {
//        if (waitNode->m_TargetDeleted) continue;
//
//        const KThreadCB* otherThread = waitNode->m_Thread;
//        KNamedObject* otherBlocker = otherThread->GetBlockingObject();
//        if (otherBlocker != nullptr && otherBlocker->GetHolder)
//
//    for (size_t i = 0; i < m_ThreadCount; ++i)
//    {
//        if (m_BlockedThreads[i].Thread == thread)
//        {
//
//}


} // namespace kernel
