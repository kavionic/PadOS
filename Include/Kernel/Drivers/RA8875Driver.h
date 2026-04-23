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
// Created: 21.04.2026 22:30

#pragma once

#include <Kernel/VFS/KDriverParametersBase.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/HAL/DigitalPort.h>
#include <RPC/RPCDispatcher.h>
#include <DeviceControl/RA8875.h>
#include <ApplicationServer/Drivers/RA8875Registers.h>


struct KRA8875DriverParameters : KDriverParametersBase
{
    static constexpr char DRIVER_NAME[] = "ra8875";

    KRA8875DriverParameters() = default;
    KRA8875DriverParameters(const PString& devicePath, PLCDRegisters* registers, DigitalPinID pinInterrupt)
        : KDriverParametersBase(devicePath)
        , Registers(uintptr_t(registers))
        , PinInterrupt(pinInterrupt)
    {}

    uintptr_t    Registers    = 0;
    DigitalPinID PinInterrupt = DigitalPinID::None;

    friend void to_json(Pjson& data, const KRA8875DriverParameters& value)
    {
        to_json(data, static_cast<const KDriverParametersBase&>(value));
        data.update(Pjson{
            {"registers",    value.Registers},
            {"pin_interrupt", value.PinInterrupt}
        });
    }
    friend void from_json(const Pjson& data, KRA8875DriverParameters& outValue)
    {
        from_json(data, static_cast<KDriverParametersBase&>(outValue));
        data.at("registers"    ).get_to(outValue.Registers);
        data.at("pin_interrupt").get_to(outValue.PinInterrupt);
    }
};


namespace kernel
{

class KRA8875Inode : public KInode, public KFilesystemFileOps
{
public:
    KRA8875Inode(const KRA8875DriverParameters& params);

    virtual void DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual void ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;

    void WaitBlitter();

private:
    static IRQResult IRQCallback(IRQn_Type irq, void* userData);
    IRQResult HandleIRQ();

    PRPCDispatcher      m_Dispatcher;
    DigitalPin          m_Pin;
    PLCDRegisters*      m_Registers = nullptr;
    KConditionVariable  m_CondVar;
};

} // namespace kernel
