// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <memory>
#include <system_error>
#include <gtest/gtest.h>
#include <Kernel/HAL/STM32/USB_STM32.h>
#include <Kernel/USB/USBHost.h>
#ifdef PADOS_MODULE_USB_HOST
#include <Kernel/USB/ClassDrivers/USBHostCDCChannel.h>
#include <Kernel/VFS/KFileHandle.h>
#endif

namespace kernel
{

// Exercise the real HAL submission, selection and accounting against RAM-backed registers.
// IRQ masking is replaced; these tests do not emulate controller handshakes or bus traffic.
class USBReceiveCapacityController : public USB_STM32
{
private:
    bool DisableIRQDelivery() override { return false; }
    void RestoreIRQDelivery(bool wasEnabled) override {}
};

class USBReceiveCapacityTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        size_t storageSize = m_StorageSize;
#ifdef PADOS_MODULE_USB_HOST
        storageSize += USBHost_STM32::DMA_BOUNCE_BUFFER_SIZE;
#endif
        m_Storage.reset(static_cast<uint8_t*>(memalign(__SCB_DCACHE_LINE_SIZE, storageSize)));
        ASSERT_NE(m_Storage.get(), nullptr);
        ASSERT_TRUE(USB_STM32::IsDirectDMAReceiveBuffer(m_Storage.get(), storageSize));
        m_Device.m_Driver = &m_Controller;
        m_Device.m_DeviceReady = true;
        m_Device.m_OutEndpoints = m_OutEndpoints.data();
        m_Device.m_InEndpoints = m_InEndpoints.data();
        m_Device.m_Port = &m_GlobalRegisters;
        m_Device.m_Device = &m_DeviceRegisters;
        m_Device.m_DMABounceBuffers = static_cast<uint8_t*>(memalign(__SCB_DCACHE_LINE_SIZE,
            USBDevice_STM32::DMA_BUFFER_COUNT * USBDevice_STM32::DMA_BOUNCE_BUFFER_SIZE));
        ASSERT_NE(m_Device.m_DMABounceBuffers, nullptr);
#ifdef PADOS_MODULE_USB_HOST
        m_Host.m_Driver = &m_Controller;
        m_Host.m_Host = &m_HostRegisters;
        m_Host.m_HostChannels = &m_ChannelRegisters;
        m_Host.m_DMABounceBuffers = reinterpret_cast<uint8_t (*)[USBHost_STM32::DMA_BOUNCE_BUFFER_SIZE]>(
            m_Storage.get() + m_StorageSize);
#endif
    }

    void Configure(size_t packetSize)
    {
        ASSERT_GT(packetSize, 0u);
        m_Device.CancelEndpointTransfer(m_EndpointOut);
        DeviceTransfer().EndpointMaxSize = packetSize;
#ifdef PADOS_MODULE_USB_HOST
        ASSERT_LE(packetSize, USBHost_STM32::DMA_BOUNCE_BUFFER_SIZE);
        CancelHost();
        HostTransfer() = USBHostChannelData();
        HostTransfer().MaxPacketSize = static_cast<uint16_t>(packetSize);
        HostTransfer().MaxDMAPacketCount = static_cast<uint16_t>(std::min<size_t>(
            USB_OTG_HCTSIZ_PKTCNT_Msk >> USB_OTG_HCTSIZ_PKTCNT_Pos,
            USB_OTG_HCTSIZ_XFRSIZ_Msk / packetSize));
        HostTransfer().BounceDMAPacketCount = static_cast<uint16_t>(USBHost_STM32::DMA_BOUNCE_BUFFER_SIZE / packetSize);
#endif
    }

    void TearDown() override
    {
        m_Device.CancelEndpointTransfer(m_EndpointOut);
#ifdef PADOS_MODULE_USB_HOST
        CancelHost();
#endif
    }

    USBDevice_STM32::EndpointTransferState& DeviceTransfer() { return m_Device.m_TransferStatusOut[1]; }

    bool SubmitDevice(void* buffer, size_t length, size_t capacity = 0)
    {
        return m_Device.EndpointTransfer(m_EndpointOut, buffer, length, capacity);
    }

    void ExpectDeviceChunk(size_t length, size_t packetCount, bool direct, const void* buffer)
    {
        const auto& transfer = DeviceTransfer();
        EXPECT_EQ(transfer.DMATransferSize, length);
        EXPECT_EQ(transfer.DMATransferDataLength, length);
        EXPECT_EQ(transfer.DMAUsesBounceBuffer, !direct);
        EXPECT_EQ(m_OutEndpoints[1].DOEPTSIZ & USB_OTG_DOEPTSIZ_XFRSIZ_Msk, length);
        EXPECT_EQ((m_OutEndpoints[1].DOEPTSIZ & USB_OTG_DOEPTSIZ_PKTCNT_Msk) >> USB_OTG_DOEPTSIZ_PKTCNT_Pos, packetCount);
        if (direct) {
            EXPECT_EQ(transfer.DMATransferBuffer, buffer);
        }
    }

    bool FinishDevice(size_t received, bool commit = true)
    {
        m_OutEndpoints[1].DOEPTSIZ = DeviceTransfer().DMATransferSize - received;
        return m_Device.FinishDMATransfer(m_EndpointOut, commit, &m_ShortPacket);
    }

    bool ContinueDevice() { return m_Device.StartDMATransfer(m_EndpointOut, DeviceTransfer().Generation); }

    void CheckToggleReset(USB_TransferType transferType, bool directionIn, bool enabled)
    {
        const uint8_t endpointAddr = directionIn ? USB_MK_IN_ADDRESS(1) : m_EndpointOut;
        m_DeviceRegisters.DCTL = 0;
        auto& transfer = *m_Device.GetEndpointTranferState(endpointAddr);
        transfer.EndpointMaxSize = 64;
        transfer.Buffer = m_Storage.get();
        transfer.BufferSize = 64;
        transfer.Generation = 17;
        transfer.BytesTransferred = 32;
        transfer.TransferActive = enabled;
        const uint32_t control = (std::to_underlying(transferType) << USB_OTG_DIEPCTL_EPTYP_Pos)
            | USB_OTG_DIEPCTL_USBAEP | USB_OTG_DIEPCTL_EONUM_DPID | (enabled ? USB_OTG_DIEPCTL_EPENA : 0);
        if (directionIn)
        {
            m_InEndpoints[1].DIEPCTL = control;
            m_InEndpoints[1].DIEPINT = USB_OTG_DIEPINT_INEPNE | USB_OTG_DIEPINT_XFRC;
            m_InEndpoints[1].DIEPTSIZ = 32;
            m_InEndpoints[1].DIEPDMA = 0x12345678;
        }
        else
        {
            m_OutEndpoints[1].DOEPCTL = control;
            m_OutEndpoints[1].DOEPINT = USB_OTG_DOEPINT_XFRC;
            m_OutEndpoints[1].DOEPTSIZ = 32;
            m_OutEndpoints[1].DOEPDMA = 0x12345678;
            m_GlobalRegisters.GINTSTS = USB_OTG_GINTSTS_BOUTNAKEFF;
        }
        // RAM supplies the NAK acknowledgements. It cannot emulate read/set bits or the actual PID change.
        ASSERT_TRUE(m_Device.EndpointClearStall(endpointAddr));
        EXPECT_EQ(transfer.Buffer, m_Storage.get());
        EXPECT_EQ(transfer.Generation, 17u);
        EXPECT_EQ(transfer.BytesTransferred, 32u);
        EXPECT_EQ(transfer.TransferActive, enabled);
        if (directionIn)
        {
            EXPECT_NE(m_InEndpoints[1].DIEPCTL & USB_OTG_DIEPCTL_SD0PID_SEVNFRM, 0u);
            EXPECT_EQ(m_InEndpoints[1].DIEPCTL & USB_OTG_DIEPCTL_EPENA, 0u);
            EXPECT_EQ(m_InEndpoints[1].DIEPINT, USB_OTG_DIEPINT_INEPNE | USB_OTG_DIEPINT_XFRC);
            EXPECT_EQ(m_InEndpoints[1].DIEPTSIZ, 32u);
            EXPECT_EQ(m_InEndpoints[1].DIEPDMA, 0x12345678u);
        }
        else
        {
            EXPECT_NE(m_OutEndpoints[1].DOEPCTL & USB_OTG_DOEPCTL_SD0PID_SEVNFRM, 0u);
            EXPECT_EQ(m_OutEndpoints[1].DOEPCTL & USB_OTG_DOEPCTL_EPENA, 0u);
            EXPECT_EQ(m_OutEndpoints[1].DOEPINT, USB_OTG_DOEPINT_XFRC);
            EXPECT_EQ(m_OutEndpoints[1].DOEPTSIZ, 32u);
            EXPECT_EQ(m_OutEndpoints[1].DOEPDMA, 0x12345678u);
            EXPECT_EQ((m_DeviceRegisters.DCTL & USB_OTG_DCTL_CGONAK) != 0, enabled);
        }
    }

    void CancelDevice() { m_Device.CancelEndpointTransfer(m_EndpointOut); }

#ifdef PADOS_MODULE_USB_HOST
    USBHostChannelData& HostTransfer() { return m_Host.m_ChannelStates[0]; }

    bool SubmitHost(const USB_TransferSegment* segments, size_t segmentCount, size_t length)
    {
        return m_Host.SubmitRequest(0, USB_RequestDirection::DEVICE_TO_HOST, USB_TransferType::BULK,
            USBH_InitialTransactionPID::Data, segments, segmentCount, length);
    }

    bool SubmitHost(void* buffer, size_t length, size_t capacity = 0)
    {
        // The single-segment descriptor deliberately dies before retries/continuations use its capacity.
        const USB_TransferSegment segment{buffer, length, capacity};
        return SubmitHost(&segment, 1, length);
    }

    void ExpectHostChunk(size_t length, size_t packetCount, bool direct, const void* buffer)
    {
        const auto& channel = HostTransfer();
        EXPECT_EQ(channel.XferSize, length);
        EXPECT_EQ(channel.TransferDataLength, length);
        EXPECT_EQ(channel.TransferPacketCount, packetCount);
        EXPECT_EQ(channel.DMAUsesBounceBuffer, !direct);
        EXPECT_EQ(m_ChannelRegisters.HCTSIZ & USB_OTG_HCTSIZ_XFRSIZ_Msk, length);
        EXPECT_EQ((m_ChannelRegisters.HCTSIZ & USB_OTG_HCTSIZ_PKTCNT_Msk) >> USB_OTG_HCTSIZ_PKTCNT_Pos, packetCount);
        if (direct) {
            EXPECT_EQ(channel.DMATransferBuffer, buffer);
        }
    }

    bool FinishHost(size_t received, size_t packets, bool complete = true, bool commit = true)
    {
        m_ChannelRegisters.HCTSIZ = (HostTransfer().XferSize - received)
            | ((HostTransfer().TransferPacketCount - packets) << USB_OTG_HCTSIZ_PKTCNT_Pos);
        return m_Host.FinishDMATransfer(0, commit, complete, nullptr);
    }

    void ContinueHost() { m_Host.StartTransfer(0); }

    void CancelHost()
    {
        if (HostTransfer().DMATransferActive)
        {
            HostTransfer().TransferActive = false;
            HostTransfer().CancelHaltPending = true;
            m_Host.CompleteChannelCancellation(0);
        }
    }
#endif

    static constexpr uint8_t m_EndpointOut = USB_MK_OUT_ADDRESS(1);
    static constexpr size_t m_StorageSize = 65536;
    std::unique_ptr<uint8_t, void (*)(void*)> m_Storage{nullptr, &std::free};
    USBReceiveCapacityController m_Controller;
    USBDevice_STM32 m_Device;
    std::array<USB_OTG_OUTEndpointTypeDef, 2> m_OutEndpoints{};
    std::array<USB_OTG_INEndpointTypeDef, 2> m_InEndpoints{};
    USB_OTG_GlobalTypeDef m_GlobalRegisters{};
    USB_OTG_DeviceTypeDef m_DeviceRegisters{};
    bool m_ShortPacket = false;
#ifdef PADOS_MODULE_USB_HOST
    USBHost_STM32 m_Host;
    USB_OTG_HostTypeDef m_HostRegisters{};
    USB_OTG_HostChannelTypeDef m_ChannelRegisters{};
#endif
};

TEST_F(USBReceiveCapacityTest, ToggleResetWritesData0WithoutChangingTransferOwnership)
{
    for (USB_TransferType transferType : {USB_TransferType::BULK, USB_TransferType::INTERRUPT}) {
        for (bool directionIn : {false, true}) {
            for (bool enabled : {false, true}) {
                CheckToggleReset(transferType, directionIn, enabled);
            }
        }
    }
}

TEST_F(USBReceiveCapacityTest, SmallPacketsKeepOnePacketWindows)
{
    for (size_t packetSize : {size_t(8), size_t(16)})
    {
        for (size_t capacity : {size_t(0), packetSize, size_t(31), size_t(32)})
        {
            Configure(packetSize);
            const bool direct = capacity == 32;
            ASSERT_TRUE(SubmitDevice(m_Storage.get(), packetSize, capacity));
            ExpectDeviceChunk(packetSize, 1, direct, m_Storage.get());
            EXPECT_EQ(DeviceTransfer().BufferSize, packetSize);
#ifdef PADOS_MODULE_USB_HOST
            ASSERT_TRUE(SubmitHost(m_Storage.get() + 32, packetSize, capacity));
            ExpectHostChunk(packetSize, 1, direct, m_Storage.get() + 32);
            EXPECT_EQ(HostTransfer().RequestedTransferLength, packetSize);
#endif
        }
    }
}

TEST_F(USBReceiveCapacityTest, MisalignedAndInaccessibleBuffersStillBounce)
{
    for (size_t packetSize : {size_t(8), size_t(16)})
    {
        for (void* buffer : {static_cast<void*>(m_Storage.get() + 1), static_cast<void*>(m_Storage.get() + 4),
                reinterpret_cast<void*>(D1_DTCMRAM_BASE), reinterpret_cast<void*>(D1_ITCMICP_BASE + 32)})
        {
            Configure(packetSize);
            ASSERT_TRUE(SubmitDevice(buffer, packetSize, 32));
            ExpectDeviceChunk(packetSize, 1, false, buffer);
#ifdef PADOS_MODULE_USB_HOST
            ASSERT_TRUE(SubmitHost(buffer, packetSize, 32));
            ExpectHostChunk(packetSize, 1, false, buffer);
#endif
        }
    }
}

TEST_F(USBReceiveCapacityTest, SelectionChecksTheWholeCacheWindow)
{
    EXPECT_EQ(USB_STM32::GetDirectDMAReceiveLength(m_Storage.get(), 16, 16, 31), 0u);
    EXPECT_EQ(USB_STM32::GetDirectDMAReceiveLength(m_Storage.get(), 16, 16, 32), 16u);
    EXPECT_EQ(USB_STM32::GetDirectDMAReceiveLength(m_Storage.get(), 48, 16), 32u);
    EXPECT_EQ(USB_STM32::GetDirectDMAReceiveLength(m_Storage.get(), 48, 16, 64), 48u);
    EXPECT_EQ(USB_STM32::GetDirectDMAReceiveLength(m_Storage.get(), 7, 8, 32), 0u);
    EXPECT_EQ(USB_STM32::GetDirectDMAReceiveLength(m_Storage.get(), 0, 8, 32), 0u);
    // A nominally large capacity does not authorize DMA/cache operations in TCM.
    const void* boundary = reinterpret_cast<void*>(D1_DTCMRAM_BASE - __SCB_DCACHE_LINE_SIZE);
    EXPECT_EQ(USB_STM32::GetDirectDMAReceiveLength(boundary, 8, 8, 64), 8u);
    EXPECT_EQ(USB_STM32::GetDirectDMAReceiveLength(boundary, 40, 8, 64), 0u);
}

TEST_F(USBReceiveCapacityTest, LargerPacketsAndLegacyTailKeepTheirSelection)
{
    for (size_t packetSize : {size_t(32), size_t(64), size_t(128), size_t(512)})
    {
        Configure(packetSize);
        ASSERT_TRUE(SubmitDevice(m_Storage.get(), packetSize));
        ExpectDeviceChunk(packetSize, 1, true, m_Storage.get());
#ifdef PADOS_MODULE_USB_HOST
        ASSERT_TRUE(SubmitHost(m_Storage.get() + 1024, packetSize));
        ExpectHostChunk(packetSize, 1, true, m_Storage.get() + 1024);
#endif
    }
    Configure(16);
    ASSERT_TRUE(SubmitDevice(m_Storage.get(), 48));
    ExpectDeviceChunk(32, 2, true, m_Storage.get());
    ASSERT_TRUE(FinishDevice(32));
    ASSERT_TRUE(ContinueDevice());
    ExpectDeviceChunk(16, 1, false, m_Storage.get() + 32);
#ifdef PADOS_MODULE_USB_HOST
    ASSERT_TRUE(SubmitHost(m_Storage.get() + 1024, 48));
    ExpectHostChunk(32, 2, true, m_Storage.get() + 1024);
    ASSERT_TRUE(FinishHost(32, 2));
    ContinueHost();
    ExpectHostChunk(16, 1, false, m_Storage.get() + 1056);
#endif
}

TEST_F(USBReceiveCapacityTest, CompletionCountsPayloadAndCancellationClearsDeviceCapacity)
{
    for (size_t packetSize : {size_t(8), size_t(16)})
    {
        for (size_t received : {size_t(0), packetSize - 1, packetSize})
        {
            Configure(packetSize);
            ASSERT_TRUE(SubmitDevice(m_Storage.get(), packetSize, 32));
            ASSERT_TRUE(FinishDevice(received));
            EXPECT_EQ(DeviceTransfer().BytesTransferred, received);
            EXPECT_EQ(m_ShortPacket, received < packetSize);
            CancelDevice();
            EXPECT_EQ(DeviceTransfer().ReceiveCapacity, 0u);
            EXPECT_FALSE(DeviceTransfer().DMATransferActive);
#ifdef PADOS_MODULE_USB_HOST
            ASSERT_TRUE(SubmitHost(m_Storage.get() + 32, packetSize, 32));
            ASSERT_TRUE(FinishHost(received, 1));
            EXPECT_EQ(HostTransfer().BytesTransferred, received);
            EXPECT_EQ(HostTransfer().ShortPacketReceived, received < packetSize);
#endif
        }
        Configure(packetSize);
        ASSERT_TRUE(SubmitDevice(m_Storage.get(), packetSize, 32));
        CancelDevice();
        ASSERT_TRUE(SubmitDevice(m_Storage.get(), packetSize));
        ExpectDeviceChunk(packetSize, 1, false, m_Storage.get());
#ifdef PADOS_MODULE_USB_HOST
        ASSERT_TRUE(SubmitHost(m_Storage.get() + 32, packetSize, 32));
        ASSERT_TRUE(FinishHost(0, 0, false));
        ContinueHost();
        ExpectHostChunk(packetSize, 1, true, m_Storage.get() + 32);
        CancelHost();
        EXPECT_FALSE(HostTransfer().DMATransferActive);
        EXPECT_FALSE(HostTransfer().CancelHaltPending);
        EXPECT_EQ(HostTransfer().BytesTransferred, 0u);
        ASSERT_TRUE(SubmitHost(m_Storage.get() + 32, packetSize));
        ExpectHostChunk(packetSize, 1, false, m_Storage.get() + 32);
#endif
    }
}

TEST_F(USBReceiveCapacityTest, SpareCapacityDoesNotAcceptOversizedPayloads)
{
    Configure(8);
    ASSERT_TRUE(SubmitDevice(m_Storage.get(), 7, 32));
    EXPECT_FALSE(FinishDevice(8));
    EXPECT_EQ(DeviceTransfer().BytesTransferred, 0u);
#ifdef PADOS_MODULE_USB_HOST
    ASSERT_TRUE(SubmitHost(m_Storage.get() + 32, 7, 32));
    EXPECT_FALSE(FinishHost(8, 1));
    EXPECT_EQ(HostTransfer().BytesTransferred, 0u);
#endif
    Configure(16);
    EXPECT_FALSE(SubmitDevice(m_Storage.get(), 16, 8));
#ifdef PADOS_MODULE_USB_HOST
    EXPECT_FALSE(SubmitHost(m_Storage.get() + 32, 16, 8));
#endif
}

TEST_F(USBReceiveCapacityTest, HardwareLimitsRetainRemainingOwnedCapacity)
{
    Configure(64);
    const size_t devicePackets = std::min<size_t>(USB_OTG_DOEPTSIZ_PKTCNT_Msk >> USB_OTG_DOEPTSIZ_PKTCNT_Pos,
        USB_OTG_DOEPTSIZ_XFRSIZ_Msk / 64);
    const size_t deviceLength = devicePackets * 64;
    ASSERT_LE(deviceLength + 64, m_StorageSize);
    ASSERT_TRUE(SubmitDevice(m_Storage.get(), deviceLength + 64, deviceLength + 64));
    ExpectDeviceChunk(deviceLength, devicePackets, true, m_Storage.get());
    ASSERT_TRUE(FinishDevice(deviceLength));
    ASSERT_TRUE(ContinueDevice());
    ExpectDeviceChunk(64, 1, true, m_Storage.get() + deviceLength);
    CancelDevice();
#ifdef PADOS_MODULE_USB_HOST
    const size_t hostPackets = HostTransfer().MaxDMAPacketCount;
    const size_t hostLength = hostPackets * 64;
    ASSERT_LE(hostLength + 64, m_StorageSize);
    ASSERT_TRUE(SubmitHost(m_Storage.get(), hostLength + 64, hostLength + 64));
    ExpectHostChunk(hostLength, hostPackets, true, m_Storage.get());
    ASSERT_TRUE(FinishHost(hostLength, hostPackets));
    ContinueHost();
    ExpectHostChunk(64, 1, true, m_Storage.get() + hostLength);
#endif
}

#ifdef PADOS_MODULE_USB_HOST
TEST_F(USBReceiveCapacityTest, VectorOffsetsAndBoundariesKeepIndependentCapacity)
{
    Configure(16);
    std::array<USB_TransferSegment, 2> segments{{{m_Storage.get(), 48, 64}, {m_Storage.get() + 64, 16}}};
    ASSERT_TRUE(SubmitHost(segments.data(), segments.size(), 64));
    ExpectHostChunk(48, 3, true, m_Storage.get());
    // A partial transaction error commits two packets before the HAL retries the remainder.
    ASSERT_TRUE(FinishHost(32, 2, false));
    ContinueHost();
    ExpectHostChunk(16, 1, true, m_Storage.get() + 32);
    ASSERT_TRUE(FinishHost(16, 1));
    ContinueHost();
    ExpectHostChunk(16, 1, false, m_Storage.get() + 64);
    ASSERT_TRUE(FinishHost(16, 1));
    EXPECT_EQ(HostTransfer().BytesTransferred, 64u);

    Configure(8);
    // Adjacent segments alone cannot grant ownership of their common cache line.
    segments = {{{m_Storage.get(), 8}, {m_Storage.get() + 8, 8}}};
    ASSERT_TRUE(SubmitHost(segments.data(), segments.size(), 16));
    ExpectHostChunk(8, 1, false, m_Storage.get());
    ASSERT_TRUE(FinishHost(8, 1));
    ContinueHost();
    ExpectHostChunk(8, 1, false, m_Storage.get() + 8);
    ASSERT_TRUE(FinishHost(8, 1));
}
#endif

#ifdef PADOS_MODULE_USB_HOST
class USBHostCDCReceiveController : public USB_STM32
{
public:
    bool HostSubmitRequest(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType endpointType,
        USBH_InitialTransactionPID initialPID, const USB_TransferSegment* segments, size_t segmentCount, size_t length) override
    {
        EXPECT_FALSE(m_Active);
        EXPECT_EQ(direction, USB_RequestDirection::DEVICE_TO_HOST);
        EXPECT_EQ(segmentCount, 1u);
        EXPECT_EQ(segments[0].Length, length);
        EXPECT_EQ(segments[0].ReceiveCapacity, std::max<size_t>(length, __SCB_DCACHE_LINE_SIZE));
        m_Segment = segments[0];
        m_Active = true;
        return true;
    }

    bool HaltChannel(USB_PipeIndex pipeIndex) override
    {
        if (m_OnHalt) {
            m_OnHalt();
        }
        m_Active = false;
        return true;
    }

    USB_TransferSegment m_Segment;
    bool m_Active = false;
    std::function<void()> m_OnHalt;
};

class USBHostCDCReceiveTest : public ::testing::TestWithParam<size_t>
{
protected:
    void SetUp() override
    {
        m_Channel = ptr_new<USBHostCDCChannel>(&m_Host, nullptr);
        m_File = ptr_new<KFileNode>(O_RDONLY | O_NONBLOCK);
        CRITICAL_SCOPE(m_Host.GetMutex());
        m_Host.m_Driver = &m_Controller;
        m_Channel->m_Buffers.emplace(GetParam(), GetParam());
        m_Channel->m_DataEndpointInSize = GetParam();
        m_Channel->m_DataPipeIn = m_Host.AllocPipe(USB_MK_IN_ADDRESS(1));
        m_Channel->m_IsActive = true;
        m_Channel->StartReceive_pl();
    }

    void TearDown() override
    {
        CRITICAL_SCOPE(m_Host.GetMutex());
        m_Channel->Close();
    }

    void Complete(USB_URBState state, size_t length)
    {
        CRITICAL_SCOPE(m_Host.GetMutex());
        ASSERT_TRUE(m_Controller.m_Active);
        m_Controller.m_Active = false;
        const USB_PipeIndex pipeIndex = m_Channel->m_DataPipeIn;
        const USB_TransactionCallback callback = m_Host.m_Pipes[pipeIndex].TransactionCallback;
        ASSERT_TRUE(callback);
        callback(pipeIndex, state, length);
    }

    void ExpectReserved()
    {
        EXPECT_NE(m_Channel->m_ReceiveBuffer, nullptr);
        EXPECT_EQ(m_Channel->m_Buffers->GetReceiveQueue().BeginReceive(), nullptr);
    }

    void Close()
    {
        CRITICAL_SCOPE(m_Host.GetMutex());
        m_Channel->Close();
        EXPECT_EQ(m_Channel->m_ReceiveBuffer, nullptr);
    }

    USBHostCDCReceiveController m_Controller;
    USBHost m_Host;
    Ptr<USBHostCDCChannel> m_Channel;
    Ptr<KFileNode> m_File;
    std::array<uint8_t, 64> m_Data{};
};

TEST_P(USBHostCDCReceiveTest, RetryKeepsReservationAndPublishesOnlyCompletedPayload)
{
    void* originalBuffer = m_Controller.m_Segment.Buffer;
    EXPECT_EQ(m_Controller.m_Segment.Length, GetParam());
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), m_Data.size(), 0), 0u);
    Complete(USB_URBState::NotReady, 0);
    EXPECT_EQ(m_Controller.m_Segment.Buffer, originalBuffer);
    ExpectReserved();
    std::memset(originalBuffer, 0x5a, GetParam() - 1);
    Complete(USB_URBState::Done, GetParam() - 1);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), m_Data.size(), 0), GetParam() - 1);
    EXPECT_EQ(m_Data[0], 0x5a);
    Complete(USB_URBState::Done, 0);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), m_Data.size(), 0), 0u);
    m_Controller.m_OnHalt = [this] { ExpectReserved(); };
    Close();
    EXPECT_FALSE(m_Controller.m_Active);
}

TEST_P(USBHostCDCReceiveTest, OversizedCompletionReleasesWithoutPublishing)
{
    Complete(USB_URBState::Done, GetParam() + 1);
    EXPECT_FALSE(m_Controller.m_Active);
    EXPECT_THROW(m_Channel->Read(m_File, m_Data.data(), m_Data.size(), 0), std::system_error);
}

INSTANTIATE_TEST_SUITE_P(PacketSizes, USBHostCDCReceiveTest, ::testing::Values(size_t(8), size_t(16), size_t(64)));
#endif

} // namespace kernel
