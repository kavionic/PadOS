/*
 * KObjectWaitGroup.cpp
 *
 *  Created on: Jan 24, 2020
 *      Author: kurts
 */

#include "KObjectWaitGroup.h"

#include "KMutex.h"
#include "Scheduler.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KObjectWaitGroup::KObjectWaitGroup(const char* name) : KNamedObject(name, KNamedObjectType::ObjectWaitGroup)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::AddObject(Ptr<KNamedObject> object)
{
	try {
		m_Objects.push_back(object);
		return true;
	} catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::SetObjects(const std::vector<Ptr<KNamedObject>>& objects)
{
	try {
		m_Objects = objects;
		return true;
	} catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::AppendObjects(const std::vector<Ptr<KNamedObject>>& objects)
{
	try {
		m_Objects.reserve(m_Objects.size() + objects.size());
		m_Objects.insert(m_Objects.end(), objects.begin(), objects.end());
		return true;
	} catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::RemoveObject(Ptr<KNamedObject> object)
{
	auto i = std::find(m_Objects.begin(), m_Objects.end(), object);
	if (i != m_Objects.end()) {
		m_Objects.erase(i);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::Clear()
{
	m_Objects.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::Wait(KMutex* lock, bigtime_t deadline)
{
    KThreadCB* thread = gk_CurrentThread;

	try {
		m_WaitNodes.resize(m_Objects.size());
	} catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }

//    for (;;)
    {
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
        	for (int i = 0; i < m_Objects.size(); ++i)
        	{
                m_WaitNodes[i].m_Thread = thread;
                m_Objects[i]->GetWaitQueue().Append(&m_WaitNodes[i]);
        	}
        	if (deadline != 0)
        	{
                thread->m_State = KThreadState::Sleeping;

                m_SleepNode.m_Thread     = thread;
                m_SleepNode.m_ResumeTime = deadline;
                add_to_sleep_list(&m_SleepNode);
        	}
        	else
        	{
        		thread->m_State = KThreadState::Waiting;
        	}
        	thread->m_BlockingObject = this;
            if (lock != nullptr) lock->Unlock();
        	m_IsWaiting = true;
            KSWITCH_CONTEXT(); // Make sure we are suspended the moment we re-enable interrupts
        } CRITICAL_END;
        if (lock != nullptr) lock->Lock();
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            m_SleepNode.Detatch();
        	for (int i = 0; i < m_Objects.size(); ++i)
        	{
        		KThreadWaitNode& waitNode = m_WaitNodes[i];

        		if (waitNode.m_TargetDeleted) {
					continue;
				}
				waitNode.Detatch();
        	}
        	m_IsWaiting = false;
        	thread->m_BlockingObject = nullptr;
        } CRITICAL_END;
    }
    return true;
}
