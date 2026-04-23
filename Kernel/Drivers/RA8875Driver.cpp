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

#include <Kernel/Drivers/RA8875Driver.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/HAL/STM32/Peripherals_STM32H7.h>
#include <ApplicationServer/Drivers/RA8875Registers.h>


namespace kernel
{

PREGISTER_KERNEL_DRIVER(KRA8875Inode, KRA8875DriverParameters);


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KRA8875Inode::KRA8875Inode(const KRA8875DriverParameters& params)
    : KInode(nullptr, nullptr, this, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
    , m_Pin(params.PinInterrupt)
    , m_Registers(reinterpret_cast<PLCDRegisters*>(params.Registers))
    , m_CondVar("ra8875irq")
{
    m_Dispatcher.AddHandler(&PRA8875::WaitBlitter, this, &KRA8875Inode::WaitBlitter);

    m_Pin.SetDirection(DigitalPinDirection_e::In);
    m_Pin.SetInterruptMode(PinInterruptMode_e::FallingEdge);
    m_Pin.EnableInterrupts();
    register_irq_handler(get_peripheral_irq(params.PinInterrupt), IRQCallback, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KRA8875Inode::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    m_Dispatcher.Dispatch(request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KRA8875Inode::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KRA8875Inode::WaitBlitter()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);

    while (m_Registers->CMD & RA8875_STATUS_BTE_BUSY_bm)
    {
        if (!m_Pin.Read())
        {
            // IRQ already asserted. Just clear the BTE IRQ flag.
            m_Registers->CMD = RA8875_INTC2;
            m_Registers->DATA = RA8875_INTC2_BTE_bm;
        }
        else
        {
            m_CondVar.IRQWaitTimeout(TimeValNanos::FromMilliseconds(100));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult KRA8875Inode::IRQCallback(IRQn_Type irq, void* userData)
{
    return static_cast<KRA8875Inode*>(userData)->HandleIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult KRA8875Inode::HandleIRQ()
{
    if (m_Pin.GetAndClearInterruptStatus())
    {
        // Clear the BTE IRQ flag.
        m_Registers->CMD  = RA8875_INTC2;
        m_Registers->DATA = RA8875_INTC2_BTE_bm;
        m_CondVar.Wakeup(1);
        return IRQResult::HANDLED;
    }
    return IRQResult::UNHANDLED;
}

} // namespace kernel
