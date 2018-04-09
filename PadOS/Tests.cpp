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

#include "sam.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <atomic>
#include <vector>
#include <utility>

#include "Tests.h"
#include "Kernel/KSemaphore.h"
#include "System/Threads.h"
#include "System/Threads/Thread.h"
#include "System/Threads/Looper.h"
#include "System/Threads/Semaphore.h"
#include "System/Threads/EventHandler.h"
#include "System/SystemMessageIDs.h"
#include "System/System.h"
#include "System/Signals/Signal.h"
#include "System/Signals/RemoteSignal.h"

using namespace os;


class UserView : public EventHandler
{
public:
        
};


Tests::Tests() : m_StdOutLock("tests_std_out"), m_MsgPort("test_port", 10)
{


//    TestThreadWait();
//    TestSemaphore();
    TestSignals();
    TestMessagePort();
}

Tests::~Tests()
{
}


void Tests::TestThreadWaitThread(void* args)
{
    int32_t* argsList = (int32_t*)args;

    exit_thread(argsList[0] + argsList[1]);
}

void Tests::TestThreadWait()
{
    int32_t args[] = {4, 38};

    printf("TestThreadWait(): Spawn test thread\n");
    thread_id thread = spawn_thread("test", TestThreadWaitThread, 0, args, true);
    int32_t result = wait_thread(thread);
    printf("Result: %" PRId32 "\n", result);

}


class MessagePortTestHandler : public EventHandler, public SignalTarget
{
public:
    MessagePortTestHandler(Semaphore& stdOutLock) : EventHandler("MessagePortTestHandler"), m_StdOutLock(stdOutLock) {
        TestSignal.Connect(this, &MessagePortTestHandler::SlotTest);
    }

    virtual bool HandleMessage(int32_t code, const void* data, size_t length) override
    {
        if (code == TestSignal.GetID()) {
            TestSignal.Dispatch(data, length);
        }            
/*        bigtime_t curTime = get_system_time_hires();
        CRITICAL_BEGIN(m_StdOutLock) {
            printf("Message %" PRId32 " received at %" PRId64 ".%" PRId64 "\n", code, curTime / 1000, curTime % 1000);
        } CRITICAL_END;*/
        return true;
    }

private:
    void SlotTest(int a, float b/*, const String& str*/) {
        printf("Test: %d, %f: '%s'\n", a, b, "str.c_str()");
    }
    Semaphore& m_StdOutLock;
    
    RemoteSignal<1, void, int, float, const String&>::Receiver TestSignal;
};

class MessagePortTestThread : public Looper
{
public:
    MessagePortTestThread(Semaphore& stdOutLock) : Looper("MessagePortTestThread", 10), m_StdOutLock(stdOutLock)
    {
        Ptr<MessagePortTestHandler> handler = ptr_new<MessagePortTestHandler>(m_StdOutLock);
        
        AddHandler(handler);
    }
    
private:
    Semaphore& m_StdOutLock;
};

void Tests::TestMessagePort()
{
    printf("Tests::TestMessagePort():\n");

    MessagePortTestThread* looper = new MessagePortTestThread(m_StdOutLock);
    
    
    m_MsgPort = looper->GetPort();

    looper->Start("looper_message_test", true, 1);

    typedef RemoteSignal<1, void, int, float, const String&>::Sender TestSignal;
        
    for (int i = 0; i < 20; ++i)
    {
        char    message[256];

        sprintf(message, "Test message %d", i);
        bigtime_t curTime = get_system_time_hires();
        
        TestSignal::Emit(m_MsgPort, -1, INFINIT_TIMEOUT, i, float(curTime) / 1.0e6f, String(message));
        
/*        if (port.SendMessage(i, message, strlen(message) + 1)) {
            CRITICAL_BEGIN(m_StdOutLock) {
                printf("Sent: %d at %" PRId64 ".%" PRId64 "\n", i, curTime / 1000, curTime % 1000);
            } CRITICAL_END;
            snooze(20000);
        } else {
            CRITICAL_BEGIN(m_StdOutLock) {
                printf("Receive: FAILED!!! %d\n", get_last_error());
            } CRITICAL_END;
        }*/
    }
    m_MsgPort.SendMessage(-1, MessageID::QUIT, nullptr, 0);
    int32_t result = looper->Wait();
//    int32_t result = wait_thread(thread);
    printf("MessagePortTest result: %" PRId32 "\n", result);
}

Semaphore s_Semaphore("test_sema", 1, true);
std::atomic_int s_SemaphoreTestCounter;
void Tests::TestSemaphoreThread(void* args)
{
//    for(;;)
    {
        for (int i = 0; i < 1000; ++i)
        {
            CRITICAL_BEGIN(s_Semaphore)
            {
                s_SemaphoreTestCounter++;
                if (s_SemaphoreTestCounter != 1) {
                    kernel::panic("Semaphore failed\n");
                }
                s_SemaphoreTestCounter--;
            } CRITICAL_END;
        }
        CRITICAL_BEGIN(s_Semaphore) {
            printf("%d\n", get_thread_id());
        } CRITICAL_END;
    }
    exit_thread(123);
}

void Tests::TestSemaphore()
{
    static const int threadCount = 1000;

    //s_Semaphore = create_semaphore("test_sema", 1, true);

    for (int i = 0; i < threadCount; ++i)
    {
        char threadName[OS_NAME_LENGTH];
        snprintf(threadName, OS_NAME_LENGTH, "test%d", i);
        spawn_thread(threadName, TestSemaphoreThread, 0);
    }

    for (;;)
    {
//        int curValue = s_SemaphoreTestCounter;
//        printf("Cur: %d\n", curValue);
        snooze(50000);
    }
}


class SignalTester : public Thread, public SignalTarget
{
public:
    SignalTester(Semaphore& stdOutLock) : Thread("SignalTester"), m_StdOutLock(stdOutLock) {}
    virtual int Run() override
    {
        TestSignal.Connect(this, &SignalTester::TestSlot);
        for (int i = 0; i < 10; ++i)
        {
            char message[128];
            sprintf(message, "SignalTest %d:%d", GetThreadID(), i);
            TestSignal(message);
        }
        return GetThreadID();
    }

    Signal<void, const char*> TestSignal;
private:
    void TestSlot(const char* arg)
    {
        CRITICAL_BEGIN(m_StdOutLock) {
            printf("%s\n", arg);
        } CRITICAL_END;
    }
    Semaphore& m_StdOutLock;
};

void Tests::TestSignals()
{
    const int THREAD_COUNT = 100;
    std::vector<SignalTester*> threads;
    threads.reserve(THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        SignalTester* thread = new SignalTester(m_StdOutLock);
        threads.push_back(thread);
    }
    for (auto i : threads) {
        i->Start("signal_tester", true);
    }
    int index = 0;
    for (auto i : threads) {
        int result = i->Wait();

        CRITICAL_BEGIN(m_StdOutLock) {
            printf("Signal tester %d result: %d\n", index, result);
        } CRITICAL_END;
        index++;
    }
}

