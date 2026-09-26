// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
