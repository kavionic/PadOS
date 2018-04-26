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
// Created: 23.02.2018 21:25:42

#pragma once

#include <atomic>

#include "Kernel/VFS/KDeviceNode.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/KSemaphore.h"
#include "Kernel/KMutex.h"
#include "Kernel/HAL/DigitalPort.h"


namespace kernel
{

class KSemaphore;

class I2CFile : public KFileHandle
{
public:
    uint8_t m_SlaveAddress = 0;
    uint8_t m_InternalAddressLength = 0;
    uint32_t m_InternalAddress = 0;
    uint32_t m_RelativeTimeout = 2000; // Timeout relative to the theoretic minimum time * 1000 
    AsyncIOResultCallback* m_Callback = nullptr;
};

struct I2CWriteRequest
{
//    I2CWriteRequest() : m_Semaphore("i2c_request", 0, false) {}
    KSemaphore*                    m_Semaphore = nullptr;
    void*                          m_Buffer = nullptr;
    int32_t                        m_Length = 0;
    int32_t                        m_CurPos = 0;
    uint8_t                        m_SlaveAddress = 0;
    uint8_t                        m_InternalAddressLength = 0;
    uint32_t                       m_InternalAddress = 0;
    AsyncIOResultCallback* m_Callback = nullptr;
    void*                          m_UserObject = nullptr;
};

class I2CDriver : public KDeviceNode
{
public:
    enum class Channels : int8_t
    {
        Channel0 = 0,
        Channel1 = 1,
        Channel2 = 2,
        ChannelCount,
    };
    I2CDriver(Channels channel);
    virtual ~I2CDriver() override;

    virtual Ptr<KFileHandle> Open(int flags) override;

    virtual int     DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual ssize_t Read(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length) override;
    virtual ssize_t Write(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length) override;

//    virtual int ReadAsync(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback) override;
//    I2CWriteRequest* StartRead(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback, KSemaphore* semaphore);
//    virtual int WriteAsync(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback) override;
//    virtual int CancelAsyncRequest(Ptr<KFileHandle> file, int handle) override;
private:
    void Reset();
    void ClearBus();
    int SetBaudrate(uint32_t baudrate);
    int GetBaudrate() const;

    I2CDriver( const I2CDriver &c );
    I2CDriver& operator=( const I2CDriver &c );

    friend void TWIHS0_Handler();
    friend void TWIHS1_Handler();
    friend void TWIHS2_Handler();
    
    enum class State_e
    {
        Idle,
        Reading,
        Writing
    };
    
    static void IRQCallback(IRQn_Type irq, void* userData) { static_cast<I2CDriver*>(userData)->HandleIRQ(); }
    void HandleIRQ();
    
    uint32_t CalcAddress(uint32_t slaveAddress, int len);

    KMutex     m_Mutex;
    KSemaphore m_RequestSema;
    Twihs* m_Port;
    DigitalPin m_ClockPin;
    DigitalPin m_DataPin;
    DigitalPinPeripheralID m_PeripheralID;
    std::atomic<State_e> m_State;
    uint32_t             m_Baudrate = 400000;

    void*                m_Buffer = nullptr;
    int32_t              m_Length = 0;
    volatile int32_t     m_CurPos = 0;

//    size_t m_CurrentRequest = 0;
//    size_t m_PendingRequests = 0;
//    std::vector<I2CWriteRequest> m_Requests;

};

} // namespace
