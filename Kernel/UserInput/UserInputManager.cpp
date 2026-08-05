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

#include <limits>

#include <Kernel/UserInput/InputDeviceInode.h>
#include <Kernel/UserInput/UserInputManager.h>
#include <Kernel/VFS/KDriverManager.h>
#include <System/ExceptionHandling.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KUserInputManager& KUserInputManager::Get()
{
    static KUserInputManager instance;
    return instance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t KUserInputManager::AddSource(PInputClass classID)
{
    KScopedLock lock(m_Mutex);

    const auto deviceNodeIterator = m_DeviceNodes.find(classID);
    if (deviceNodeIterator == m_DeviceNodes.end()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    Ptr<KInputDeviceInode> sourceNode = deviceNodeIterator->second;

    const int32_t firstSourceID = m_NextSourceID;
    for (;;)
    {
        const int32_t sourceID = m_NextSourceID;
        AdvanceNextSourceID();

        if (m_SourceNodes.find(sourceID) == m_SourceNodes.end())
        {
            m_SourceNodes[sourceID] = sourceNode;
            PScopeFail sourceMapCleanup([this, sourceID]() { m_SourceNodes.erase(sourceID); });

            sourceNode->AddSource(sourceID);
            return sourceID;
        }
        if (m_NextSourceID == firstSourceID) {
            PERROR_THROW_CODE(PErrorCode::NOMEM);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KUserInputManager::RemoveSource(int32_t sourceID)
{
    KScopedLock lock(m_Mutex);

    const auto sourceIterator = m_SourceNodes.find(sourceID);
    if (sourceIterator != m_SourceNodes.end())
    {
        sourceIterator->second->RemoveSource(sourceID);
        m_SourceNodes.erase(sourceIterator);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KUserInputManager::AddEvent(const PInputEvent& event)
{
    Ptr<KInputDeviceInode> targetNode;

    {
        KScopedLock lock(m_Mutex);

        const auto sourceIterator = m_SourceNodes.find(event.SourceID);
        if (sourceIterator == m_SourceNodes.end()) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        targetNode = sourceIterator->second;
    }

    targetNode->AddEvent(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KUserInputManager::KUserInputManager()
    : m_Mutex("user_input_manager", PEMutexRecursionMode_RaiseError)
{
    Ptr<KInputDeviceInode> keyboardNode = ptr_new<KInputDeviceInode>(PInputClass::Keyboard, 1024);
    Ptr<KInputDeviceInode> mouseNode    = ptr_new<KInputMotionDeviceInode>(PInputClass::Mouse, 256);
    Ptr<KInputDeviceInode> touchNode    = ptr_new<KInputMotionDeviceInode>(PInputClass::TouchScreen, 256);

    m_DeviceNodes[PInputClass::Keyboard]    = keyboardNode;
    m_DeviceNodes[PInputClass::Mouse]       = mouseNode;
    m_DeviceNodes[PInputClass::TouchScreen] = touchNode;

    m_KeyboardNodeHandle = kregister_device_root_trw("input/keyboard", keyboardNode);
    m_MouseNodeHandle    = kregister_device_root_trw("input/mouse", mouseNode);
    m_TouchNodeHandle    = kregister_device_root_trw("input/touch", touchNode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KUserInputManager::AdvanceNextSourceID()
{
    if (m_NextSourceID == std::numeric_limits<int32_t>::max()) {
        m_NextSourceID = 0;
    } else {
        ++m_NextSourceID;
    }
}

} // namespace kernel
