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
// Created: 22.02.2018 23:06:23

#pragma once

#include "UART.h"
#include "Kernel/VFS/KDeviceNode.h"

namespace kernel
{

class UARTDriver : public KDeviceNode
{
public:
    UARTDriver(UART::Channels channel);

    virtual ~UARTDriver() override;

    virtual int     DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual ssize_t Read(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length) override;
    virtual ssize_t Write(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length) override;


private:
    UARTDriver( const UARTDriver &c );
    UARTDriver& operator=( const UARTDriver &c );

    UART::Channels m_Channel;

    UART m_Port;
};

} // namespace
