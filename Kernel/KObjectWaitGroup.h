/*
 * KObjectWaitGroup.h
 *
 *  Created on: Jan 24, 2020
 *      Author: kurts
 */

#pragma once
#include <vector>

#include "KNamedObject.h"
#include "Ptr/Ptr.h"

namespace kernel
{
class KMutex;

class KObjectWaitGroup : public KNamedObject
{
public:
	KObjectWaitGroup(const char* name);

	bool AddObject(Ptr<KNamedObject> object);
	bool SetObjects(const std::vector<Ptr<KNamedObject>>& objects);
	bool AppendObjects(const std::vector<Ptr<KNamedObject>>& objects);
	bool RemoveObject(Ptr<KNamedObject> object);
	void Clear();

	bool Wait() { return Wait(nullptr, 0); }
	bool WaitTimeout(bigtime_t timeout) { return Wait(nullptr, get_system_time() + timeout); }
	bool WaitDeadline(bigtime_t deadline) { return Wait(nullptr, deadline); }

	bool Wait(KMutex& lock) { return Wait(&lock, 0); }
	bool WaitTimeout(KMutex& lock, bigtime_t timeout) { return Wait(&lock, get_system_time() + timeout); }
	bool WaitDeadline(KMutex& lock, bigtime_t deadline) { return Wait(&lock, deadline); }

private:
	bool Wait(KMutex* lock, bigtime_t deadline);

	std::vector<Ptr<KNamedObject>> m_Objects;
	std::vector<KThreadWaitNode>   m_WaitNodes;
	KThreadWaitNode				   m_SleepNode;
	bool                           m_IsWaiting = false;
};


} // namespace
