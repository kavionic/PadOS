// This file is part of PadOS.
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <functional>
#include <system_error>
#include <System/ExceptionHandling.h>

#include <gtest/gtest.h>
#include <Kernel/HAL/STM32/USB_STM32.h>
#include <Kernel/KThreadWaitNode.h>
#include <Kernel/USB/USBDevice.h>
#include <Kernel/USB/ClassDrivers/USBClientClassCDC.h>
#include <Kernel/USB/ClassDrivers/USBClientCDCChannel.h>
#include <Kernel/VFS/KFileHandle.h>

namespace kernel
{

// USBDevice, CDC transfer/recovery handlers, queues and wait listeners are real.
// The controller is modeled; channel opening below omits VFS registration and permanent threads.
class USBClientCDCRecoveryController : public USB_STM32
{
public:
    struct Transfer
    {
        uint8_t* Buffer = nullptr;
        size_t Length = 0;
        size_t ReceiveCapacity = 0;
        size_t Submissions = 0;
        size_t Clears = 0;
        bool Active = false;
        bool Halted = false;
        bool Data1 = false;
        bool Open = false;
        size_t ToggleResets = 0;
    };

    Transfer& GetTransfer(uint8_t endpointAddr)
    {
        const size_t directionOffset = ((endpointAddr & USB_ADDRESS_DIR_IN) != 0) ? USB_ADDRESS_MAX_EP_COUNT : 0;
        return m_Transfers[USB_ADDRESS_EPNUM(endpointAddr) + directionOffset];
    }

    void SetReady(bool ready)
    {
        m_Ready = ready;
        if (!ready) {
            EndpointCloseAll();
        }
    }

    void FailNextSubmission() { m_FailNextSubmission = true; }
    void FailNextDisable() { m_FailNextDisable = true; }
    void FailNextClear() { m_FailNextClear = true; }
    void SetOnCloseAll(const std::function<void()>& callback) { m_OnCloseAll = callback; }
    void SetOnClear(const std::function<void()>& callback) { m_OnClear = callback; }

    bool EndpointOpen(const USB_DescEndpoint& descriptor) override
    {
        if (!m_Ready) {
            return false;
        }
        Transfer& transfer = GetTransfer(descriptor.bEndpointAddress);
        transfer.Open = true;
        transfer.Halted = false;
        transfer.Data1 = false;
        ++transfer.ToggleResets;
        return true;
    }

    void EndpointStall(uint8_t endpointAddr) override
    {
        if (m_FailNextDisable)
        {
            m_FailNextDisable = false;
            SetReady(false);
            IRQDeviceRecoveryNeeded();
        }
        else
        {
            Transfer& transfer = GetTransfer(endpointAddr);
            transfer.Active = false;
            transfer.Halted = true;
        }
    }

    bool EndpointClearStall(uint8_t endpointAddr) override
    {
        if (m_FailNextClear)
        {
            m_FailNextClear = false;
            SetReady(false);
            IRQDeviceRecoveryNeeded();
        }
        Transfer& transfer = GetTransfer(endpointAddr);
        if (!m_Ready || !transfer.Open) {
            return false;
        }
        transfer.Halted = false;
        transfer.Data1 = false;
        ++transfer.Clears;
        ++transfer.ToggleResets;
        if (m_OnClear) {
            m_OnClear();
        }
        return m_Ready;
    }

    bool EndpointTransfer(uint8_t endpointAddr, void* buffer, size_t totalLength, size_t receiveCapacity = 0) override
    {
        Transfer& transfer = GetTransfer(endpointAddr);
        if (!m_Ready || m_FailNextSubmission || transfer.Active || transfer.Halted
            || (USB_ADDRESS_EPNUM(endpointAddr) != 0 && !transfer.Open))
        {
            m_FailNextSubmission = false;
            return false;
        }
        transfer.Buffer = static_cast<uint8_t*>(buffer);
        transfer.Length = totalLength;
        transfer.ReceiveCapacity = receiveCapacity;
        if (endpointAddr == USB_MK_OUT_ADDRESS(1)) {
            EXPECT_EQ(receiveCapacity, std::max<size_t>(totalLength, __SCB_DCACHE_LINE_SIZE));
        }
        transfer.Active = true;
        ++transfer.Submissions;
        return true;
    }

    void EndpointClose(uint8_t endpointAddr) override
    {
        GetTransfer(endpointAddr).Active = false;
        GetTransfer(endpointAddr).Open = false;
    }

    void EndpointCloseAll() override
    {
        for (uint8_t endpointNumber = 1; endpointNumber < USB_ADDRESS_MAX_EP_COUNT; ++endpointNumber)
        {
            EndpointClose(USB_MK_OUT_ADDRESS(endpointNumber));
            EndpointClose(USB_MK_IN_ADDRESS(endpointNumber));
        }
        if (m_OnCloseAll) {
            m_OnCloseAll();
        }
    }

private:
    bool DisableIRQDelivery() override { return false; }
    void RestoreIRQDelivery(bool wasEnabled) override {}

    std::array<Transfer, USB_ADDRESS_MAX_EP_COUNT * 2> m_Transfers{};
    bool m_Ready = true;
    bool m_FailNextSubmission = false;
    bool m_FailNextDisable = false;
    bool m_FailNextClear = false;
    std::function<void()> m_OnCloseAll;
    std::function<void()> m_OnClear;
};

class USBClientCDCRecoveryClassDriver : public USBClientClassCDC
{
public:
    const USB_DescriptorHeader* Open(const USB_DescInterface* interfaceDesc, const void* endDesc) override
    {
        return m_OpenHandler(interfaceDesc, endDesc);
    }

    std::function<const USB_DescriptorHeader*(const USB_DescInterface*, const void*)> m_OpenHandler;
};

class USBClientCDCRecoveryTest : public ::testing::TestWithParam<uint16_t>
{
protected:
    void SetUp() override
    {
        m_PacketSize = GetParam();
        m_ClassDriver = ptr_new<USBClientCDCRecoveryClassDriver>();
        m_ClassDriver->m_OpenHandler = [this](const USB_DescInterface* interfaceDesc, const void* endDesc)
        {
            return OpenChannel(interfaceDesc, endDesc);
        };
        m_File = ptr_new<KFileNode>(O_RDWR | O_NONBLOCK);
        m_ClassDriver->USBClassDriverDevice::Init(&m_Device);
        m_Controller.IRQDeviceRecoveryNeeded.Connect(&m_Device, &USBDevice::IRQDeviceRecoveryNeeded);
        m_Controller.IRQBusResetStarted.Connect(&m_Device, &USBDevice::IRQBusResetStarted);
        m_Controller.IRQSessionEnded.Connect(&m_Device, &USBDevice::IRQSessionEnded);
        AddDescriptor(USB_DescConfiguration(0, 4, m_ConfigurationValue, 0, USB_DescConfiguration::ATTRIBUTES_RESERVED_HIGH, 100));
        AddChannelDescriptors(0, m_EndpointNotification, m_EndpointOut, m_EndpointIn);
        AddChannelDescriptors(2, m_SecondNotification, m_SecondOut, m_SecondIn);

        CRITICAL_SCOPE(m_Device.GetMutex());
        m_Device.m_Driver = &m_Controller;
        m_Device.m_SelectedSpeed = USB_Speed::FULL;
        m_Device.m_DispatchEventValid.store(true, std::memory_order_relaxed);
        m_Device.m_ClassDrivers.push_back(m_ClassDriver);
        m_Device.m_ControlTransfer.Setup(&m_Device, &m_Controller, m_PacketSize);
        ASSERT_TRUE(m_Device.HandleSelectConfiguration(m_ConfigurationValue));
        m_Channel = m_ClassDriver->m_Channels[0];
        m_SecondChannel = m_ClassDriver->m_Channels[1];
    }

    void TearDown() override
    {
        m_Controller.SetOnCloseAll({});
        m_Controller.SetOnClear({});
        m_ClassDriver->Reset();
        CRITICAL_SCOPE(m_Device.GetMutex());
        m_Device.m_ClassDrivers.clear();
        m_Device.m_InterfaceToDriverMap.clear();
        m_Device.m_EndpointToDriverMap.clear();
        m_ClassDriver->USBClassDriverDevice::Shutdown();
    }

    bool Request(USB_RequestCode requestCode, uint8_t endpointAddr)
    {
        return StandardRequest(
            USB_RequestRecipient::ENDPOINT,
            requestCode,
            std::to_underlying(USB_RequestFeatureSelector::ENDPOINT_HALT),
            endpointAddr
        );
    }

    bool StandardRequest(USB_RequestRecipient recipient, USB_RequestCode requestCode, uint16_t value, uint16_t index)
    {
        CRITICAL_SCOPE(m_Device.GetMutex());
        // Reproduce SETUP dispatch and EP0 ownership reset without running the permanent device thread.
        m_Device.m_DispatchEventValid.store(true, std::memory_order_relaxed);
        for (uint8_t controlEndpoint : {USB_MK_OUT_ADDRESS(0), USB_MK_IN_ADDRESS(0)})
        {
            m_Controller.GetTransfer(controlEndpoint).Active = false;
            m_Device.GetEndpoint(controlEndpoint).Reset();
        }
        USB_ControlRequest request(
            recipient,
            USB_RequestType::STANDARD,
            USB_RequestDirection::HOST_TO_DEVICE,
            std::to_underlying(requestCode),
            value,
            index,
            0
        );
        return m_Device.HandleControlRequest(request);
    }

    template<typename Descriptor>
    void AddDescriptor(const Descriptor& descriptor)
    {
        m_Device.AddConfigDescriptor(0, &descriptor, sizeof(descriptor));
    }

    void AddChannelDescriptors(uint8_t interfaceNum, uint8_t notification, uint8_t endpointOut, uint8_t endpointIn)
    {
        AddDescriptor(USB_DescInterface(
            interfaceNum,
            0,
            1,
            USB_ClassCode::CDC,
            std::to_underlying(USB_CDC_CommSubclassType::ABSTRACT_CONTROL_MODEL),
            0,
            0
        ));
        AddDescriptor(USB_DescEndpoint(
            notification,
            USB_TransferType::INTERRUPT,
            USB_IsoEndpointSyncType::NONE,
            USB_EndpointUsageType::DATA,
            8,
            16
        ));
        AddDescriptor(USB_DescInterface(interfaceNum + 1, 0, 2, USB_ClassCode::CDC_DATA, 0, 0, 0));
        AddDescriptor(USB_DescEndpoint(
            endpointOut,
            USB_TransferType::BULK,
            USB_IsoEndpointSyncType::NONE,
            USB_EndpointUsageType::DATA,
            m_PacketSize,
            0
        ));
        AddDescriptor(USB_DescEndpoint(
            endpointIn,
            USB_TransferType::BULK,
            USB_IsoEndpointSyncType::NONE,
            USB_EndpointUsageType::DATA,
            m_PacketSize,
            0
        ));
    }

    const USB_DescriptorHeader* OpenChannel(const USB_DescInterface* interfaceDesc, const void* endDesc)
    {
        // Test-only opening omits device nodes. Configuration parsing, endpoint opening and CDC Reset() remain real.
        const auto* notification = static_cast<const USB_DescEndpoint*>(interfaceDesc->GetNext());
        const auto* dataInterface = static_cast<const USB_DescInterface*>(notification->GetNext());
        uint8_t endpointOut = 0;
        uint8_t endpointIn = 0;
        uint16_t outSize = 0;
        uint16_t inSize = 0;
        if (!m_Device.OpenEndpoint(*notification)) {
            return nullptr;
        }
        const USB_DescriptorHeader* next = m_Device.OpenEndpointPair(
            dataInterface->GetNext(),
            USB_TransferType::BULK,
            endpointOut,
            endpointIn,
            outSize,
            inSize
        );
        if (next == nullptr || next > endDesc) {
            return nullptr;
        }
        Ptr<USBClientCDCChannel> channel = ptr_new<USBClientCDCChannel>(
            &m_Device,
            notification->bEndpointAddress,
            endpointOut,
            endpointIn,
            outSize,
            inSize
        );
        m_ClassDriver->m_Channels.push_back(channel);
        m_ClassDriver->m_InterfaceToChannelMap[interfaceDesc->bInterfaceNumber] = channel;
        m_ClassDriver->m_InterfaceToChannelMap[dataInterface->bInterfaceNumber] = channel;
        m_ClassDriver->m_EndpointToChannelMap[endpointOut] = channel;
        m_ClassDriver->m_EndpointToChannelMap[endpointIn] = channel;
        channel->HandleEndpointHaltCleared(endpointOut);
        return next;
    }

    bool SelectConfiguration(uint16_t value)
    {
        return StandardRequest(USB_RequestRecipient::DEVICE, USB_RequestCode::SET_CONFIGURATION, value, 0);
    }

    bool SelectInterface(uint16_t interfaceNum, uint16_t alternate = 0)
    {
        return StandardRequest(USB_RequestRecipient::INTERFACE, USB_RequestCode::SET_INTERFACE, alternate, interfaceNum);
    }

    void DispatchQueuedCompletion()
    {
        CRITICAL_SCOPE(m_Device.GetMutex());
        USBDeviceEvent event;
        ASSERT_EQ(m_Device.m_EventQueue.Read(&event, 1), 1u);
        ASSERT_EQ(event.EventID, USBDeviceEventID::TransferComplete);
        const auto& completion = event.TransferComplete;
        USBEndpointState& endpoint = m_Device.GetEndpoint(completion.EndpointAddr);
        ASSERT_TRUE(endpoint.Busy);
        endpoint.Busy = false;
        endpoint.Claimed = false;
        m_ClassDriver->HandleDataTransfer(completion.EndpointAddr, completion.Result, completion.Length);
    }

    void Halt(uint8_t endpointAddr)
    {
        ASSERT_TRUE(Request(USB_RequestCode::SET_FEATURE, endpointAddr));
        ExpectState(endpointAddr, false, false, true);
    }

    void Clear(uint8_t endpointAddr) { ASSERT_TRUE(Request(USB_RequestCode::CLEAR_FEATURE, endpointAddr)); }

    void ExpectState(uint8_t endpointAddr, bool busy, bool claimed, bool stalled)
    {
        CRITICAL_SCOPE(m_Device.GetMutex());
        const USBEndpointState& endpoint = m_Device.GetEndpoint(endpointAddr);
        EXPECT_EQ(endpoint.Busy, busy);
        EXPECT_EQ(endpoint.Claimed, claimed);
        EXPECT_EQ(endpoint.Stalled, stalled);
    }

    void Complete(uint8_t endpointAddr, USB_TransferResult result, size_t length)
    {
        CRITICAL_SCOPE(m_Device.GetMutex());
        ASSERT_TRUE(m_Controller.GetTransfer(endpointAddr).Active);
        m_Controller.GetTransfer(endpointAddr).Active = false;
        // Reproduce the worker's completion dispatch without running its permanent thread.
        USBEndpointState& endpoint = m_Device.GetEndpoint(endpointAddr);
        ASSERT_TRUE(endpoint.Busy);
        endpoint.Busy = false;
        endpoint.Claimed = false;
        m_ClassDriver->HandleDataTransfer(endpointAddr, result, length);
    }

    void Receive(uint8_t value, size_t length)
    {
        auto& transfer = m_Controller.GetTransfer(m_EndpointOut);
        ASSERT_TRUE(transfer.Active);
        ASSERT_LE(length, transfer.Length);
        std::memset(transfer.Buffer, value, length);
        Complete(m_EndpointOut, USB_TransferResult::Success, length);
    }

    void QueueCompletion(uint8_t endpointAddr, USB_TransferResult result, size_t length)
    {
        CRITICAL_SCOPE(m_Device.GetMutex());
        m_Controller.GetTransfer(endpointAddr).Active = false;
        m_Device.IRQTransferComplete(endpointAddr, length, result);
    }

    size_t GetQueuedEventCount()
    {
        CRITICAL_SCOPE(m_Device.GetMutex());
        return m_Device.m_EventQueue.GetLength();
    }

    void CloseChannel()
    {
        CRITICAL_SCOPE(m_Device.GetMutex());
        m_Channel->Close();
    }

    static void ExpectError(PErrorCode expected, const std::function<void()>& operation)
    {
        try
        {
            operation();
            FAIL() << "Expected an I/O error";
        }
        catch (const std::system_error& error)
        {
            EXPECT_EQ(error.code().value(), std::to_underlying(expected));
        }
    }

    static constexpr uint8_t m_EndpointOut = USB_MK_OUT_ADDRESS(1);
    static constexpr uint8_t m_EndpointIn = USB_MK_IN_ADDRESS(1);
    static constexpr uint8_t m_EndpointNotification = USB_MK_IN_ADDRESS(2);
    static constexpr uint8_t m_SecondOut = USB_MK_OUT_ADDRESS(3);
    static constexpr uint8_t m_SecondIn = USB_MK_IN_ADDRESS(3);
    static constexpr uint8_t m_SecondNotification = USB_MK_IN_ADDRESS(4);
    static constexpr uint8_t m_ConfigurationValue = 7; // Value deliberately differs from descriptor index + 1.
    uint16_t m_PacketSize = 0;

    USBClientCDCRecoveryController m_Controller;
    USBDevice m_Device;
    Ptr<USBClientCDCRecoveryClassDriver> m_ClassDriver;
    Ptr<USBClientCDCChannel> m_Channel;
    Ptr<USBClientCDCChannel> m_SecondChannel;
    Ptr<KFileNode> m_File;
    std::array<uint8_t, 2048> m_Data{};
};

TEST_P(USBClientCDCRecoveryTest, ActiveOutRecoversAndWakesReaders)
{
    KThreadWaitNode listener;
    PScopeExit detachListener([&listener] { listener.Detatch(); });
    ASSERT_TRUE(m_Channel->AddListener(&listener, ObjectWaitMode::Read));
    Halt(m_EndpointOut);
    EXPECT_EQ(listener.GetList(), nullptr);
    EXPECT_FALSE(m_Channel->AddListener(&listener, ObjectWaitMode::Read));
    ExpectError(PErrorCode::IO, [this] { m_Channel->Read(m_File, m_Data.data(), 1, 0); });
    Clear(m_EndpointOut);
    ExpectState(m_EndpointOut, true, true, false);
    ASSERT_TRUE(m_Controller.GetTransfer(USB_MK_IN_ADDRESS(0)).Active);
    EXPECT_EQ(m_Controller.GetTransfer(USB_MK_IN_ADDRESS(0)).Length, 0u);
    const size_t receivedLength = std::min<size_t>(13, m_PacketSize);
    Receive(0x5a, receivedLength);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), m_Data.size(), 0), receivedLength);
    EXPECT_EQ(m_Data[0], 0x5a);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), 1, 0), 0u);
}

TEST_P(USBClientCDCRecoveryTest, FullReceiveQueueRearmsOnlyAfterAWholeBlockIsDrained)
{
    size_t packetCount = 0;
    while (m_Controller.GetTransfer(m_EndpointOut).Active)
    {
        Receive(static_cast<uint8_t>(packetCount), m_PacketSize);
        ++packetCount;
        ASSERT_LT(packetCount, m_Data.size() / m_PacketSize);
    }
    Halt(m_EndpointOut);
    Clear(m_EndpointOut);
    ExpectState(m_EndpointOut, false, false, false);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), m_PacketSize - 1, 0), m_PacketSize - 1);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointOut).Active);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), 1, 0), 1u);
    ExpectState(m_EndpointOut, true, true, false);
    for (size_t i = 1; i < packetCount; ++i)
    {
        EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), m_PacketSize, 0), m_PacketSize);
        EXPECT_EQ(m_Data[0], i);
    }
    Receive(0xa5, 7);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), m_Data.size(), 0), 7u);
    EXPECT_EQ(m_Data[0], 0xa5);
}

TEST_P(USBClientCDCRecoveryTest, ActiveInDropsOnlyTheCanceledBlockAndWakesWriters)
{
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), m_PacketSize, 0), m_PacketSize);
    std::memset(m_Data.data(), 0xa5, m_Data.size());
    const size_t laterLength = m_Channel->Write(m_File, m_Data.data(), m_Data.size(), 0);
    ASSERT_GT(laterLength, 0u);
    ASSERT_LT(laterLength, m_Data.size());
    EXPECT_EQ(m_Channel->Write(m_File, m_Data.data(), 1, 0), 0u);
    KThreadWaitNode listener;
    PScopeExit detachListener([&listener] { listener.Detatch(); });
    ASSERT_TRUE(m_Channel->AddListener(&listener, ObjectWaitMode::Write));
    Halt(m_EndpointIn);
    EXPECT_EQ(listener.GetList(), nullptr);
    ExpectError(PErrorCode::IO, [this] { m_Channel->Write(m_File, m_Data.data(), 1, 0); });
    ExpectError(PErrorCode::IO, [this] { m_Channel->Sync(m_File); });
    Clear(m_EndpointIn);
    const auto& transfer = m_Controller.GetTransfer(m_EndpointIn);
    ASSERT_TRUE(transfer.Active);
    ASSERT_EQ(transfer.Length, laterLength);
    EXPECT_EQ(transfer.Buffer[0], 0xa5);
    Complete(m_EndpointIn, USB_TransferResult::Success, laterLength);
    ASSERT_TRUE(transfer.Active);
    EXPECT_EQ(transfer.Length, 0u);
    Complete(m_EndpointIn, USB_TransferResult::Success, 0);
    EXPECT_FALSE(transfer.Active);
    EXPECT_FALSE(m_Channel->AddListener(&listener, ObjectWaitMode::Write));
}

TEST_P(USBClientCDCRecoveryTest, IdleInPreservesBufferedWritesAndRemembersSync)
{
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), 7, 0), 7u);
    Halt(m_EndpointIn);
    Clear(m_EndpointIn);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointIn).Active);
    Halt(m_EndpointIn);
    m_Channel->Sync(m_File);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointIn).Active);
    Clear(m_EndpointIn);
    ASSERT_TRUE(m_Controller.GetTransfer(m_EndpointIn).Active);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointIn).Length, 7u);
    Complete(m_EndpointIn, USB_TransferResult::Success, 7);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointIn).Active);

    Halt(m_EndpointIn);
    m_File->SetOpenFlags(O_RDWR | O_NONBLOCK | O_SYNC);
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), 9, 0), 9u);
    Clear(m_EndpointIn);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointIn).Length, 9u);
}

TEST_P(USBClientCDCRecoveryTest, CanceledZLPOwnsNoBlockAndRetainsTermination)
{
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), m_PacketSize, 0), m_PacketSize);
    Complete(m_EndpointIn, USB_TransferResult::Success, m_PacketSize);
    ASSERT_EQ(m_Controller.GetTransfer(m_EndpointIn).Length, 0u);
    Halt(m_EndpointIn);
    Clear(m_EndpointIn);
    ASSERT_TRUE(m_Controller.GetTransfer(m_EndpointIn).Active);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointIn).Length, 0u);
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), 7, 0), 7u);
    Halt(m_EndpointIn);
    Clear(m_EndpointIn);
    ASSERT_TRUE(m_Controller.GetTransfer(m_EndpointIn).Active);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointIn).Length, 7u);
    Complete(m_EndpointIn, USB_TransferResult::Success, 7);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointIn).Active);
}

TEST_P(USBClientCDCRecoveryTest, RepeatedClearsResetData0WithoutReleasingAnUnhaltedTransfer)
{
    std::memset(m_Data.data(), 0xa5, m_PacketSize);
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), m_PacketSize, 0), m_PacketSize);
    for (uint8_t endpointAddr : {m_EndpointOut, m_EndpointIn})
    {
        auto& transfer = m_Controller.GetTransfer(endpointAddr);
        uint8_t* originalBuffer = transfer.Buffer;
        const size_t originalSubmissions = transfer.Submissions;
        const size_t originalClears = transfer.Clears;
        const size_t originalResets = transfer.ToggleResets;
        for (size_t repeat = 0; repeat < 3; ++repeat)
        {
            transfer.Data1 = true;
            Clear(endpointAddr);
            EXPECT_FALSE(transfer.Data1);
            EXPECT_EQ(transfer.ToggleResets, originalResets + repeat + 1);
            EXPECT_EQ(transfer.Buffer, originalBuffer);
            EXPECT_EQ(transfer.Submissions, originalSubmissions);
            EXPECT_EQ(transfer.Clears, originalClears + repeat + 1);
            ExpectState(endpointAddr, true, true, false);
        }
    }
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointIn).Buffer[0], 0xa5);
    Complete(m_EndpointIn, USB_TransferResult::Success, m_PacketSize);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointIn).Length, 0u);
    Clear(m_EndpointIn);
    Complete(m_EndpointIn, USB_TransferResult::Success, 0);
    for (size_t i = 0; i < 4; ++i)
    {
        Halt(m_EndpointOut);
        Halt(m_EndpointOut);
        Clear(m_EndpointOut);
        Clear(m_EndpointOut);
        Receive(static_cast<uint8_t>(i), 1);
        EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), 1, 0), 1u);
        EXPECT_EQ(m_Data[0], i);
    }
}

TEST_P(USBClientCDCRecoveryTest, QueuedCompletionsAreConsumedBeforeRearming)
{
    auto& receive = m_Controller.GetTransfer(m_EndpointOut);
    receive.Buffer[0] = 0x5a;
    QueueCompletion(m_EndpointOut, USB_TransferResult::Success, 1);
    ASSERT_EQ(GetQueuedEventCount(), 1u);
    Halt(m_EndpointOut);
    EXPECT_EQ(GetQueuedEventCount(), 0u);
    Clear(m_EndpointOut);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), 1, 0), 1u);
    EXPECT_EQ(m_Data[0], 0x5a);
    Receive(0xa5, 1);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), 1, 0), 1u);
    EXPECT_EQ(m_Data[0], 0xa5);

    QueueCompletion(m_EndpointOut, USB_TransferResult::Failed, 0);
    Halt(m_EndpointOut);
    Clear(m_EndpointOut);
    EXPECT_EQ(GetQueuedEventCount(), 0u);
    EXPECT_FALSE(receive.Active);
    ExpectError(PErrorCode::IO, [this] { m_Channel->Read(m_File, m_Data.data(), 1, 0); });
}

TEST_P(USBClientCDCRecoveryTest, UnrelatedCompletionAndSubmissionErrorsRemainLatched)
{
    Complete(m_EndpointOut, USB_TransferResult::Failed, 0);
    Halt(m_EndpointOut);
    Clear(m_EndpointOut);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointOut).Active);
    ExpectError(PErrorCode::IO, [this] { m_Channel->Read(m_File, m_Data.data(), 1, 0); });

    m_Controller.FailNextSubmission();
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), m_PacketSize, 0), m_PacketSize);
    Halt(m_EndpointIn);
    Clear(m_EndpointIn);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointIn).Active);
    ExpectError(PErrorCode::IO, [this] { m_Channel->Sync(m_File); });
}

TEST_P(USBClientCDCRecoveryTest, FailedRecoverySubmissionIsNotClearedByAnotherHalt)
{
    Halt(m_EndpointOut);
    m_Controller.FailNextSubmission();
    Clear(m_EndpointOut);
    ExpectState(m_EndpointOut, false, false, false);
    Halt(m_EndpointOut);
    Clear(m_EndpointOut);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointOut).Active);
    ExpectError(PErrorCode::IO, [this] { m_Channel->Read(m_File, m_Data.data(), 1, 0); });
}

TEST_P(USBClientCDCRecoveryTest, FailedDisableResetAndDisconnectPreventRearming)
{
    m_Controller.FailNextDisable();
    EXPECT_FALSE(Request(USB_RequestCode::SET_FEATURE, m_EndpointOut));
    ExpectState(m_EndpointOut, false, false, true);
    const size_t submissions = m_Controller.GetTransfer(m_EndpointOut).Submissions;
    EXPECT_FALSE(Request(USB_RequestCode::CLEAR_FEATURE, m_EndpointOut));
    ExpectState(m_EndpointOut, false, false, true);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointOut).Submissions, submissions);
    EXPECT_FALSE(m_Controller.GetTransfer(USB_MK_IN_ADDRESS(0)).Active);
    m_Controller.IRQBusResetStarted();
    EXPECT_FALSE(Request(USB_RequestCode::CLEAR_FEATURE, m_EndpointOut));
    m_Controller.IRQSessionEnded();
    CloseChannel();
    m_Controller.SetReady(true);
    EXPECT_FALSE(Request(USB_RequestCode::CLEAR_FEATURE, m_EndpointOut));
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointOut).Active);
    ExpectError(PErrorCode::PIPE, [this] { m_Channel->Read(m_File, m_Data.data(), 1, 0); });
    ExpectError(PErrorCode::PIPE, [this] { m_Channel->Write(m_File, m_Data.data(), 1, 0); });
}

TEST_P(USBClientCDCRecoveryTest, SpareCapacityPublishesOnlySuccessfulPayload)
{
    const auto& transfer = m_Controller.GetTransfer(m_EndpointOut);
    EXPECT_EQ(transfer.Length, m_PacketSize);
    EXPECT_EQ(transfer.ReceiveCapacity, std::max<size_t>(m_PacketSize, __SCB_DCACHE_LINE_SIZE));
    Receive(0, 0);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), m_Data.size(), 0), 0u);
    Receive(0x5a, m_PacketSize - 1);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), m_Data.size(), 0), m_PacketSize - 1);
    Complete(m_EndpointOut, USB_TransferResult::Success, m_PacketSize + 1);
    EXPECT_FALSE(transfer.Active);
    ExpectError(PErrorCode::IO, [this] { m_Channel->Read(m_File, m_Data.data(), 1, 0); });
}

TEST_P(USBClientCDCRecoveryTest, UnhaltedClearPreservesQueuedPayloadAndUnrelatedErrors)
{
    auto& receive = m_Controller.GetTransfer(m_EndpointOut);
    receive.Buffer[0] = 0x5a;
    QueueCompletion(m_EndpointOut, USB_TransferResult::Success, 1);
    receive.Data1 = true;
    Clear(m_EndpointOut);
    Clear(m_EndpointOut);
    EXPECT_FALSE(receive.Data1);
    EXPECT_EQ(GetQueuedEventCount(), 1u);
    ExpectState(m_EndpointOut, true, true, false);
    DispatchQueuedCompletion();
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), 1, 0), 1u);
    EXPECT_EQ(m_Data[0], 0x5a);

    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), m_PacketSize, 0), m_PacketSize);
    auto& transmit = m_Controller.GetTransfer(m_EndpointIn);
    QueueCompletion(m_EndpointIn, USB_TransferResult::Success, m_PacketSize);
    transmit.Data1 = true;
    Clear(m_EndpointIn);
    EXPECT_FALSE(transmit.Data1);
    EXPECT_EQ(GetQueuedEventCount(), 1u);
    DispatchQueuedCompletion();
    EXPECT_TRUE(transmit.Active);
    EXPECT_EQ(transmit.Length, 0u);
    Complete(m_EndpointIn, USB_TransferResult::Success, 0);

    QueueCompletion(m_EndpointOut, USB_TransferResult::Failed, 0);
    Clear(m_EndpointOut);
    DispatchQueuedCompletion();
    Clear(m_EndpointOut);
    EXPECT_FALSE(receive.Active);
    ExpectError(PErrorCode::IO, [this] { m_Channel->Read(m_File, m_Data.data(), 1, 0); });
}

TEST_P(USBClientCDCRecoveryTest, InterfaceZeroResetsOnlyItsEndpointsAndPreservesChannels)
{
    const auto& otherReceive = m_Controller.GetTransfer(m_SecondOut);
    uint8_t* otherBuffer = otherReceive.Buffer;
    const size_t otherSubmissions = otherReceive.Submissions;
    Halt(m_EndpointNotification);
    Halt(m_EndpointOut);
    Halt(m_EndpointIn);
    Halt(m_SecondIn);
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), 7, 0), 7u);
    m_Channel->Sync(m_File);
    for (uint8_t endpointAddr : {m_EndpointOut, m_EndpointIn, m_EndpointNotification, m_SecondIn}) {
        m_Controller.GetTransfer(endpointAddr).Data1 = true;
    }
    ASSERT_TRUE(SelectInterface(1));
    EXPECT_EQ(m_ClassDriver->GetChannel(0), m_Channel);
    EXPECT_EQ(m_ClassDriver->GetChannel(1), m_SecondChannel);
    ExpectState(m_EndpointOut, true, true, false);
    ExpectState(m_EndpointIn, true, true, false);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointIn).Length, 7u);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointOut).Data1);
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointIn).Data1);
    ExpectState(m_EndpointNotification, false, false, true);
    ExpectState(m_SecondIn, false, false, true);
    EXPECT_TRUE(m_Controller.GetTransfer(m_EndpointNotification).Data1);
    EXPECT_TRUE(m_Controller.GetTransfer(m_SecondIn).Data1);
    EXPECT_EQ(otherReceive.Buffer, otherBuffer);
    EXPECT_EQ(otherReceive.Submissions, otherSubmissions);
    EXPECT_TRUE(otherReceive.Active);
    ASSERT_TRUE(SelectInterface(0));
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointNotification).Data1);
    ExpectState(m_EndpointNotification, false, false, false);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointIn).Length, 7u);
    Complete(m_EndpointIn, USB_TransferResult::Success, 7);
}

TEST_P(USBClientCDCRecoveryTest, InterfaceReselectionPreservesActiveAndQueuedTransfers)
{
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), m_PacketSize, 0), m_PacketSize);
    auto& transmit = m_Controller.GetTransfer(m_EndpointIn);
    uint8_t* originalBuffer = transmit.Buffer;
    const size_t originalSubmissions = transmit.Submissions;
    m_Controller.GetTransfer(m_EndpointOut).Buffer[0] = 0xa5;
    QueueCompletion(m_EndpointOut, USB_TransferResult::Success, 1);
    for (size_t repeat = 0; repeat < 3; ++repeat)
    {
        transmit.Data1 = true;
        ASSERT_TRUE(SelectInterface(1));
        EXPECT_FALSE(transmit.Data1);
        EXPECT_EQ(transmit.Buffer, originalBuffer);
        EXPECT_EQ(transmit.Submissions, originalSubmissions);
        EXPECT_EQ(GetQueuedEventCount(), 1u);
    }
    DispatchQueuedCompletion();
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), 1, 0), 1u);
    EXPECT_EQ(m_Data[0], 0xa5);
    Complete(m_EndpointIn, USB_TransferResult::Success, m_PacketSize);
    ASSERT_TRUE(SelectInterface(1));
    EXPECT_EQ(transmit.Length, 0u);
    Complete(m_EndpointIn, USB_TransferResult::Success, 0);
}

TEST_P(USBClientCDCRecoveryTest, InterfaceReselectionPreservesBufferedWritesAndUnrelatedErrors)
{
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), 7, 0), 7u);
    ASSERT_TRUE(SelectInterface(1));
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointIn).Active);
    m_Channel->Sync(m_File);
    ASSERT_EQ(m_Controller.GetTransfer(m_EndpointIn).Length, 7u);
    Complete(m_EndpointIn, USB_TransferResult::Success, 7);
    Complete(m_EndpointOut, USB_TransferResult::Failed, 0);
    Halt(m_EndpointOut);
    ASSERT_TRUE(SelectInterface(1));
    EXPECT_FALSE(m_Controller.GetTransfer(m_EndpointOut).Active);
    ExpectError(PErrorCode::IO, [this] { m_Channel->Read(m_File, m_Data.data(), 1, 0); });
}

TEST_P(USBClientCDCRecoveryTest, CurrentConfigurationRecreatesChannelsAndResetsEveryEndpoint)
{
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), m_PacketSize, 0), m_PacketSize);
    QueueCompletion(m_EndpointIn, USB_TransferResult::Success, m_PacketSize);
    Halt(m_EndpointOut);
    Halt(m_SecondIn);
    QueueCompletion(USB_MK_IN_ADDRESS(0), USB_TransferResult::Success, 0);
    for (uint8_t endpointAddr : {m_EndpointNotification, m_EndpointOut, m_EndpointIn,
            m_SecondNotification, m_SecondOut, m_SecondIn}) {
        m_Controller.GetTransfer(endpointAddr).Data1 = true;
    }
    ASSERT_TRUE(SelectConfiguration(m_ConfigurationValue));
    EXPECT_EQ(GetQueuedEventCount(), 0u);
    EXPECT_NE(m_ClassDriver->GetChannel(0), m_Channel);
    EXPECT_NE(m_ClassDriver->GetChannel(1), m_SecondChannel);
    ExpectError(PErrorCode::PIPE, [this] { m_Channel->Write(m_File, m_Data.data(), 1, 0); });
    ExpectError(PErrorCode::PIPE, [this] { m_SecondChannel->Read(m_File, m_Data.data(), 1, 0); });
    for (uint8_t endpointAddr : {m_EndpointNotification, m_EndpointOut, m_EndpointIn,
            m_SecondNotification, m_SecondOut, m_SecondIn})
    {
        const auto& transfer = m_Controller.GetTransfer(endpointAddr);
        EXPECT_FALSE(transfer.Halted);
        EXPECT_FALSE(transfer.Data1);
        EXPECT_EQ(transfer.ToggleResets, 2u);
    }
    EXPECT_TRUE(m_Controller.GetTransfer(USB_MK_IN_ADDRESS(0)).Active);
    EXPECT_EQ(m_Controller.GetTransfer(USB_MK_IN_ADDRESS(0)).Length, 0u);
    m_Channel = m_ClassDriver->GetChannel(0);
    m_SecondChannel = m_ClassDriver->GetChannel(1);
    Receive(0x5a, 1);
    EXPECT_EQ(m_Channel->Read(m_File, m_Data.data(), 1, 0), 1u);
    EXPECT_EQ(m_Data[0], 0x5a);
    ASSERT_EQ(m_Channel->Write(m_File, m_Data.data(), m_PacketSize, 0), m_PacketSize);
    Complete(m_EndpointIn, USB_TransferResult::Success, m_PacketSize);
}

TEST_P(USBClientCDCRecoveryTest, UnsupportedSelectionsLeaveBothChannelsUntouched)
{
    auto& receive = m_Controller.GetTransfer(m_EndpointOut);
    uint8_t* originalBuffer = receive.Buffer;
    const size_t originalResets = receive.ToggleResets;
    EXPECT_FALSE(SelectInterface(1, 1));
    EXPECT_FALSE(SelectInterface(4));
    EXPECT_FALSE(SelectInterface(0x101));
    EXPECT_FALSE(SelectConfiguration(1));
    EXPECT_FALSE(SelectConfiguration(0x100 | m_ConfigurationValue));
    EXPECT_EQ(receive.Buffer, originalBuffer);
    EXPECT_EQ(receive.ToggleResets, originalResets);
    EXPECT_EQ(m_ClassDriver->GetChannel(0), m_Channel);
    EXPECT_EQ(m_ClassDriver->GetChannel(1), m_SecondChannel);
    EXPECT_FALSE(m_Controller.GetTransfer(USB_MK_IN_ADDRESS(0)).Active);
}

TEST_P(USBClientCDCRecoveryTest, FailedClearAndSelectionOverlapDoNotSubmitStaleDMA)
{
    m_Controller.FailNextClear();
    const size_t submissions = m_Controller.GetTransfer(m_EndpointOut).Submissions;
    EXPECT_FALSE(Request(USB_RequestCode::CLEAR_FEATURE, m_EndpointOut));
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointOut).Submissions, submissions);
    EXPECT_FALSE(SelectInterface(1));
    EXPECT_FALSE(m_Controller.GetTransfer(USB_MK_IN_ADDRESS(0)).Active);
    EXPECT_FALSE(SelectConfiguration(m_ConfigurationValue));
    EXPECT_EQ(m_ClassDriver->GetChannelCount(), 0u);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointOut).Submissions, submissions);
}

TEST_P(USBClientCDCRecoveryTest, ResetDuringConfigurationCleanupPreventsReopening)
{
    m_Controller.SetOnCloseAll([this] { m_Controller.IRQBusResetStarted(); });
    const size_t submissions = m_Controller.GetTransfer(m_EndpointOut).Submissions;
    EXPECT_FALSE(SelectConfiguration(m_ConfigurationValue));
    EXPECT_EQ(m_ClassDriver->GetChannelCount(), 0u);
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointOut).Submissions, submissions);
    EXPECT_FALSE(m_Controller.GetTransfer(USB_MK_IN_ADDRESS(0)).Active);
}

TEST_P(USBClientCDCRecoveryTest, DisconnectDuringInterfaceResetPreventsStatusAndRearming)
{
    m_Controller.SetOnClear([this]
    {
        m_Controller.SetReady(false);
        m_Controller.IRQSessionEnded();
    });
    Halt(m_EndpointOut);
    const size_t submissions = m_Controller.GetTransfer(m_EndpointOut).Submissions;
    EXPECT_FALSE(SelectInterface(1));
    EXPECT_EQ(m_Controller.GetTransfer(m_EndpointOut).Submissions, submissions);
    EXPECT_FALSE(m_Controller.GetTransfer(USB_MK_IN_ADDRESS(0)).Active);
}

INSTANTIATE_TEST_SUITE_P(PacketSizes, USBClientCDCRecoveryTest, ::testing::Values(uint16_t(8), uint16_t(16), uint16_t(64)));

} // namespace kernel
