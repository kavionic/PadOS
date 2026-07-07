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

#include <map>
#include <stdint.h>

#include <GUI/GUIEvent.h>
#include <Kernel/KMutex.h>
#include <Ptr/Ptr.h>


namespace kernel
{

class KInputDeviceInode;

class KUserInputManager
{
public:
    static KUserInputManager& Get();

    int32_t AddSource(PInputClass classID);
    void    RemoveSource(int32_t sourceID);
    void    AddEvent(const PInputEvent& event);

private:
    KUserInputManager();

    void AdvanceNextSourceID();

    KMutex m_Mutex;

    std::map<PInputClass, Ptr<KInputDeviceInode>> m_DeviceNodes;

    int m_KeyboardNodeHandle = -1;
    int m_MouseNodeHandle = -1;
    int m_TouchNodeHandle = -1;

    int32_t m_NextSourceID = 0;

    std::map<int32_t, Ptr<KInputDeviceInode>> m_SourceNodes;

    KUserInputManager(const KUserInputManager&) = delete;
    KUserInputManager& operator=(const KUserInputManager&) = delete;
};

} // namespace kernel
