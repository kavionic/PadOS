// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 19.10.2025 22:30

#pragma once

#include <utility>
#include <string.h>

#include <PadOS/Filesystem.h>
#include <System/ExceptionHandling.h>
#include <RPC/RPCInvoker.h>
#include <RPC/RPCDefinition.h>

class PDeviceControlInterface
{
public:
    void SetDeviceFD(int fd) noexcept { m_DeviceFD = fd; }
    int  GetDeviceFD() const noexcept { return m_DeviceFD; }

private:
    int m_DeviceFD = -1;
};

template<typename TDefinition>
class PDeviceControlDefInvoker : public PRPCInvoker<typename TDefinition::Signature>
{
public:
    using Definition = TDefinition;

    PDeviceControlDefInvoker(const PDeviceControlInterface& dcInterface) :
        PRPCInvoker<typename TDefinition::Signature>(
            [&dcInterface, handlerID = TDefinition::HandlerID](const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
            {
                const PErrorCode result = device_control(dcInterface.GetDeviceFD(), handlerID, inData, inDataLength, outData, outDataLength);
                if (result != PErrorCode::Success)
                {
                    printf("ERROR: DeviceControlDefInvoker %d/%d failed: %s\n", dcInterface.GetDeviceFD(), handlerID, strerror(std::to_underlying(result)));
                    PERROR_THROW_CODE(result);
                }
            }
        ) {}
};

template<int THandlerID, typename TReturnType, typename... TArgTypes>
class PDeviceControlInvoker : public PDeviceControlDefInvoker<PRPCDefinition<THandlerID, TReturnType, TArgTypes...>>
{
public:
    PDeviceControlInvoker(const PDeviceControlInterface& dcInterface) : PDeviceControlDefInvoker<PRPCDefinition<THandlerID, TReturnType, TArgTypes...>>(dcInterface) {}
};

template<int THandlerID, typename TReturnType, typename... TArgTypes>
class PDeviceControlInvoker<THandlerID, TReturnType(TArgTypes...)> : public PDeviceControlInvoker<THandlerID, TReturnType, TArgTypes...>
{};
