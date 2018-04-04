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
// Created: 06.03.2018 11:48:18

#pragma once

#include "System/System.h"
#include "System/Signals/SignalTarget.h"
#include "System/Threads/Semaphore.h"
#include "System/Utils/MessagePort.h"

class Tests : public SignalTarget
{
public:
    Tests();
    ~Tests();

    static void TestThreadWaitThread(void* args);
    void TestThreadWait();

    void TestMessagePort();

    static void TestSemaphoreThread(void* args);
    void TestSemaphore();
    void TestSignals();

private:
    Semaphore m_StdOutLock;

    MessagePort m_MsgPort;

    Tests( const Tests &c );
    Tests& operator=( const Tests &c );

};

