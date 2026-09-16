// This file is part of PadOS.
//
// Copyright (C) 2022-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 22.05.2022 17:00

#include <string.h>
#include <utility>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Kernel/HAL/PeripheralMapping.h>
#include <Kernel/HAL/STM32/USB_STM32.h>
#include <Kernel/USB/USBProtocol.h>
#ifdef PADOS_MODULE_USB_HOST
#include <Kernel/USB/USBHost.h>
#endif
#include <Utils/Utils.h>
#include <System/TimeValue.h>


namespace kernel
{

#if defined(STM32H7)
static constexpr uintptr_t USB_STM32_ITCM_INACCESSIBLE_END = D1_ITCMICP_BASE + 128 * 1024;
static constexpr uintptr_t USB_STM32_DTCM_END = D1_DTCMRAM_BASE + 128 * 1024;
#else
#error USB DMA memory accessibility must be defined for this STM32 platform.
#endif

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_STM32::USB_STM32()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_STM32::~USB_STM32()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::IsDirectDMABuffer(const void* buffer, size_t length)
{
    const uintptr_t bufferAddress = reinterpret_cast<uintptr_t>(buffer);
    if ((bufferAddress % __SCB_DCACHE_LINE_SIZE) != 0 || (length % __SCB_DCACHE_LINE_SIZE) != 0) {
        return false;
    }
    return bufferAddress >= USB_STM32_ITCM_INACCESSIBLE_END
        && (bufferAddress >= USB_STM32_DTCM_END || bufferAddress + length <= D1_DTCMRAM_BASE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::Setup(USB_OTG_ID portID, USB_Mode mode, USB_Speed speed, USB_OTG_Phy phyInterface, bool enableDMA, bool useExternalVBus, bool batteryChargingEnabled, const PinMuxTarget& pinDM, const PinMuxTarget& pinDP, const PinMuxTarget& pinID, DigitalPinID pinVBus, bool useSOF)
{
    m_Port = get_usb_from_id(portID);
    if (m_Port == nullptr || pinDM.PINID == DigitalPinID::None || pinDP.PINID == DigitalPinID::None) {
        return false;
    }
    m_FIFOBase      = reinterpret_cast<volatile uint32_t*>(reinterpret_cast<volatile uint8_t*>(m_Port) + USB_OTG_FIFO_BASE);
    m_IRQ = get_usb_irq(portID);

    m_ConfigSpeed               = speed;
    m_PhyInterface              = phyInterface;
    m_UseDMA                    = enableDMA;
    m_UseExternalVBus           = useExternalVBus;
    m_BatteryChargingEnabled    = batteryChargingEnabled;

    DigitalPin::ActivatePeripheralMux(pinDM);
    DigitalPin::ActivatePeripheralMux(pinDP);

    DigitalPin(pinDM.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    DigitalPin(pinDP.PINID).SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);

    if (pinID.PINID != DigitalPinID::None) {
        DigitalPin::ActivatePeripheralMux(pinID);
    }
    if (pinVBus != DigitalPinID::None)
    {
        DigitalPin(pinVBus).SetDirection(DigitalPinDirection_e::Analog);
    }
    if (!SetupCore(useExternalVBus, batteryChargingEnabled))
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "Failed to setup USB core.");
        return false;
    }
    if (!SetUSBMode(mode))
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "Failed switch mode.");
        return false;
    }
    const bool enableVBusSense = pinVBus != DigitalPinID::None;
    if (mode == USB_Mode::Host)
    {
#ifdef PADOS_MODULE_USB_HOST
        if (!m_HostDriver.Setup(this, portID, enableVBusSense)) {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "Failed to setup host mode.");
        }
#else
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "USB host mode is disabled.");
        return false;
#endif
    }
    else
    {
        if (!m_DeviceDriver.Setup(this, portID, enableVBusSense, useSOF))
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "Failed to setup device mode.");
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USB_STM32::Shutdown()
{
#ifdef PADOS_MODULE_USB_HOST
    if (GetUSBMode() == USB_Mode::Host) {
        m_HostDriver.Shutdown();
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::ResetHostCore()
{
    if (!SetupCore(m_UseExternalVBus, m_BatteryChargingEnabled)) {
        return false;
    }
    return SetUSBMode(USB_Mode::Host);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::DisableIRQDelivery()
{
    const uint32_t currentExceptionNumber = __get_IPSR();
    const uint32_t irqExceptionNumber = static_cast<uint32_t>(std::to_underlying(m_IRQ)) + 16u;

    if (currentExceptionNumber == irqExceptionNumber) {
        return false;
    }

    const bool wasEnabled = NVIC_GetEnableIRQ(m_IRQ) != 0;
    if (wasEnabled) {
        NVIC_DisableIRQ(m_IRQ);
    }
    return wasEnabled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USB_STM32::RestoreIRQDelivery(bool wasEnabled)
{
    if (wasEnabled) {
        NVIC_EnableIRQ(m_IRQ);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::SetupCore(bool useExternalVBus, bool batteryChargingEnabled)
{
    if (m_PhyInterface == USB_OTG_Phy::ULPI)
    {
        m_Port->GCCFG &= ~(USB_OTG_GCCFG_PWRDWN);

        // Init the ULPI interface.
        m_Port->GUSBCFG &= ~(USB_OTG_GUSBCFG_TSDPS | USB_OTG_GUSBCFG_ULPIFSLS | USB_OTG_GUSBCFG_PHYSEL);

        // Select VBus source.
        m_Port->GUSBCFG &= ~(USB_OTG_GUSBCFG_ULPIEVBUSD | USB_OTG_GUSBCFG_ULPIEVBUSI);
        if (useExternalVBus) {
            m_Port->GUSBCFG |= USB_OTG_GUSBCFG_ULPIEVBUSD;
        }
        // Reset after a PHY select.
        if (!CoreReset())
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "Failed to reset USB core.");
            return false;
        }
    }
    else
    {
        // Select FS embedded Phy.
        m_Port->GUSBCFG |= USB_OTG_GUSBCFG_PHYSEL;

        // Reset after a PHY select.
        if (!CoreReset())
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "Failed to reset USB core.");
            return false;
        }

        if (!batteryChargingEnabled) {
            m_Port->GCCFG |= USB_OTG_GCCFG_PWRDWN; // Activate the USB transceiver.
        } else {
            m_Port->GCCFG &= ~(USB_OTG_GCCFG_PWRDWN); // Deactivate the USB transceiver.
        }
    }
    if (m_UseDMA)
    {
        // Reserve 18 FIFO locations for DMA buffers.
        set_bit_group(m_Port->GDFIFOCFG, 0xffffu << 16, DMA_FIFO_USABLE_WORD_COUNT << 16);

        m_Port->GAHBCFG &= ~USB_OTG_GAHBCFG_HBSTLEN_Msk;
        m_Port->GAHBCFG |= USB_OTG_GAHBCFG_HBSTLEN_2;
        m_Port->GAHBCFG |= USB_OTG_GAHBCFG_DMAEN;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::WaitForAHBIdle()
{
    for (TimeValNanos endTime = kget_monotonic_time() + TimeValNanos::FromMilliseconds(100); (m_Port->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL) == 0; ) {
        if (kget_monotonic_time() > endTime) return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

volatile uint32_t* USB_STM32::GetFIFOBase(uint32_t endpoint)
{
    return m_FIFOBase + endpoint * USB_OTG_FIFO_SIZE / 4;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::CoreReset()
{
    if (!WaitForAHBIdle())
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "CoreReset() Timeout while waiting for AHB to become idle.");
        return false;
    }
    for (TimeValNanos endTime = kget_monotonic_time() + TimeValNanos::FromMilliseconds(100); (m_Port->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL) == 0; ) {
        if (kget_monotonic_time() > endTime) return false;
    }

    // Core soft reset.
    m_Port->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;

    for (TimeValNanos endTime = kget_monotonic_time() + TimeValNanos::FromMilliseconds(100); (m_Port->GRSTCTL & USB_OTG_GRSTCTL_CSRST); ) {
        if (kget_monotonic_time() > endTime) return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::FlushTxFifo(uint32_t count)
{
    if (!WaitForAHBIdle())
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "FlushTxFifo() Timeout while waiting for AHB to become idle.");
        return false;
    }
    m_Port->GRSTCTL = (USB_OTG_GRSTCTL_TXFFLSH | (count << USB_OTG_GRSTCTL_TXFNUM_Pos));

    for (TimeValNanos endTime = kget_monotonic_time() + TimeValNanos::FromMilliseconds(100); (m_Port->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH); ) {
        if (kget_monotonic_time() > endTime) return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::FlushRxFifo()
{
    if (!WaitForAHBIdle())
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "FlushRxFifo() Timeout while waiting for AHB to become idle.");
        return false;
    }
    m_Port->GRSTCTL = USB_OTG_GRSTCTL_RXFFLSH;

    for (TimeValNanos endTime = kget_monotonic_time() + TimeValNanos::FromMilliseconds(100); (m_Port->GRSTCTL & USB_OTG_GRSTCTL_RXFFLSH); ) {
        if (kget_monotonic_time() > endTime) return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::SetUSBMode(USB_Mode mode)
{
    m_Port->GUSBCFG &= ~(USB_OTG_GUSBCFG_FHMOD | USB_OTG_GUSBCFG_FDMOD);

    if (mode == USB_Mode::Host) {
        m_Port->GUSBCFG |= USB_OTG_GUSBCFG_FHMOD;
    } else if (mode == USB_Mode::Device) {
        m_Port->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD;
    } else {
        return false;
    }

    for (TimeValNanos endTime = kget_monotonic_time() + TimeValNanos::FromMilliseconds(50); GetUSBMode() != mode; )
    {
        if (kget_monotonic_time() > endTime) return false;
        snooze_ms(1);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_Mode USB_STM32::GetUSBMode() const
{
    return (m_Port->GINTSTS & USB_OTG_GINTSTS_CMOD) ? USB_Mode::Host : USB_Mode::Device;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USB_STM32::ReadFromFIFO(void* buffer, size_t length)
{
    volatile const uint32_t* fifoRegister = GetFIFOBase(0);

    // Read all full 32-bit words.
    const uint32_t fullWords = length  / 4;
    uint32_t* dst32 = reinterpret_cast<uint32_t*>(buffer);
    if (reinterpret_cast<intptr_t>(dst32) & 0x03) {
        for (uint32_t i = 0; i < fullWords; i++) unaligned_write(dst32++, *fifoRegister++);
    } else {
        for (uint32_t i = 0; i < fullWords; i++) *dst32++ = *fifoRegister++;
    }
    // Read the remaining bytes, if any.
    length &= 0x03;
    if (length != 0)
    {
        uint8_t* dst8 = reinterpret_cast<uint8_t*>(dst32);
        uint32_t data = *fifoRegister;
        while (length--)
        {
            *dst8++ = uint8_t(data & 0xff);
            data >>= 8;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USB_STM32::WriteToFIFO(uint32_t fifoIndex, const void* buffer, size_t length)
{
    volatile uint32_t* fifoRegister = GetFIFOBase(fifoIndex);

    // Write all full 32-bit words.
    const uint32_t fullWords = length / 4;
    const uint32_t* src32 = reinterpret_cast<const uint32_t*>(buffer);
    if (reinterpret_cast<intptr_t>(src32) & 0x03) {
        for (uint32_t i = 0; i < fullWords; i++) *fifoRegister++ = unaligned_read<uint32_t>(src32++);
    } else {
        for (uint32_t i = 0; i < fullWords; i++) *fifoRegister++ = *src32++;
    }
    // Write the remaining bytes, if any.
    length &= 0x03;
    if (length != 0)
    {
        uint32_t data = 0;
        const uint8_t* src8 = reinterpret_cast<const uint8_t*>(src32);
        for (uint32_t i = 0; i < length; ++i)
        {
            data |= uint32_t(*src8++) << (i * 8);
        }
        *fifoRegister = data;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USB_STM32::EnableIRQ(bool enable)
{
    if (enable) {
        m_Port->GAHBCFG |= USB_OTG_GAHBCFG_GINT;
    } else {
        m_Port->GAHBCFG &= ~USB_OTG_GAHBCFG_GINT;
    }
}


} // namespace kernel
