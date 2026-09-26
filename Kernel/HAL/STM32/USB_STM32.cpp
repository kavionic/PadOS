// This file is part of PadOS.
//
// Copyright (c) 2022-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 22.05.2022 17:00

#include <bit>
#include <utility>
#include <Kernel/Kernel.h>
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

bool USB_STM32::IsDMABufferAccessible(const void* buffer, size_t length)
{
    const uintptr_t bufferAddress = reinterpret_cast<uintptr_t>(buffer);
    return bufferAddress >= USB_STM32_ITCM_INACCESSIBLE_END
        && (bufferAddress >= USB_STM32_DTCM_END || bufferAddress + length <= D1_DTCMRAM_BASE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::IsDirectDMATransmitBuffer(const void* buffer, size_t length)
{
    // DMA addresses must be word aligned. A final partial word stays within the same accessible memory region.
    return (reinterpret_cast<uintptr_t>(buffer) % sizeof(uint32_t)) == 0 && IsDMABufferAccessible(buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::IsDirectDMAReceiveBuffer(const void* buffer, size_t length)
{
    return (reinterpret_cast<uintptr_t>(buffer) % __SCB_DCACHE_LINE_SIZE) == 0
        && (length % __SCB_DCACHE_LINE_SIZE) == 0
        && IsDMABufferAccessible(buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USB_STM32::GetDirectDMAReceiveLength(const void* buffer, size_t length, size_t packetSize, size_t receiveCapacity)
{
    if ((reinterpret_cast<uintptr_t>(buffer) % __SCB_DCACHE_LINE_SIZE) == 0)
    {
        size_t directLength;
        if (receiveCapacity != 0)
        {
            // Hardware still receives complete packets; only cache maintenance may use the owned padding.
            const size_t availableLength = std::min(length, receiveCapacity - receiveCapacity % __SCB_DCACHE_LINE_SIZE);
            directLength = availableLength - availableLength % packetSize;
        }
        else
        {
            // The cache-line size is a power of two, so the common factor depends only on trailing zero bits.
            const int commonAlignmentShift = std::min(
                std::countr_zero(packetSize), std::countr_zero(size_t(__SCB_DCACHE_LINE_SIZE)));
            const size_t chunkAlignment = (packetSize >> commonAlignmentShift) * __SCB_DCACHE_LINE_SIZE;
            directLength = length - length % chunkAlignment;
        }
        const size_t cacheLength = align_up(directLength, __SCB_DCACHE_LINE_SIZE);
        return (directLength != 0 && IsDMABufferAccessible(buffer, cacheLength)) ? directLength : 0;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USB_STM32::CleanDMATransmitBuffer(const void* buffer, size_t length)
{
    if (length != 0)
    {
        const uintptr_t bufferAddress = reinterpret_cast<uintptr_t>(buffer);
        const uintptr_t cacheAddress = align_down(bufferAddress, __SCB_DCACHE_LINE_SIZE);
        const size_t cacheLength = align_up(bufferAddress + length, __SCB_DCACHE_LINE_SIZE) - cacheAddress;
        SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(cacheAddress), static_cast<int32_t>(cacheLength));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::Setup(USB_OTG_ID portID, USB_Mode mode, USB_Speed speed, USB_OTG_Phy phyInterface, bool useExternalVBus, bool batteryChargingEnabled, const PinMuxTarget& pinDM, const PinMuxTarget& pinDP, const PinMuxTarget& pinID, DigitalPinID pinVBus, bool useSOF)
{
    m_Port = get_usb_from_id(portID);
    if (m_Port == nullptr || pinDM.PINID == DigitalPinID::None || pinDP.PINID == DigitalPinID::None) {
        return false;
    }
    m_IRQ = get_usb_irq(portID);

    m_ConfigSpeed               = speed;
    m_PhyInterface              = phyInterface;
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
        if (!m_HostDriver.Setup(this, portID, enableVBusSense))
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "Failed to setup host mode.");
            return false;
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
    ReleaseCoreReset();
    if (!SetupCore(m_UseExternalVBus, m_BatteryChargingEnabled)) {
        return false;
    }
    return SetUSBMode(USB_Mode::Host);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USB_STM32::HoldCoreInReset()
{
    const uint32_t resetMask = (m_Port == get_usb_from_id(USB_OTG_ID::USB1_HS))
        ? RCC_AHB1RSTR_USB1OTGHSRST : RCC_AHB1RSTR_USB2OTGFSRST;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        RCC->AHB1RSTR |= resetMask;
        const uint32_t resetState = RCC->AHB1RSTR;
        (void)resetState;
        __DSB();
    } CRITICAL_END;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::ResetDeviceCore()
{
    ReleaseCoreReset();
    return SetupCore(m_UseExternalVBus, m_BatteryChargingEnabled) && SetUSBMode(USB_Mode::Device);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USB_STM32::DisableIRQDelivery()
{
    if (kis_in_irq(m_IRQ)) {
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

void USB_STM32::ReleaseCoreReset()
{
    const uint32_t resetMask = (m_Port == get_usb_from_id(USB_OTG_ID::USB1_HS))
        ? RCC_AHB1RSTR_USB1OTGHSRST : RCC_AHB1RSTR_USB2OTGFSRST;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        RCC->AHB1RSTR &= ~resetMask;
        const uint32_t resetState = RCC->AHB1RSTR;
        (void)resetState;
        __DSB();
    } CRITICAL_END;
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
    // Reserve its 18 FIFO locations.
    set_bit_group(m_Port->GDFIFOCFG, 0xffffu << 16, DMA_FIFO_USABLE_WORD_COUNT << 16);

    m_Port->GAHBCFG &= ~USB_OTG_GAHBCFG_HBSTLEN_Msk;
    m_Port->GAHBCFG |= USB_OTG_GAHBCFG_HBSTLEN_2;
    m_Port->GAHBCFG |= USB_OTG_GAHBCFG_DMAEN;

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

bool USB_STM32::CoreReset()
{
    if (!WaitForAHBIdle())
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSB, "CoreReset() Timeout while waiting for AHB to become idle.");
        return false;
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

void USB_STM32::EnableIRQ(bool enable)
{
    if (enable) {
        m_Port->GAHBCFG |= USB_OTG_GAHBCFG_GINT;
    } else {
        m_Port->GAHBCFG &= ~USB_OTG_GAHBCFG_GINT;
    }
}


} // namespace kernel
