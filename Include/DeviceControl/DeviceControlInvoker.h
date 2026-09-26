// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
class PDeviceControlDefInvoker : public PRPCInvoker<TDefinition::IsConst, typename TDefinition::Signature>
{
public:
    using Definition = TDefinition;

    PDeviceControlDefInvoker(const PDeviceControlInterface& dcInterface) :
        PRPCInvoker<TDefinition::IsConst, typename TDefinition::Signature>(
            [&dcInterface, handlerID = TDefinition::HandlerID](const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
            {
                const PErrorCode result = device_control(dcInterface.GetDeviceFD(), handlerID, inData, inDataLength, outData, outDataLength);
                if (result != PErrorCode::Success)
                {
                    p_system_log<PLogSeverity::ERROR>(LogCat_General, "DeviceControlDefInvoker {}/{} failed: {}", dcInterface.GetDeviceFD(), handlerID, strerror(std::to_underlying(result)));
                    PERROR_THROW_CODE(result);
                }
            }
        ) {}
};

template<int THandlerID, typename TReturnType, typename... TArgTypes>
class PDeviceControlInvoker
{
public:
};

template<int THandlerID, typename TReturnType, typename... TArgTypes>
class PDeviceControlInvoker<THandlerID, TReturnType(TArgTypes...)> : public PDeviceControlDefInvoker< PRPCDefinition<THandlerID, false, TReturnType, TArgTypes...>>
{};

template<int THandlerID, typename TReturnType, typename... TArgTypes>
class PDeviceControlInvoker<THandlerID, TReturnType(TArgTypes...) const> : public PDeviceControlDefInvoker<PRPCDefinition<THandlerID, true, TReturnType, TArgTypes...>>
{};
