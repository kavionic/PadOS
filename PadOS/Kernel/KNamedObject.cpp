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
// Created: 08.03.2018 17:55:02

#include "Platform.h"

#include <string.h>

#include "KNamedObject.h"
#include "KHandleArray.h"

using namespace kernel;

static uint8_t gk_NamedObjectsTableBuffer[sizeof(KHandleArray<KNamedObject>)];
static KHandleArray<KNamedObject>& gk_NamedObjectsTable = *reinterpret_cast<KHandleArray<KNamedObject>*>(gk_NamedObjectsTableBuffer);

void KNamedObject::InitializeStatics()
{
    new ((void*)gk_NamedObjectsTableBuffer) KHandleArray<KNamedObject>();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KNamedObject::KNamedObject(const char* name, KNamedObjectType type) : m_Type(type)
{
    strncpy(m_Name, name, OS_NAME_LENGTH - 1);
    m_Name[OS_NAME_LENGTH - 1] = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KNamedObject::~KNamedObject()
{
    int ourPriLevel = gk_CurrentThread->m_PriorityLevel;
    bool needSchedule = false;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        for (KThreadWaitNode* waitNode = m_WaitQueue.m_First; waitNode != nullptr; waitNode = m_WaitQueue.m_First)
        {
            waitNode->m_TargetDeleted = true;
            KThreadCB* thread = waitNode->m_Thread;
            if (thread != nullptr && (thread->m_State == KThreadState::Sleeping || thread->m_State == KThreadState::Waiting)) {
                if (thread->m_PriorityLevel > ourPriLevel) needSchedule = true;
                add_thread_to_ready_list(thread);
            }
            m_WaitQueue.Remove(waitNode);
        }
    } CRITICAL_END;
    if (needSchedule) {
        KSWITCH_CONTEXT();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool kernel::KNamedObject::DebugValidate() const
{
    if (m_Magic != MAGIC) { panic("KnamedObject has been overwritten!\n"); return false; } return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t KNamedObject::RegisterObject(Ptr<KNamedObject> object)
{
    int32_t handle = gk_NamedObjectsTable.AllocHandle();
    if (handle == -1) {
        return -1;
    }
    object->SetHandle(handle);
    gk_NamedObjectsTable.Set(handle, object);
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KNamedObject::FreeHandle(int32_t handle, KNamedObjectType type)
{
    if (GetObject(handle, type) != nullptr) {
        return gk_NamedObjectsTable.FreeHandle(handle);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KNamedObject> KNamedObject::GetObject(int32_t handle, KNamedObjectType type)
{
    Ptr<KNamedObject> object = gk_NamedObjectsTable.Get(handle);

    if (object != nullptr && object->GetType() == type) {
        return object;
    } else {
        return nullptr;
    }
}

