// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include <deque>
#include <set>
#include <vector>

#include <DeviceControl/InputDevice.h>
#include <GUI/GUIEvent.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/KMutex.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KInode.h>
#include <RPC/RPCDispatcher.h>


namespace kernel
{

class KInputDeviceInode : public KInode, public KFilesystemFileOps
{
public:
    static constexpr size_t DEFAULT_MAX_QUEUED_EVENTS = 256;
    static constexpr size_t MAX_EVENT_SIZE = 4096;

    KInputDeviceInode(PInputClass classID, size_t maxQueuedEvents = DEFAULT_MAX_QUEUED_EVENTS);

    PInputClass GetClassID() const { return m_ClassID; }

    void AddSource(int32_t sourceID);
    void RemoveSource(int32_t sourceID);
    virtual void AddEvent(const PInputEvent& event);

    virtual size_t Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual void   ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;
    virtual void   DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

protected:
    using EventBuffer = std::vector<uint8_t>;

    void AddEventBuffer(EventBuffer&& event);

    virtual void QueueEvent_pl(EventBuffer&& event);

    EventBuffer CreateEventBuffer(const PInputEvent& event) const;
    PInputEvent GetEventHeader(const EventBuffer& event) const;
    virtual void ValidateEvent(const PInputEvent& event, size_t availableSize) const;

    PInputClass             m_ClassID;
    size_t                  m_MaxQueuedEvents;
    mutable KMutex          m_Mutex;
    KConditionVariable      m_ReadCondition;
    std::deque<EventBuffer> m_EventQueue;

private:
    static const PInputEvent& GetInputEventHeader(const EventBuffer& event);
    size_t GetRegisteredDevices(PInputDeviceInfo* devices, size_t maxDeviceCount) const;

    std::set<int32_t> m_SourceIDs;
    PRPCDispatcher    m_DeviceControlDispatcher;

    KInputDeviceInode(const KInputDeviceInode&) = delete;
    KInputDeviceInode& operator=(const KInputDeviceInode&) = delete;
};

class KInputMotionDeviceInode : public KInputDeviceInode
{
public:
    KInputMotionDeviceInode(PInputClass classID, size_t maxQueuedEvents = DEFAULT_MAX_QUEUED_EVENTS);

protected:
    virtual void QueueEvent_pl(EventBuffer&& event) override;
    virtual void ValidateEvent(const PInputEvent& event, size_t availableSize) const override;

private:
    using EventQueueIterator = std::deque<EventBuffer>::iterator;

    EventQueueIterator FindCoalescableEvent_pl(const EventBuffer& event);

    static const PMouseEvent& GetMouseEvent(const EventBuffer& event);
    static PMouseEvent& GetMouseEvent(EventBuffer& event);
    static const PTouchEvent& GetTouchEvent(const EventBuffer& event);
    static void CoalesceMouseMoveEvent(EventBuffer& queuedEvent, const EventBuffer& event);

    static bool IsMoveEvent(PInputEventID eventID);
    bool IsMatchingMoveEvent(const EventBuffer& lhs, const EventBuffer& rhs) const;
};

} // namespace kernel
