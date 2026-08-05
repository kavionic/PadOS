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

#include <algorithm>
#include <fcntl.h>
#include <limits>
#include <string.h>

#include <Kernel/KAddressValidation.h>
#include <Kernel/Kernel.h>
#include <Kernel/UserInput/InputDeviceInode.h>
#include <Kernel/VFS/KFileHandle.h>
#include <System/ExceptionHandling.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KInputDeviceInode::KInputDeviceInode(PInputClass classID, size_t maxQueuedEvents)
    : KInode(nullptr, nullptr, this, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
    , m_ClassID(classID)
    , m_MaxQueuedEvents(std::max<size_t>(1, maxQueuedEvents))
    , m_Mutex("input_device", PEMutexRecursionMode_RaiseError)
    , m_ReadCondition("input_device_read")
{
    m_DeviceControlDispatcher.AddHandler(&PInputDeviceControl::GetRegisteredDevices, this, &KInputDeviceInode::GetRegisteredDevices);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputDeviceInode::AddSource(int32_t sourceID)
{
    const PInputEvent event
    {
        .EventSize = sizeof(PInputEvent),
        .EventType = PInputEventType::DeviceEvent,
        .ClassID = m_ClassID,
        .Timestamp = kget_monotonic_time(),
        .EventID = PInputEventID::DeviceAdded,
        .SourceID = sourceID
    };
    EventBuffer eventBuffer = CreateEventBuffer(event);

    KScopedLock lock(m_Mutex);

    const bool inserted = m_SourceIDs.insert(sourceID).second;
    if (!inserted) {
        PERROR_THROW_CODE(PErrorCode::EXIST);
    }

    const bool wasEmpty = m_EventQueue.empty();
    QueueEvent_pl(std::move(eventBuffer));
    if (wasEmpty && !m_EventQueue.empty()) {
        m_ReadCondition.WakeupAll();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputDeviceInode::RemoveSource(int32_t sourceID)
{
    const PInputEvent event
    {
        .EventSize = sizeof(PInputEvent),
        .EventType = PInputEventType::DeviceEvent,
        .ClassID = m_ClassID,
        .Timestamp = kget_monotonic_time(),
        .EventID = PInputEventID::DeviceRemoved,
        .SourceID = sourceID
    };
    EventBuffer eventBuffer = CreateEventBuffer(event);

    KScopedLock lock(m_Mutex);

    const auto sourceIterator = m_SourceIDs.find(sourceID);
    if (sourceIterator != m_SourceIDs.end())
    {
        m_SourceIDs.erase(sourceIterator);

        const bool wasEmpty = m_EventQueue.empty();
        QueueEvent_pl(std::move(eventBuffer));
        if (wasEmpty && !m_EventQueue.empty()) {
            m_ReadCondition.WakeupAll();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputDeviceInode::AddEvent(const PInputEvent& event)
{
    ValidateEvent(event, event.EventSize);
    AddEventBuffer(CreateEventBuffer(event));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KInputDeviceInode::Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t /*position*/)
{
    if (length == 0) {
        return 0;
    }
    if (buffer == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    KScopedLock lock(m_Mutex);

    while (m_EventQueue.empty())
    {
        if (file->GetOpenFlags() & O_NONBLOCK) {
            PERROR_THROW_CODE(PErrorCode::WOULDBLOCK);
        }
        const PErrorCode result = m_ReadCondition.WaitCancelable(m_Mutex);
        if (result != PErrorCode::Success) {
            PERROR_THROW_CODE(result);
        }
    }

    const EventBuffer& queuedEvent = m_EventQueue.front();
    if (length < queuedEvent.size()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    EventBuffer event = std::move(m_EventQueue.front());
    m_EventQueue.pop_front();

    memcpy(buffer, event.data(), event.size());
    return event.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KInputDeviceInode::Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t /*position*/)
{
    if (length == 0) {
        return 0;
    }
    if (buffer == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    const uint8_t* currentEvent = static_cast<const uint8_t*>(buffer);
    size_t remainingLength = length;

    while (remainingLength > 0)
    {
        if (remainingLength < sizeof(PInputEvent)) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }

        PInputEvent eventHeader;
        memcpy(&eventHeader, currentEvent, sizeof(eventHeader));
        if (eventHeader.EventSize > remainingLength) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        ValidateEvent(eventHeader, remainingLength);

        EventBuffer event(eventHeader.EventSize);
        memcpy(event.data(), currentEvent, event.size());
        AddEventBuffer(std::move(event));

        currentEvent += eventHeader.EventSize;
        remainingLength -= eventHeader.EventSize;
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputDeviceInode::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);

    KScopedLock lock(m_Mutex);

    size_t queuedBytes = 0;
    for (const EventBuffer& event : m_EventQueue) {
        queuedBytes += event.size();
    }
    statBuf->st_size = static_cast<off_t>(queuedBytes);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputDeviceInode::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    m_DeviceControlDispatcher.Dispatch(request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KInputDeviceInode::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    switch (mode)
    {
        case ObjectWaitMode::Read:
        case ObjectWaitMode::ReadWrite:
            if (m_EventQueue.empty()) {
                return m_ReadCondition.AddListener(waitNode, ObjectWaitMode::Read);
            } else {
                return false;
            }
        case ObjectWaitMode::Write:
            return false;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputDeviceInode::AddEventBuffer(EventBuffer&& event)
{
    ValidateEvent(GetEventHeader(event), event.size());

    KScopedLock lock(m_Mutex);

    const bool wasEmpty = m_EventQueue.empty();
    QueueEvent_pl(std::move(event));
    if (wasEmpty && !m_EventQueue.empty()) {
        m_ReadCondition.WakeupAll();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputDeviceInode::QueueEvent_pl(EventBuffer&& event)
{
    if (m_EventQueue.size() >= m_MaxQueuedEvents) {
        m_EventQueue.pop_front();
    }
    m_EventQueue.push_back(std::move(event));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KInputDeviceInode::EventBuffer KInputDeviceInode::CreateEventBuffer(const PInputEvent& event) const
{
    EventBuffer eventBuffer(event.EventSize);
    memcpy(eventBuffer.data(), &event, eventBuffer.size());
    return eventBuffer;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PInputEvent KInputDeviceInode::GetEventHeader(const EventBuffer& event) const
{
    if (event.size() < sizeof(PInputEvent)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return GetInputEventHeader(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputDeviceInode::ValidateEvent(const PInputEvent& event, size_t availableSize) const
{
    if (event.EventSize < sizeof(PInputEvent) || event.EventSize > MAX_EVENT_SIZE || event.EventSize > availableSize) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (event.ClassID != m_ClassID) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PInputEvent& KInputDeviceInode::GetInputEventHeader(const EventBuffer& event)
{
    return *reinterpret_cast<const PInputEvent*>(event.data());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KInputDeviceInode::GetRegisteredDevices(PInputDeviceInfo* devices, size_t maxDeviceCount) const
{
    if (maxDeviceCount > std::numeric_limits<size_t>::max() / sizeof(PInputDeviceInfo)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    validate_user_write_pointer_trw(devices, maxDeviceCount * sizeof(PInputDeviceInfo));

    KScopedLock lock(m_Mutex);

    size_t deviceIndex = 0;
    for (const int32_t sourceID : m_SourceIDs)
    {
        if (deviceIndex >= maxDeviceCount) {
            break;
        }
        devices[deviceIndex] = PInputDeviceInfo{
            .ClassID = m_ClassID,
            .SourceID = sourceID
        };
        ++deviceIndex;
    }
    return m_SourceIDs.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KInputMotionDeviceInode::KInputMotionDeviceInode(PInputClass classID, size_t maxQueuedEvents)
    : KInputDeviceInode(classID, maxQueuedEvents)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputMotionDeviceInode::QueueEvent_pl(EventBuffer&& event)
{
    const PInputEvent eventHeader = GetEventHeader(event);

    if (m_EventQueue.size() >= m_MaxQueuedEvents && IsMoveEvent(eventHeader.EventID))
    {
        const EventQueueIterator coalescableEvent = FindCoalescableEvent_pl(event);
        if (coalescableEvent != m_EventQueue.end())
        {
            if (eventHeader.ClassID == PInputClass::Mouse && eventHeader.EventID == PInputEventID::MouseMove) {
                CoalesceMouseMoveEvent(*coalescableEvent, event);
            } else {
                *coalescableEvent = std::move(event);
            }
            return;
        }
    }
    KInputDeviceInode::QueueEvent_pl(std::move(event));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputMotionDeviceInode::ValidateEvent(const PInputEvent& event, size_t availableSize) const
{
    KInputDeviceInode::ValidateEvent(event, availableSize);

    if (event.EventType == PInputEventType::DeviceEvent) {
        return;
    }

    switch (m_ClassID)
    {
        case PInputClass::Mouse:
            if (event.EventType != PInputEventType::MouseEvent || event.EventSize < sizeof(PMouseEvent)) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            if (event.EventID != PInputEventID::MouseDown && event.EventID != PInputEventID::MouseUp && event.EventID != PInputEventID::MouseMove && event.EventID != PInputEventID::MouseWheel) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            break;

        case PInputClass::TouchScreen:
            if (event.EventType != PInputEventType::TouchEvent || event.EventSize < sizeof(PTouchEvent)) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            if (event.EventID != PInputEventID::TouchDown && event.EventID != PInputEventID::TouchUp && event.EventID != PInputEventID::TouchMove) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            break;

        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KInputMotionDeviceInode::EventQueueIterator KInputMotionDeviceInode::FindCoalescableEvent_pl(const EventBuffer& event)
{
    for (EventQueueIterator iterator = m_EventQueue.begin(); iterator != m_EventQueue.end(); ++iterator)
    {
        if (IsMatchingMoveEvent(*iterator, event)) {
            return iterator;
        }
    }
    return m_EventQueue.end();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PMouseEvent& KInputMotionDeviceInode::GetMouseEvent(const EventBuffer& event)
{
    return *reinterpret_cast<const PMouseEvent*>(event.data());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMouseEvent& KInputMotionDeviceInode::GetMouseEvent(EventBuffer& event)
{
    return *reinterpret_cast<PMouseEvent*>(event.data());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PTouchEvent& KInputMotionDeviceInode::GetTouchEvent(const EventBuffer& event)
{
    return *reinterpret_cast<const PTouchEvent*>(event.data());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInputMotionDeviceInode::CoalesceMouseMoveEvent(EventBuffer& queuedEvent, const EventBuffer& event)
{
    GetMouseEvent(queuedEvent).Position += GetMouseEvent(event).Position;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KInputMotionDeviceInode::IsMoveEvent(PInputEventID eventID)
{
    return eventID == PInputEventID::MouseMove || eventID == PInputEventID::TouchMove;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KInputMotionDeviceInode::IsMatchingMoveEvent(const EventBuffer& lhs, const EventBuffer& rhs) const
{
    const PInputEvent lhsHeader = GetEventHeader(lhs);
    const PInputEvent rhsHeader = GetEventHeader(rhs);

    if (lhsHeader.ClassID != rhsHeader.ClassID || lhsHeader.EventID != rhsHeader.EventID || lhsHeader.SourceID != rhsHeader.SourceID) {
        return false;
    }
    if (lhsHeader.ClassID == PInputClass::TouchScreen && lhs.size() >= sizeof(PTouchEvent) && rhs.size() >= sizeof(PTouchEvent))
    {
        return GetTouchEvent(lhs).TouchID == GetTouchEvent(rhs).TouchID;
    }
    return true;
}

} // namespace kernel
