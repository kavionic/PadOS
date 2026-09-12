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

#include <algorithm>
#include <array>
#include <limits>
#include <string.h>
#include <system_error>
#include <sys/uio.h>
#include <utility>

#include <PadOS/DeviceControl.h>
#include <PadOS/Filesystem.h>
#include <System/Endian.h>
#include <System/ExceptionHandling.h>
#include <Kernel/KTime.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/USB/USBProtocolMSC.h>
#include <Kernel/USB/ClassDrivers/USBHostClassMSC.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KVFSManager.h>
#include <Utils/String.h>

namespace kernel
{

static constexpr size_t USB_MSC_MAX_TRANSFER_SIZE = 32 * 1024;
static constexpr TimeValNanos USB_MSC_COMMAND_TIMEOUT = TimeValNanos::FromSeconds(30.0);
static constexpr size_t USB_MSC_MAX_TRANSFER_PACKETS = 1023;
static constexpr uint32_t USB_MSC_SUPPORTED_BLOCK_SIZE = 512;
static constexpr size_t USB_MSC_INQUIRY_RESPONSE_SIZE = 36;
static constexpr size_t USB_MSC_REQUEST_SENSE_RESPONSE_SIZE = 18;
static constexpr size_t USB_MSC_READ_CAPACITY_10_RESPONSE_SIZE = 8;
static constexpr size_t USB_MSC_READ_CAPACITY_16_RESPONSE_SIZE = 32;

enum class USBMSCTransactionStage
{
    Idle,
    CommandBlock,
    DataIn,
    DataOut,
    DataHaltClear,
    Status,
    StatusHaltClear,
    Reset,
    ResetClearIn,
    ResetClearOut
};

const char* USBMSCGetTransactionStageName(USBMSCTransactionStage stage)
{
    switch (stage)
    {
        case USBMSCTransactionStage::Idle:           return "idle";
        case USBMSCTransactionStage::CommandBlock:   return "command-block";
        case USBMSCTransactionStage::DataIn:         return "data-in";
        case USBMSCTransactionStage::DataOut:        return "data-out";
        case USBMSCTransactionStage::DataHaltClear:  return "data-halt-clear";
        case USBMSCTransactionStage::Status:         return "status";
        case USBMSCTransactionStage::StatusHaltClear: return "status-halt-clear";
        case USBMSCTransactionStage::Reset:          return "reset";
        case USBMSCTransactionStage::ResetClearIn:   return "reset-clear-in";
        case USBMSCTransactionStage::ResetClearOut:  return "reset-clear-out";
    }
    return "unknown";
}

const char* USBMSCGetURBStateName(USB_URBState state)
{
    switch (state)
    {
        case USB_URBState::Idle:     return "idle";
        case USB_URBState::Done:     return "done";
        case USB_URBState::NotReady: return "not-ready";
        case USB_URBState::Stall:    return "stall";
        case USB_URBState::Error:    return "error";
    }
    return "unknown";
}

struct USBMSCSenseResult
{
    USB_MSC_SCSI_SenseKey SenseKey = USB_MSC_SCSI_SenseKey::NO_SENSE;
    uint8_t AdditionalSenseCode = 0;
    uint8_t AdditionalSenseQualifier = 0;
};

struct USBMSCLogicalUnitCapacity
{
    uint32_t BlockSize = 0;
    uint64_t SectorCount = 0;
};

class USBMSCIOVectorCursor
{
public:
    USBMSCIOVectorCursor() = default;

    USBMSCIOVectorCursor(const iovec_t* segments, size_t segmentCount, size_t totalLength)
        : m_Segments(segments)
        , m_SegmentCount(segmentCount)
        , m_RemainingLength(totalLength)
    {
    }

    USBMSCIOVectorCursor(const USBMSCIOVectorCursor& source, size_t totalLength)
        : m_Segments(source.m_Segments)
        , m_SegmentCount(source.m_SegmentCount)
        , m_SegmentIndex(source.m_SegmentIndex)
        , m_SegmentOffset(source.m_SegmentOffset)
        , m_RemainingLength(totalLength)
    {
    }

    size_t GetRemainingLength() const
    {
        return m_RemainingLength;
    }

    bool IsValid() const
    {
        USBMSCIOVectorCursor cursor = *this;
        while (cursor.m_RemainingLength != 0)
        {
            if (cursor.m_Segments == nullptr) {
                return false;
            }
            cursor.SkipEmptySegments();
            if (cursor.m_SegmentIndex >= cursor.m_SegmentCount || cursor.m_Segments[cursor.m_SegmentIndex].iov_base == nullptr)
            {
                return false;
            }
            const size_t segmentRemaining = cursor.m_Segments[cursor.m_SegmentIndex].iov_len - cursor.m_SegmentOffset;
            cursor.Advance(std::min(segmentRemaining, cursor.m_RemainingLength));
        }
        return true;
    }

    bool GetCurrentSpan(void** buffer, size_t* length)
    {
        if (buffer == nullptr || length == nullptr) {
            return false;
        }
        if (m_Segments == nullptr) {
            return false;
        }

        SkipEmptySegments();
        if (m_RemainingLength == 0 || m_SegmentIndex >= m_SegmentCount || m_Segments[m_SegmentIndex].iov_base == nullptr)
        {
            return false;
        }

        uint8_t* spanStart = static_cast<uint8_t*>(m_Segments[m_SegmentIndex].iov_base) + m_SegmentOffset;
        size_t spanLength = std::min(m_Segments[m_SegmentIndex].iov_len - m_SegmentOffset, m_RemainingLength);
        size_t segmentIndex = m_SegmentIndex + 1;
        while (spanLength < m_RemainingLength && segmentIndex < m_SegmentCount)
        {
            if (m_Segments[segmentIndex].iov_len == 0)
            {
                ++segmentIndex;
                continue;
            }
            if (m_Segments[segmentIndex].iov_base == nullptr ||
                reinterpret_cast<uintptr_t>(m_Segments[segmentIndex].iov_base) != reinterpret_cast<uintptr_t>(spanStart) + spanLength)
            {
                break;
            }
            spanLength += std::min(m_Segments[segmentIndex].iov_len, m_RemainingLength - spanLength);
            ++segmentIndex;
        }

        *buffer = spanStart;
        *length = spanLength;
        return true;
    }

    bool HasPacketAlignedSpans(size_t packetSize) const
    {
        if (packetSize == 0) {
            return false;
        }

        // BOT has one continuous bulk data phase. Every separately submitted
        // span must therefore end on a packet boundary rather than generating
        // an intermediate short packet.
        USBMSCIOVectorCursor cursor = *this;
        while (cursor.GetRemainingLength() != 0)
        {
            void* buffer = nullptr;
            size_t length = 0;
            if (!cursor.GetCurrentSpan(&buffer, &length) || length == 0 || (length % packetSize) != 0) {
                return false;
            }
            cursor.Advance(length);
        }
        return true;
    }

    void CopyFrom(const void* source, size_t length)
    {
        const uint8_t* sourceBytes = static_cast<const uint8_t*>(source);
        size_t remainingCopyLength = length;

        while (remainingCopyLength != 0)
        {
            SkipEmptySegments();
            kassert(m_SegmentIndex < m_SegmentCount);

            const size_t segmentRemaining = m_Segments[m_SegmentIndex].iov_len - m_SegmentOffset;
            const size_t copyLength = std::min(segmentRemaining, remainingCopyLength);
            uint8_t* destination = static_cast<uint8_t*>(m_Segments[m_SegmentIndex].iov_base) + m_SegmentOffset;

            memcpy(destination, sourceBytes, copyLength);
            sourceBytes += copyLength;
            Advance(copyLength);
            remainingCopyLength -= copyLength;
        }
    }

    void CopyTo(void* destination, size_t length)
    {
        uint8_t* destinationBytes = static_cast<uint8_t*>(destination);
        size_t remainingCopyLength = length;

        while (remainingCopyLength != 0)
        {
            SkipEmptySegments();
            kassert(m_SegmentIndex < m_SegmentCount);

            const size_t segmentRemaining = m_Segments[m_SegmentIndex].iov_len - m_SegmentOffset;
            const size_t copyLength = std::min(segmentRemaining, remainingCopyLength);
            const uint8_t* source = static_cast<const uint8_t*>(m_Segments[m_SegmentIndex].iov_base) + m_SegmentOffset;

            memcpy(destinationBytes, source, copyLength);
            destinationBytes += copyLength;
            Advance(copyLength);
            remainingCopyLength -= copyLength;
        }
    }

    void Advance(size_t length)
    {
        kassert(length <= m_RemainingLength);
        while (length != 0)
        {
            kassert(m_Segments != nullptr);
            SkipEmptySegments();
            kassert(m_SegmentIndex < m_SegmentCount);

            const size_t segmentRemaining = m_Segments[m_SegmentIndex].iov_len - m_SegmentOffset;
            const size_t advanceLength = std::min(segmentRemaining, length);
            m_SegmentOffset += advanceLength;
            m_RemainingLength -= advanceLength;
            length -= advanceLength;
        }
        SkipEmptySegments();
    }

private:
    void SkipEmptySegments()
    {
        while (m_Segments != nullptr && m_SegmentIndex < m_SegmentCount &&
            m_SegmentOffset == m_Segments[m_SegmentIndex].iov_len)
        {
            ++m_SegmentIndex;
            m_SegmentOffset = 0;
        }
    }

    const iovec_t* m_Segments = nullptr;
    size_t         m_SegmentCount = 0;
    size_t         m_SegmentIndex = 0;
    size_t         m_SegmentOffset = 0;
    size_t         m_RemainingLength = 0;
};

uint32_t USBMSCLoadBigEndian32(const uint8_t* data)
{
    return (uint32_t(data[0]) << 24) | (uint32_t(data[1]) << 16) | (uint32_t(data[2]) << 8) | data[3];
}

uint64_t USBMSCLoadBigEndian64(const uint8_t* data)
{
    return (uint64_t(USBMSCLoadBigEndian32(data)) << 32) | USBMSCLoadBigEndian32(data + 4);
}

void USBMSCStoreBigEndian16(uint8_t* data, uint16_t value)
{
    data[0] = uint8_t(value >> 8);
    data[1] = uint8_t(value);
}

void USBMSCStoreBigEndian32(uint8_t* data, uint32_t value)
{
    data[0] = uint8_t(value >> 24);
    data[1] = uint8_t(value >> 16);
    data[2] = uint8_t(value >> 8);
    data[3] = uint8_t(value);
}

class USBHostMSCBlockDevice;

class USBHostMSCInterface : public PtrTarget
{
public:
    USBHostMSCInterface(USBHost* host, USBHostClassMSC* classDriver);
    virtual ~USBHostMSCInterface();

    const USB_DescriptorHeader* Open_pl(uint8_t deviceAddress, const USB_DescInterface* interfaceDesc, const void* endDesc);
    std::vector<int> Close_pl();
    void Startup_pl();

    uint8_t GetDeviceAddress() const;
    bool IsActive_pl() const;
    bool IsConnected();

    void Initialize();
    size_t Transfer(bool write, uint8_t logicalUnitNumber, uint32_t blockSize, uint64_t startBlock, const iovec_t* segments, size_t segmentCount, size_t length);
    void SynchronizeCache(uint8_t logicalUnitNumber);

private:
    bool GetMaximumLogicalUnitNumber_pl(uint8_t* maximumLogicalUnitNumber);
    bool ProbeLogicalUnit_pl(uint8_t logicalUnitNumber, USBMSCLogicalUnitCapacity* capacity);
    PErrorCode ExecuteCommand_pl(uint8_t logicalUnitNumber, const uint8_t* commandBlock, size_t commandBlockLength, USB_MSC_DataDirection direction, void* data, size_t dataLength, size_t* transferredLength, USBMSCSenseResult* senseResult);
    PErrorCode ExecuteVectorCommand_pl(uint8_t logicalUnitNumber, const uint8_t* commandBlock, size_t commandBlockLength, USB_MSC_DataDirection direction, const USBMSCIOVectorCursor& dataCursor, size_t dataLength, size_t* transferredLength, USBMSCSenseResult* senseResult);
    PErrorCode RequestSense_pl(uint8_t logicalUnitNumber, USBMSCSenseResult* senseResult);
    PErrorCode RunTransaction_pl(uint8_t logicalUnitNumber, const uint8_t* commandBlock, size_t commandBlockLength, USB_MSC_DataDirection direction, const USBMSCIOVectorCursor& dataCursor, size_t dataLength, size_t* transferredLength, USB_MSC_CommandStatus* commandStatus);

    bool StartCommandBlock_pl();
    bool StartDataTransfer_pl();
    bool StartStatusTransfer_pl();
    bool SubmitBulkOut_pl(void* buffer, size_t length);
    bool SubmitBulkIn_pl(void* buffer, size_t length);
    void StartDataHaltClear_pl();
    void StartStatusHaltClear_pl();
    void StartResetRecovery_pl(PErrorCode result);
    void FailResetRecovery_pl(bool cancelPipes = true);
    void CompleteTransaction_pl(PErrorCode result, USB_MSC_CommandStatus commandStatus);

    void HandleBulkTransfer_pl(USB_PipeIndex pipeIndex, USB_URBState state, size_t transactionLength);
    void HandleDataHaltClear_pl(bool result, uint8_t deviceAddress);
    void HandleStatusHaltClear_pl(bool result, uint8_t deviceAddress);
    void HandleReset_pl(bool result, uint8_t deviceAddress);
    void HandleResetClearIn_pl(bool result, uint8_t deviceAddress);
    void HandleResetClearOut_pl(bool result, uint8_t deviceAddress);
    void HandleGetMaximumLogicalUnitNumber_pl(bool result, uint8_t deviceAddress);

    std::vector<Ptr<USBHostMSCBlockDevice>> CreateDeviceNodes(const std::vector<USBMSCLogicalUnitCapacity>& capacities);
    void AttachDeviceNodes_pl(std::vector<Ptr<USBHostMSCBlockDevice>>&& devices);
    static void ReadPartitionData(void* userData, off64_t position, void* buffer, size_t size);

    USBHost*            m_Host = nullptr;
    USBHostClassMSC*    m_ClassDriver = nullptr;

    KMutex              m_CommandMutex;
    KConditionVariable  m_TransactionCondition;

    uint8_t             m_DeviceAddress = 0;
    uint8_t             m_InterfaceNumber = 0;
    uint8_t             m_BulkEndpointIn = USB_INVALID_ENDPOINT;
    uint8_t             m_BulkEndpointOut = USB_INVALID_ENDPOINT;
    size_t              m_BulkEndpointInSize = 0;
    size_t              m_BulkEndpointOutSize = 0;
    USB_PipeIndex       m_BulkPipeIn = USB_INVALID_PIPE;
    USB_PipeIndex       m_BulkPipeOut = USB_INVALID_PIPE;
    bool                m_IsConnected = false;
    bool                m_IsStarted = false;

    alignas(4) USB_MSC_CommandBlockWrapper  m_CommandBlockWrapper;
    alignas(4) USB_MSC_CommandStatusWrapper m_CommandStatusWrapper;
    USBMSCTransactionStage       m_TransactionStage = USBMSCTransactionStage::Idle;
    PErrorCode                   m_TransactionResult = PErrorCode::Success;
    PErrorCode                   m_RecoveryResult = PErrorCode::IO;
    bool                         m_TransportRecoveryCompleted = false;
    USB_MSC_CommandStatus        m_TransactionCommandStatus = USB_MSC_CommandStatus::COMMAND_PASSED;
    USBMSCIOVectorCursor         m_TransactionDataCursor;
    size_t                       m_TransactionDataLength = 0;
    size_t                       m_TransactionTransferredLength = 0;
    size_t                       m_TransactionRequestLength = 0;
    USB_PipeIndex                m_StalledDataPipe = USB_INVALID_PIPE;
    USB_PipeIndex                m_LastTransportPipe = USB_INVALID_PIPE;
    USB_URBState                 m_LastTransportURBState = USB_URBState::Idle;
    size_t                       m_LastTransportLength = 0;
    uint32_t                     m_NextTag = 1;
    bool                         m_TransactionActive = false;
    bool                         m_StatusRetryUsed = false;

    alignas(4) uint8_t          m_MaximumLogicalUnitNumber = 0;
    bool                        m_ControlRequestPending = false;
    bool                        m_ControlRequestResult = false;

    std::vector<uint8_t>                    m_TransferBuffer;
    std::vector<Ptr<USBHostMSCBlockDevice>> m_BlockDevices;
};

class USBHostMSCBlockDevice : public KInode, public KFilesystemFileOps
{
public:
    USBHostMSCBlockDevice(Ptr<USBHostMSCInterface> interface, uint8_t logicalUnitNumber, uint32_t blockSize, off64_t start, off64_t size, int partitionType);
    virtual ~USBHostMSCBlockDevice();

    virtual Ptr<KFileNode> OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags) override;
    virtual size_t Read(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position) override;
    virtual size_t Write(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position) override;
    virtual void DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual void ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuffer) override;
    virtual void Sync(Ptr<KFileNode> file) override;

    int GetNodeHandle() const;
    void SetNodeHandle(int nodeHandle);

private:
    size_t PrepareTransfer(const iovec_t* segments, size_t segmentCount, off64_t position, uint64_t* startBlock) const;

    Ptr<USBHostMSCInterface> m_Interface;
    uint8_t                  m_LogicalUnitNumber = 0;
    uint32_t                 m_BlockSize = 0;
    off64_t                  m_Start = 0;
    off64_t                  m_Size = 0;
    int                      m_PartitionType = 0;
    int                      m_NodeHandle = -1;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostMSCInterface::USBHostMSCInterface(USBHost* host, USBHostClassMSC* classDriver)
    : m_Host(host)
    , m_ClassDriver(classDriver)
    , m_CommandMutex("usbh_msc_command", PEMutexRecursionMode_RaiseError)
    , m_TransactionCondition("usbh_msc_transaction")
    , m_TransferBuffer(USB_MSC_MAX_TRANSFER_SIZE)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostMSCInterface::~USBHostMSCInterface()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USB_DescriptorHeader* USBHostMSCInterface::Open_pl(uint8_t deviceAddress, const USB_DescInterface* interfaceDesc, const void* endDesc)
{
    kassert(m_Host->GetMutex().IsLocked());

    // The current host configuration flow supports only the default alternate setting.
    if (interfaceDesc->bInterfaceClass != USB_ClassCode::MSC
        || interfaceDesc->bInterfaceSubClass != std::to_underlying(USB_MSC_SubclassCode::SCSI_TRANSPARENT_COMMAND_SET)
        || interfaceDesc->bInterfaceProtocol != std::to_underlying(USB_MSC_ProtocolCode::BULK_ONLY_TRANSPORT)
        || interfaceDesc->bAlternateSetting != 0)
    {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    USBDeviceNode* device = m_Host->GetDevice(deviceAddress);
    if (device == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }

    m_DeviceAddress = deviceAddress;
    m_InterfaceNumber = interfaceDesc->bInterfaceNumber;

    const USB_DescriptorHeader* descriptor = interfaceDesc->GetNext();
    while (descriptor < endDesc)
    {
        if (!descriptor->ValidateLength(endDesc)) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        if (descriptor->bDescriptorType == USB_DescriptorType::INTERFACE
            || descriptor->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION)
        {
            break;
        }
        if (descriptor->bDescriptorType == USB_DescriptorType::ENDPOINT)
        {
            if (descriptor->bLength < sizeof(USB_DescEndpoint)) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }

            const USB_DescEndpoint* endpointDesc = static_cast<const USB_DescEndpoint*>(descriptor);
            if (!endpointDesc->Validate(device->m_Speed)) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }
            if (endpointDesc->GetTransferType() == USB_TransferType::BULK)
            {
                const bool isInputEndpoint = (endpointDesc->bEndpointAddress & USB_ADDRESS_DIR_IN) != 0;
                if (isInputEndpoint && m_BulkEndpointIn == USB_INVALID_ENDPOINT)
                {
                    m_BulkEndpointIn = endpointDesc->bEndpointAddress;
                    m_BulkEndpointInSize = endpointDesc->GetMaxPacketSize();
                }
                else if (!isInputEndpoint && m_BulkEndpointOut == USB_INVALID_ENDPOINT)
                {
                    m_BulkEndpointOut = endpointDesc->bEndpointAddress;
                    m_BulkEndpointOutSize = endpointDesc->GetMaxPacketSize();
                }
            }
        }
        descriptor = descriptor->GetNext();
    }

    if (m_BulkEndpointIn == USB_INVALID_ENDPOINT || m_BulkEndpointOut == USB_INVALID_ENDPOINT) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    try
    {
        m_BulkPipeIn = m_Host->AllocPipe(m_BulkEndpointIn);
        m_BulkPipeOut = m_Host->AllocPipe(m_BulkEndpointOut);
    }
    catch (...)
    {
        if (m_BulkPipeIn != USB_INVALID_PIPE) {
            m_Host->FreePipe(m_BulkPipeIn);
        }
        if (m_BulkPipeOut != USB_INVALID_PIPE) {
            m_Host->FreePipe(m_BulkPipeOut);
        }
        m_BulkPipeIn = USB_INVALID_PIPE;
        m_BulkPipeOut = USB_INVALID_PIPE;
        throw;
    }
    if (m_BulkPipeIn == USB_INVALID_PIPE || m_BulkPipeOut == USB_INVALID_PIPE)
    {
        if (m_BulkPipeIn != USB_INVALID_PIPE) {
            m_Host->FreePipe(m_BulkPipeIn);
        }
        if (m_BulkPipeOut != USB_INVALID_PIPE) {
            m_Host->FreePipe(m_BulkPipeOut);
        }
        m_BulkPipeIn = USB_INVALID_PIPE;
        m_BulkPipeOut = USB_INVALID_PIPE;
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    bool inputPipeOpened = false;
    bool outputPipeOpened = false;
    if (m_Host->OpenPipe(m_BulkPipeIn, m_BulkEndpointIn, deviceAddress, device->m_Speed, USB_TransferType::BULK, m_BulkEndpointInSize)) {
        inputPipeOpened = true;
    }
    if (m_Host->OpenPipe(m_BulkPipeOut, m_BulkEndpointOut, deviceAddress, device->m_Speed, USB_TransferType::BULK, m_BulkEndpointOutSize)) {
        outputPipeOpened = true;
    }
    if (!inputPipeOpened || !outputPipeOpened)
    {
        if (inputPipeOpened) {
            m_Host->ClosePipe(m_BulkPipeIn);
        }
        if (outputPipeOpened) {
            m_Host->ClosePipe(m_BulkPipeOut);
        }
        m_Host->FreePipe(m_BulkPipeIn);
        m_Host->FreePipe(m_BulkPipeOut);
        m_BulkPipeIn = USB_INVALID_PIPE;
        m_BulkPipeOut = USB_INVALID_PIPE;
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    m_Host->SetDataToggle(m_BulkPipeIn, false);
    m_Host->SetDataToggle(m_BulkPipeOut, false);
    m_IsConnected = true;
    return descriptor;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<int> USBHostMSCInterface::Close_pl()
{
    kassert(m_Host->GetMutex().IsLocked());

    m_IsConnected = false;
    m_IsStarted = false;
    m_ControlRequestPending = false;

    if (m_TransactionActive)
    {
        m_TransactionResult = PErrorCode::NODEV;
        m_TransactionActive = false;
        m_TransactionStage = USBMSCTransactionStage::Idle;
    }
    m_TransactionCondition.WakeupAll();

    if (m_BulkPipeIn != USB_INVALID_PIPE)
    {
        m_Host->CancelPipe(m_BulkPipeIn);
        m_Host->ClosePipe(m_BulkPipeIn);
        m_Host->FreePipe(m_BulkPipeIn);
        m_BulkPipeIn = USB_INVALID_PIPE;
    }
    if (m_BulkPipeOut != USB_INVALID_PIPE)
    {
        m_Host->CancelPipe(m_BulkPipeOut);
        m_Host->ClosePipe(m_BulkPipeOut);
        m_Host->FreePipe(m_BulkPipeOut);
        m_BulkPipeOut = USB_INVALID_PIPE;
    }

    std::vector<int> nodeHandles;
    nodeHandles.reserve(m_BlockDevices.size());
    for (const Ptr<USBHostMSCBlockDevice>& blockDevice : m_BlockDevices) {
        if (blockDevice->GetNodeHandle() != -1) {
            nodeHandles.push_back(blockDevice->GetNodeHandle());
        }
    }
    m_BlockDevices.clear();
    return nodeHandles;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::Startup_pl()
{
    kassert(m_Host->GetMutex().IsLocked());
    if (!m_IsStarted)
    {
        m_IsStarted = true;
        m_ClassDriver->QueueInitialization(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t USBHostMSCInterface::GetDeviceAddress() const
{
    return m_DeviceAddress;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostMSCInterface::IsActive_pl() const
{
    kassert(m_Host->GetMutex().IsLocked());
    return m_IsConnected && m_IsStarted;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostMSCInterface::IsConnected()
{
    CRITICAL_SCOPE(m_Host->GetMutex());
    return m_IsConnected;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::Initialize()
{
    std::vector<USBMSCLogicalUnitCapacity> capacities;
    {
        KScopedLock commandLock(m_CommandMutex);

        uint8_t maximumLogicalUnitNumber = 0;
        if (!GetMaximumLogicalUnitNumber_pl(&maximumLogicalUnitNumber)) {
            return;
        }

        capacities.resize(size_t(maximumLogicalUnitNumber) + 1);
        for (size_t logicalUnitIndex = 0; logicalUnitIndex < capacities.size(); ++logicalUnitIndex) {
            if (!ProbeLogicalUnit_pl(uint8_t(logicalUnitIndex), &capacities[logicalUnitIndex])) {
                capacities[logicalUnitIndex] = USBMSCLogicalUnitCapacity();
            }
        }
    }

    std::vector<Ptr<USBHostMSCBlockDevice>> devices = CreateDeviceNodes(capacities);
    std::vector<int> rejectedNodeHandles;
    bool devicesAttached = false;
    {
        CRITICAL_SCOPE(m_Host->GetMutex());
        if (m_IsConnected)
        {
            AttachDeviceNodes_pl(std::move(devices));
            devicesAttached = true;
        }
    }

    if (!devicesAttached) {
        for (const Ptr<USBHostMSCBlockDevice>& device : devices) {
            if (device->GetNodeHandle() != -1) {
                rejectedNodeHandles.push_back(device->GetNodeHandle());
            }
        }
    }
    if (!rejectedNodeHandles.empty()) {
        m_ClassDriver->QueueNodeRemovals(std::move(rejectedNodeHandles));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBHostMSCInterface::Transfer(bool write, uint8_t logicalUnitNumber, uint32_t blockSize, uint64_t startBlock, const iovec_t* segments, size_t segmentCount, size_t length)
{
    KScopedLock commandLock(m_CommandMutex);
    USBMSCIOVectorCursor cursor(segments, segmentCount, length);
    uint64_t currentBlock = startBlock;

    const size_t endpointPacketSize = write ? m_BulkEndpointOutSize : m_BulkEndpointInSize;
    size_t maximumTransferLength = std::min(USB_MSC_MAX_TRANSFER_SIZE, endpointPacketSize * USB_MSC_MAX_TRANSFER_PACKETS);
    maximumTransferLength -= maximumTransferLength % blockSize;
    if (maximumTransferLength == 0) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    while (cursor.GetRemainingLength() != 0)
    {
        size_t transferLength = std::min(cursor.GetRemainingLength(), maximumTransferLength);
        transferLength -= transferLength % blockSize;
        const size_t transferBlockCount = transferLength / blockSize;
        if (transferBlockCount == 0 || transferBlockCount > std::numeric_limits<uint16_t>::max()) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        if (currentBlock > std::numeric_limits<uint32_t>::max()) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }

        std::array<uint8_t, 10> commandBlock = {};
        commandBlock[0] = std::to_underlying(write ? USB_MSC_SCSI_OperationCode::WRITE_10 : USB_MSC_SCSI_OperationCode::READ_10);
        USBMSCStoreBigEndian32(commandBlock.data() + 2, uint32_t(currentBlock));
        USBMSCStoreBigEndian16(commandBlock.data() + 7, uint16_t(transferBlockCount));

        USBMSCIOVectorCursor transferCursor(cursor, transferLength);
        const bool useVectorTransfer = transferCursor.IsValid() && transferCursor.HasPacketAlignedSpans(endpointPacketSize);
        if (!useVectorTransfer && write)
        {
            USBMSCIOVectorCursor copyCursor = transferCursor;
            copyCursor.CopyTo(m_TransferBuffer.data(), transferLength);
        }

        size_t transferredLength = 0;
        USBMSCSenseResult senseResult;
        PErrorCode result;
        if (useVectorTransfer)
        {
            result = ExecuteVectorCommand_pl(
                logicalUnitNumber,
                commandBlock.data(),
                commandBlock.size(),
                write ? USB_MSC_DataDirection::DATA_OUT : USB_MSC_DataDirection::DATA_IN,
                transferCursor,
                transferLength,
                &transferredLength,
                &senseResult
            );
        }
        else
        {
            result = ExecuteCommand_pl(
                logicalUnitNumber,
                commandBlock.data(),
                commandBlock.size(),
                write ? USB_MSC_DataDirection::DATA_OUT : USB_MSC_DataDirection::DATA_IN,
                m_TransferBuffer.data(),
                transferLength,
                &transferredLength,
                &senseResult
            );
        }
        PERROR_ERRORCODE_THROW_ON_FAIL(result);
        if (transferredLength != transferLength)
        {
            kernel_log<PLogSeverity::ERROR>(
                LogCategoryUSBHost,
                "MSC {} command completed with a short transfer: device={}, LUN={}, block={}, transferred={}/{}.",
                write ? "write" : "read",
                m_DeviceAddress,
                logicalUnitNumber,
                currentBlock,
                transferredLength,
                transferLength
            );
            PERROR_THROW_CODE(PErrorCode::IO);
        }

        if (!useVectorTransfer && !write)
        {
            USBMSCIOVectorCursor copyCursor = transferCursor;
            copyCursor.CopyFrom(m_TransferBuffer.data(), transferLength);
        }
        cursor.Advance(transferLength);
        currentBlock += transferBlockCount;
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::SynchronizeCache(uint8_t logicalUnitNumber)
{
    KScopedLock commandLock(m_CommandMutex);

    std::array<uint8_t, 10> commandBlock = {};
    commandBlock[0] = std::to_underlying(USB_MSC_SCSI_OperationCode::SYNCHRONIZE_CACHE_10);

    size_t transferredLength = 0;
    USBMSCSenseResult senseResult;
    const PErrorCode result = ExecuteCommand_pl(
        logicalUnitNumber,
        commandBlock.data(),
        commandBlock.size(),
        USB_MSC_DataDirection::DATA_OUT,
        nullptr,
        0,
        &transferredLength,
        &senseResult
    );
    if (result == PErrorCode::IO && senseResult.SenseKey == USB_MSC_SCSI_SenseKey::ILLEGAL_REQUEST) {
        return;
    }
    PERROR_ERRORCODE_THROW_ON_FAIL(result);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostMSCInterface::GetMaximumLogicalUnitNumber_pl(uint8_t* maximumLogicalUnitNumber)
{
    kassert(m_CommandMutex.IsLocked());

    CRITICAL_SCOPE(m_Host->GetMutex());
    if (!m_IsConnected) {
        return false;
    }

    m_MaximumLogicalUnitNumber = 0;
    m_ControlRequestPending = true;
    m_ControlRequestResult = false;

    USB_ControlRequest request(
        USB_RequestRecipient::INTERFACE,
        USB_RequestType::CLASS,
        USB_RequestDirection::DEVICE_TO_HOST,
        std::to_underlying(USB_MSC_Request::GET_MAX_LUN),
        0,
        m_InterfaceNumber,
        1
    );

    Ptr<USBHostMSCInterface> self = ptr_tmp_cast(this);
    const bool requestStarted = m_Host->GetControlHandler().SendControlRequest(
        m_DeviceAddress,
        request,
        &m_MaximumLogicalUnitNumber,
        [self](bool result, uint8_t deviceAddress) { self->HandleGetMaximumLogicalUnitNumber_pl(result, deviceAddress); }
    );
    if (!requestStarted)
    {
        if (m_ControlRequestPending) {
            m_ControlRequestPending = false;
        }
        *maximumLogicalUnitNumber = 0;
        return true;
    }

    while (m_ControlRequestPending && m_IsConnected)
    {
        const PErrorCode waitResult = m_TransactionCondition.Wait(m_Host->GetMutex());
        if (waitResult != PErrorCode::Success && waitResult != PErrorCode::INTR) {
            PERROR_ERRORCODE_THROW_ON_FAIL(waitResult);
        }
    }
    if (!m_IsConnected) {
        return false;
    }

    if (!m_ControlRequestResult || m_MaximumLogicalUnitNumber > 15) {
        *maximumLogicalUnitNumber = 0;
    } else {
        *maximumLogicalUnitNumber = m_MaximumLogicalUnitNumber;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostMSCInterface::ProbeLogicalUnit_pl(uint8_t logicalUnitNumber, USBMSCLogicalUnitCapacity* capacity)
{
    kassert(m_CommandMutex.IsLocked());

    std::array<uint8_t, 6> inquiryCommand = {};
    alignas(4) std::array<uint8_t, USB_MSC_INQUIRY_RESPONSE_SIZE> inquiryResponse = {};
    inquiryCommand[0] = std::to_underlying(USB_MSC_SCSI_OperationCode::INQUIRY);
    inquiryCommand[4] = uint8_t(inquiryResponse.size());

    size_t transferredLength = 0;
    USBMSCSenseResult senseResult;
    PErrorCode result = ExecuteCommand_pl(
        logicalUnitNumber,
        inquiryCommand.data(),
        inquiryCommand.size(),
        USB_MSC_DataDirection::DATA_IN,
        inquiryResponse.data(),
        inquiryResponse.size(),
        &transferredLength,
        &senseResult
    );
    if (result != PErrorCode::Success || transferredLength < inquiryResponse.size() || (inquiryResponse[0] & 0x1f) != 0) {
        return false;
    }

    std::array<uint8_t, 6> readyCommand = {};
    readyCommand[0] = std::to_underlying(USB_MSC_SCSI_OperationCode::TEST_UNIT_READY);
    bool isReady = false;
    for (size_t attempt = 0; attempt < 50; ++attempt)
    {
        senseResult = USBMSCSenseResult();
        result = ExecuteCommand_pl(
            logicalUnitNumber,
            readyCommand.data(),
            readyCommand.size(),
            USB_MSC_DataDirection::DATA_OUT,
            nullptr,
            0,
            &transferredLength,
            &senseResult
        );
        if (result == PErrorCode::Success)
        {
            isReady = true;
            break;
        }
        if (senseResult.AdditionalSenseCode == std::to_underlying(USB_MSC_SCSI_AdditionalSenseCode::MEDIUM_NOT_PRESENT)) {
            return false;
        }
        if (senseResult.SenseKey != USB_MSC_SCSI_SenseKey::NOT_READY
            && senseResult.SenseKey != USB_MSC_SCSI_SenseKey::UNIT_ATTENTION)
        {
            return false;
        }
        if (!IsConnected()) {
            return false;
        }
        ksnooze_ms(100);
    }
    if (!isReady) {
        return false;
    }

    std::array<uint8_t, 10> capacity10Command = {};
    alignas(4) std::array<uint8_t, USB_MSC_READ_CAPACITY_10_RESPONSE_SIZE> capacity10Response = {};
    capacity10Command[0] = std::to_underlying(USB_MSC_SCSI_OperationCode::READ_CAPACITY_10);
    result = ExecuteCommand_pl(
        logicalUnitNumber,
        capacity10Command.data(),
        capacity10Command.size(),
        USB_MSC_DataDirection::DATA_IN,
        capacity10Response.data(),
        capacity10Response.size(),
        &transferredLength,
        &senseResult
    );
    if (result != PErrorCode::Success || transferredLength != capacity10Response.size()) {
        return false;
    }

    uint64_t lastLogicalBlockAddress = USBMSCLoadBigEndian32(capacity10Response.data());
    uint32_t blockSize = USBMSCLoadBigEndian32(capacity10Response.data() + 4);
    if (lastLogicalBlockAddress == std::numeric_limits<uint32_t>::max())
    {
        std::array<uint8_t, 16> capacity16Command = {};
        alignas(4) std::array<uint8_t, USB_MSC_READ_CAPACITY_16_RESPONSE_SIZE> capacity16Response = {};
        capacity16Command[0] = std::to_underlying(USB_MSC_SCSI_OperationCode::SERVICE_ACTION_IN_16);
        capacity16Command[1] = std::to_underlying(USB_MSC_SCSI_ServiceAction::READ_CAPACITY_16);
        USBMSCStoreBigEndian32(capacity16Command.data() + 10, uint32_t(capacity16Response.size()));

        result = ExecuteCommand_pl(
            logicalUnitNumber,
            capacity16Command.data(),
            capacity16Command.size(),
            USB_MSC_DataDirection::DATA_IN,
            capacity16Response.data(),
            capacity16Response.size(),
            &transferredLength,
            &senseResult
        );
        if (result != PErrorCode::Success || transferredLength != capacity16Response.size()) {
            return false;
        }
        lastLogicalBlockAddress = USBMSCLoadBigEndian64(capacity16Response.data());
        blockSize = USBMSCLoadBigEndian32(capacity16Response.data() + 8);
    }

    const size_t maximumEndpointTransfer = std::min(
        USB_MSC_MAX_TRANSFER_SIZE,
        std::min(m_BulkEndpointInSize, m_BulkEndpointOutSize) * USB_MSC_MAX_TRANSFER_PACKETS
    );
    if (lastLogicalBlockAddress > std::numeric_limits<uint32_t>::max()
        || blockSize != USB_MSC_SUPPORTED_BLOCK_SIZE
        || blockSize > maximumEndpointTransfer)
    {
        return false;
    }

    const uint64_t sectorCount = lastLogicalBlockAddress + 1;
    if (sectorCount > uint64_t(std::numeric_limits<off64_t>::max()) / blockSize) {
        return false;
    }

    capacity->BlockSize = blockSize;
    capacity->SectorCount = sectorCount;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode USBHostMSCInterface::ExecuteCommand_pl(uint8_t logicalUnitNumber, const uint8_t* commandBlock, size_t commandBlockLength, USB_MSC_DataDirection direction, void* data, size_t dataLength, size_t* transferredLength, USBMSCSenseResult* senseResult)
{
    iovec_t dataSegment = {data, dataLength};
    const USBMSCIOVectorCursor dataCursor(&dataSegment, 1, dataLength);
    return ExecuteVectorCommand_pl(
        logicalUnitNumber,
        commandBlock,
        commandBlockLength,
        direction,
        dataCursor,
        dataLength,
        transferredLength,
        senseResult
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode USBHostMSCInterface::ExecuteVectorCommand_pl(uint8_t logicalUnitNumber, const uint8_t* commandBlock, size_t commandBlockLength, USB_MSC_DataDirection direction, const USBMSCIOVectorCursor& dataCursor, size_t dataLength, size_t* transferredLength, USBMSCSenseResult* senseResult)
{
    kassert(m_CommandMutex.IsLocked());
    *senseResult = USBMSCSenseResult();

    size_t transportRetryCount = 0;
    size_t unitAttentionRetryCount = 0;
    for (;;)
    {
        USB_MSC_CommandStatus commandStatus = USB_MSC_CommandStatus::COMMAND_PASSED;
        const PErrorCode result = RunTransaction_pl(
            logicalUnitNumber,
            commandBlock,
            commandBlockLength,
            direction,
            dataCursor,
            dataLength,
            transferredLength,
            &commandStatus
        );
        if (result != PErrorCode::Success)
        {
            if (m_TransportRecoveryCompleted && transportRetryCount == 0)
            {
                ++transportRetryCount;
                kernel_log<PLogSeverity::WARNING>(
                    LogCategoryUSBHost,
                    "MSC transport reset completed; retrying device={} LUN={} opcode=0x{:02x} once.",
                    m_DeviceAddress,
                    logicalUnitNumber,
                    commandBlock[0]
                );
                continue;
            }
            return result;
        }
        if (commandStatus == USB_MSC_CommandStatus::COMMAND_PASSED) {
            return PErrorCode::Success;
        }

        const PErrorCode senseRequestResult = RequestSense_pl(logicalUnitNumber, senseResult);
        if (senseRequestResult != PErrorCode::Success)
        {
            kernel_log<PLogSeverity::ERROR>(
                LogCategoryUSBHost,
                "MSC REQUEST SENSE failed after command: device={}, LUN={}, opcode=0x{:02x}, result={}.",
                m_DeviceAddress,
                logicalUnitNumber,
                commandBlock[0],
                p_strerror(senseRequestResult)
            );
            return senseRequestResult;
        }
        if (senseResult->SenseKey == USB_MSC_SCSI_SenseKey::UNIT_ATTENTION && unitAttentionRetryCount == 0)
        {
            ++unitAttentionRetryCount;
            continue;
        }
        kernel_log<PLogSeverity::ERROR>(
            LogCategoryUSBHost,
            "MSC command failed: device={}, LUN={}, opcode=0x{:02x}, status={}, sense=0x{:02x}/0x{:02x}/0x{:02x}.",
            m_DeviceAddress,
            logicalUnitNumber,
            commandBlock[0],
            std::to_underlying(commandStatus),
            std::to_underlying(senseResult->SenseKey),
            senseResult->AdditionalSenseCode,
            senseResult->AdditionalSenseQualifier
        );
        if (senseResult->SenseKey == USB_MSC_SCSI_SenseKey::NOT_READY) {
            return PErrorCode::AGAIN;
        }
        if (senseResult->SenseKey == USB_MSC_SCSI_SenseKey::DATA_PROTECT) {
            return PErrorCode::ROFS;
        }
        return PErrorCode::IO;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode USBHostMSCInterface::RequestSense_pl(uint8_t logicalUnitNumber, USBMSCSenseResult* senseResult)
{
    kassert(m_CommandMutex.IsLocked());

    std::array<uint8_t, 6> commandBlock = {};
    alignas(4) std::array<uint8_t, USB_MSC_REQUEST_SENSE_RESPONSE_SIZE> response = {};
    commandBlock[0] = std::to_underlying(USB_MSC_SCSI_OperationCode::REQUEST_SENSE);
    commandBlock[4] = uint8_t(response.size());

    size_t transferredLength = 0;
    USB_MSC_CommandStatus commandStatus = USB_MSC_CommandStatus::COMMAND_PASSED;
    iovec_t dataSegment = {response.data(), response.size()};
    const USBMSCIOVectorCursor dataCursor(&dataSegment, 1, response.size());
    const PErrorCode result = RunTransaction_pl(
        logicalUnitNumber,
        commandBlock.data(),
        commandBlock.size(),
        USB_MSC_DataDirection::DATA_IN,
        dataCursor,
        response.size(),
        &transferredLength,
        &commandStatus
    );
    if (result != PErrorCode::Success) {
        return result;
    }
    if (commandStatus != USB_MSC_CommandStatus::COMMAND_PASSED || transferredLength < 14) {
        return PErrorCode::IO;
    }

    const uint8_t responseCode = response[0] & 0x7f;
    if ((responseCode != std::to_underlying(USB_MSC_SCSI_SenseResponseCode::CURRENT_ERRORS)
        && responseCode != std::to_underlying(USB_MSC_SCSI_SenseResponseCode::DEFERRED_ERRORS))
        || response[7] < 6)
    {
        return PErrorCode::IO;
    }

    senseResult->SenseKey = USB_MSC_SCSI_SenseKey(response[2] & 0x0f);
    senseResult->AdditionalSenseCode = response[12];
    senseResult->AdditionalSenseQualifier = response[13];
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode USBHostMSCInterface::RunTransaction_pl(uint8_t logicalUnitNumber, const uint8_t* commandBlock, size_t commandBlockLength, USB_MSC_DataDirection direction, const USBMSCIOVectorCursor& dataCursor, size_t dataLength, size_t* transferredLength, USB_MSC_CommandStatus* commandStatus)
{
    kassert(m_CommandMutex.IsLocked());

    const USBMSCIOVectorCursor transactionDataCursor(dataCursor, dataLength);
    if (logicalUnitNumber > USB_MSC_CommandBlockWrapper::LOGICAL_UNIT_NUMBER_MASK
        || commandBlock == nullptr
        || commandBlockLength == 0
        || commandBlockLength > USB_MSC_CommandBlockWrapper::MAX_COMMAND_BLOCK_LENGTH
        || dataLength > std::numeric_limits<uint32_t>::max()
        || !transactionDataCursor.IsValid())
    {
        return PErrorCode::INVAL;
    }

    CRITICAL_SCOPE(m_Host->GetMutex());
    if (!m_IsConnected) {
        return PErrorCode::NODEV;
    }

    m_CommandBlockWrapper = USB_MSC_CommandBlockWrapper();
    m_CommandBlockWrapper.Signature = PHostToLittleEndian(USB_MSC_CommandBlockWrapper::SIGNATURE);
    m_CommandBlockWrapper.Tag = PHostToLittleEndian(m_NextTag++);
    m_CommandBlockWrapper.DataTransferLength = PHostToLittleEndian(uint32_t(dataLength));
    m_CommandBlockWrapper.Flags = direction;
    m_CommandBlockWrapper.LogicalUnitNumber = logicalUnitNumber;
    m_CommandBlockWrapper.CommandBlockLength = uint8_t(commandBlockLength);
    memcpy(m_CommandBlockWrapper.CommandBlock, commandBlock, commandBlockLength);

    m_TransactionDataCursor = transactionDataCursor;
    m_TransactionDataLength = dataLength;
    m_TransactionTransferredLength = 0;
    m_TransactionRequestLength = 0;
    m_TransactionResult = PErrorCode::IO;
    m_TransportRecoveryCompleted = false;
    m_TransactionCommandStatus = USB_MSC_CommandStatus::COMMAND_PASSED;
    m_TransactionStage = USBMSCTransactionStage::CommandBlock;
    m_StalledDataPipe = USB_INVALID_PIPE;
    m_LastTransportPipe = USB_INVALID_PIPE;
    m_LastTransportURBState = USB_URBState::Idle;
    m_LastTransportLength = 0;
    m_TransactionActive = true;
    m_StatusRetryUsed = false;

    if (!StartCommandBlock_pl()) {
        StartResetRecovery_pl(PErrorCode::IO);
    }

    TimeValNanos deadline = (m_TransactionStage == USBMSCTransactionStage::Reset)
        ? TimeValNanos::infinit
        : kget_monotonic_time() + USB_MSC_COMMAND_TIMEOUT;
    while (m_TransactionActive)
    {
        const PErrorCode waitResult = m_TransactionCondition.WaitDeadline(m_Host->GetMutex(), deadline);
        if (waitResult == PErrorCode::TIMEDOUT)
        {
            if (m_TransactionStage == USBMSCTransactionStage::Reset
                || m_TransactionStage == USBMSCTransactionStage::ResetClearIn
                || m_TransactionStage == USBMSCTransactionStage::ResetClearOut)
            {
                deadline = TimeValNanos::infinit;
            }
            else
            {
                StartResetRecovery_pl(PErrorCode::TIMEDOUT);
                deadline = TimeValNanos::infinit;
            }
        }
        else if (waitResult != PErrorCode::Success && waitResult != PErrorCode::INTR)
        {
            StartResetRecovery_pl(waitResult);
            deadline = TimeValNanos::infinit;
        }
    }

    *transferredLength = m_TransactionTransferredLength;
    *commandStatus = m_TransactionCommandStatus;
    return m_TransactionResult;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostMSCInterface::StartCommandBlock_pl()
{
    kassert(m_Host->GetMutex().IsLocked());
    m_TransactionStage = USBMSCTransactionStage::CommandBlock;
    return SubmitBulkOut_pl(&m_CommandBlockWrapper, sizeof(m_CommandBlockWrapper));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostMSCInterface::StartDataTransfer_pl()
{
    kassert(m_Host->GetMutex().IsLocked());
    if (m_TransactionDataCursor.GetRemainingLength() == 0) {
        return StartStatusTransfer_pl();
    }

    void* transferBuffer = nullptr;
    size_t transferLength = 0;
    if (!m_TransactionDataCursor.GetCurrentSpan(&transferBuffer, &transferLength) || transferLength == 0) {
        return false;
    }
    m_TransactionRequestLength = transferLength;

    if (m_CommandBlockWrapper.Flags == USB_MSC_DataDirection::DATA_IN)
    {
        m_TransactionStage = USBMSCTransactionStage::DataIn;
        return SubmitBulkIn_pl(transferBuffer, transferLength);
    }
    m_TransactionStage = USBMSCTransactionStage::DataOut;
    return SubmitBulkOut_pl(transferBuffer, transferLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostMSCInterface::StartStatusTransfer_pl()
{
    kassert(m_Host->GetMutex().IsLocked());
    m_CommandStatusWrapper = USB_MSC_CommandStatusWrapper();
    m_TransactionStage = USBMSCTransactionStage::Status;
    return SubmitBulkIn_pl(&m_CommandStatusWrapper, sizeof(m_CommandStatusWrapper));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostMSCInterface::SubmitBulkOut_pl(void* buffer, size_t length)
{
    kassert(m_Host->GetMutex().IsLocked());
    Ptr<USBHostMSCInterface> self = ptr_tmp_cast(this);
    const bool result = m_Host->BulkSendData(
        m_BulkPipeOut,
        buffer,
        length,
        true,
        [self](USB_PipeIndex pipeIndex, USB_URBState state, size_t transactionLength)
        {
            self->HandleBulkTransfer_pl(pipeIndex, state, transactionLength);
        }
    );
    if (!result) {
        m_Host->CancelPipe(m_BulkPipeOut);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostMSCInterface::SubmitBulkIn_pl(void* buffer, size_t length)
{
    kassert(m_Host->GetMutex().IsLocked());
    Ptr<USBHostMSCInterface> self = ptr_tmp_cast(this);
    const bool result = m_Host->BulkReceiveData(
        m_BulkPipeIn,
        buffer,
        length,
        [self](USB_PipeIndex pipeIndex, USB_URBState state, size_t transactionLength)
        {
            self->HandleBulkTransfer_pl(pipeIndex, state, transactionLength);
        }
    );
    if (!result) {
        m_Host->CancelPipe(m_BulkPipeIn);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::StartDataHaltClear_pl()
{
    kassert(m_Host->GetMutex().IsLocked());
    const bool inputStalled = m_TransactionStage == USBMSCTransactionStage::DataIn;
    const uint8_t endpoint = inputStalled ? m_BulkEndpointIn : m_BulkEndpointOut;
    m_StalledDataPipe = inputStalled ? m_BulkPipeIn : m_BulkPipeOut;
    m_TransactionStage = USBMSCTransactionStage::DataHaltClear;

    Ptr<USBHostMSCInterface> self = ptr_tmp_cast(this);
    const bool requestStarted = m_Host->GetControlHandler().ReqClearEndpointHalt(
        m_DeviceAddress,
        endpoint,
        [self](bool result, uint8_t deviceAddress) { self->HandleDataHaltClear_pl(result, deviceAddress); }
    );
    if (!requestStarted && m_TransactionActive && m_TransactionStage == USBMSCTransactionStage::DataHaltClear) {
        StartResetRecovery_pl(PErrorCode::IO);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::StartStatusHaltClear_pl()
{
    kassert(m_Host->GetMutex().IsLocked());
    m_TransactionStage = USBMSCTransactionStage::StatusHaltClear;

    Ptr<USBHostMSCInterface> self = ptr_tmp_cast(this);
    const bool requestStarted = m_Host->GetControlHandler().ReqClearEndpointHalt(
        m_DeviceAddress,
        m_BulkEndpointIn,
        [self](bool result, uint8_t deviceAddress) { self->HandleStatusHaltClear_pl(result, deviceAddress); }
    );
    if (!requestStarted && m_TransactionActive && m_TransactionStage == USBMSCTransactionStage::StatusHaltClear) {
        StartResetRecovery_pl(PErrorCode::IO);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::StartResetRecovery_pl(PErrorCode result)
{
    kassert(m_Host->GetMutex().IsLocked());
    if (!m_IsConnected)
    {
        CompleteTransaction_pl(PErrorCode::NODEV, USB_MSC_CommandStatus::PHASE_ERROR);
        return;
    }

    kernel_log<PLogSeverity::ERROR>(
        LogCategoryUSBHost,
        "MSC transport error: device={}, LUN={}, opcode=0x{:02x}, stage={}, result={}, transferred={}/{}, last-urb={} pipe={} bytes={}; starting reset recovery.",
        m_DeviceAddress,
        m_CommandBlockWrapper.LogicalUnitNumber,
        m_CommandBlockWrapper.CommandBlock[0],
        USBMSCGetTransactionStageName(m_TransactionStage),
        p_strerror(result),
        m_TransactionTransferredLength,
        m_TransactionDataLength,
        USBMSCGetURBStateName(m_LastTransportURBState),
        m_LastTransportPipe,
        m_LastTransportLength
    );

    m_RecoveryResult = result;
    const bool inputCanceled = m_Host->CancelPipe(m_BulkPipeIn);
    const bool outputCanceled = m_Host->CancelPipe(m_BulkPipeOut);
    if (!inputCanceled || !outputCanceled)
    {
        FailResetRecovery_pl(false);
        return;
    }
    m_TransactionStage = USBMSCTransactionStage::Reset;

    USB_ControlRequest request(
        USB_RequestRecipient::INTERFACE,
        USB_RequestType::CLASS,
        USB_RequestDirection::HOST_TO_DEVICE,
        std::to_underlying(USB_MSC_Request::BULK_ONLY_RESET),
        0,
        m_InterfaceNumber,
        0
    );
    Ptr<USBHostMSCInterface> self = ptr_tmp_cast(this);
    const bool requestStarted = m_Host->GetControlHandler().SendControlRequest(
        m_DeviceAddress,
        request,
        nullptr,
        [self](bool requestResult, uint8_t deviceAddress) { self->HandleReset_pl(requestResult, deviceAddress); }
    );
    if (!requestStarted && m_TransactionActive && m_TransactionStage == USBMSCTransactionStage::Reset) {
        FailResetRecovery_pl();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::FailResetRecovery_pl(bool cancelPipes)
{
    kassert(m_Host->GetMutex().IsLocked());
    m_IsConnected = false;
    m_IsStarted = false;
    if (cancelPipes)
    {
        m_Host->CancelPipe(m_BulkPipeIn);
        m_Host->CancelPipe(m_BulkPipeOut);
    }
    CompleteTransaction_pl(m_RecoveryResult, USB_MSC_CommandStatus::PHASE_ERROR);
    m_Host->ReEnumerate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::CompleteTransaction_pl(PErrorCode result, USB_MSC_CommandStatus commandStatus)
{
    kassert(m_Host->GetMutex().IsLocked());
    m_TransactionResult = result;
    m_TransactionCommandStatus = commandStatus;
    m_TransactionActive = false;
    m_TransactionStage = USBMSCTransactionStage::Idle;
    m_TransactionCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::HandleBulkTransfer_pl(USB_PipeIndex pipeIndex, USB_URBState state, size_t transactionLength)
{
    kassert(m_Host->GetMutex().IsLocked());
    if (!m_TransactionActive || !m_IsConnected) {
        return;
    }

    m_LastTransportPipe = pipeIndex;
    m_LastTransportURBState = state;
    m_LastTransportLength = transactionLength;

    if (state == USB_URBState::NotReady)
    {
        if (m_TransactionStage == USBMSCTransactionStage::CommandBlock) {
            if (!SubmitBulkOut_pl(&m_CommandBlockWrapper, sizeof(m_CommandBlockWrapper))) {
                StartResetRecovery_pl(PErrorCode::IO);
            }
        } else if (m_TransactionStage == USBMSCTransactionStage::DataIn || m_TransactionStage == USBMSCTransactionStage::DataOut) {
            if (transactionLength != 0 || !StartDataTransfer_pl()) {
                StartResetRecovery_pl(PErrorCode::IO);
            }
        }
        return;
    }

    switch (m_TransactionStage)
    {
        case USBMSCTransactionStage::CommandBlock:
            if (pipeIndex != m_BulkPipeOut || state != USB_URBState::Done || transactionLength != sizeof(m_CommandBlockWrapper)) {
                StartResetRecovery_pl(PErrorCode::IO);
            } else if (!StartDataTransfer_pl()) {
                StartResetRecovery_pl(PErrorCode::IO);
            }
            break;

        case USBMSCTransactionStage::DataIn:
            if (pipeIndex != m_BulkPipeIn)
            {
                StartResetRecovery_pl(PErrorCode::IO);
            }
            else if (state == USB_URBState::Stall)
            {
                if (transactionLength > m_TransactionRequestLength
                    || transactionLength > m_TransactionDataCursor.GetRemainingLength())
                {
                    StartResetRecovery_pl(PErrorCode::IO);
                }
                else
                {
                    m_TransactionTransferredLength += transactionLength;
                    m_TransactionDataCursor.Advance(transactionLength);
                    StartDataHaltClear_pl();
                }
            }
            else if (state != USB_URBState::Done
                || transactionLength > m_TransactionRequestLength
                || transactionLength > m_TransactionDataCursor.GetRemainingLength())
            {
                StartResetRecovery_pl(PErrorCode::IO);
            }
            else
            {
                m_TransactionTransferredLength += transactionLength;
                m_TransactionDataCursor.Advance(transactionLength);
                if (transactionLength < m_TransactionRequestLength)
                {
                    if (!StartStatusTransfer_pl()) {
                        StartResetRecovery_pl(PErrorCode::IO);
                    }
                }
                else if (!StartDataTransfer_pl())
                {
                    StartResetRecovery_pl(PErrorCode::IO);
                }
            }
            break;

        case USBMSCTransactionStage::DataOut:
            if (pipeIndex != m_BulkPipeOut)
            {
                StartResetRecovery_pl(PErrorCode::IO);
            }
            else if (state == USB_URBState::Stall)
            {
                if (transactionLength > m_TransactionRequestLength
                    || transactionLength > m_TransactionDataCursor.GetRemainingLength())
                {
                    StartResetRecovery_pl(PErrorCode::IO);
                }
                else
                {
                    m_TransactionTransferredLength += transactionLength;
                    m_TransactionDataCursor.Advance(transactionLength);
                    StartDataHaltClear_pl();
                }
            }
            else if (state != USB_URBState::Done
                || transactionLength != m_TransactionRequestLength
                || transactionLength > m_TransactionDataCursor.GetRemainingLength())
            {
                StartResetRecovery_pl(PErrorCode::IO);
            }
            else
            {
                m_TransactionTransferredLength += transactionLength;
                m_TransactionDataCursor.Advance(transactionLength);
                if (!StartDataTransfer_pl()) {
                    StartResetRecovery_pl(PErrorCode::IO);
                }
            }
            break;

        case USBMSCTransactionStage::Status:
            if (pipeIndex != m_BulkPipeIn)
            {
                StartResetRecovery_pl(PErrorCode::IO);
            }
            else if (state == USB_URBState::Stall)
            {
                if (m_StatusRetryUsed)
                {
                    StartResetRecovery_pl(PErrorCode::IO);
                }
                else
                {
                    m_StatusRetryUsed = true;
                    StartStatusHaltClear_pl();
                }
            }
            else if (state != USB_URBState::Done || transactionLength != sizeof(m_CommandStatusWrapper))
            {
                StartResetRecovery_pl(PErrorCode::IO);
            }
            else
            {
                const uint32_t signature = PLittleEndianToHost(m_CommandStatusWrapper.Signature);
                const uint32_t tag = PLittleEndianToHost(m_CommandStatusWrapper.Tag);
                const uint32_t expectedTag = PLittleEndianToHost(m_CommandBlockWrapper.Tag);
                const uint32_t residue = PLittleEndianToHost(m_CommandStatusWrapper.DataResidue);

                if (signature != USB_MSC_CommandStatusWrapper::SIGNATURE
                    || tag != expectedTag
                    || residue > m_TransactionDataLength
                    || m_CommandStatusWrapper.Status > USB_MSC_CommandStatus::PHASE_ERROR)
                {
                    kernel_log<PLogSeverity::ERROR>(
                        LogCategoryUSBHost,
                        "MSC invalid CSW: device={}, signature=0x{:08x}, tag=0x{:08x}/0x{:08x}, residue={}/{}, status={}.",
                        m_DeviceAddress,
                        signature,
                        tag,
                        expectedTag,
                        residue,
                        m_TransactionDataLength,
                        std::to_underlying(m_CommandStatusWrapper.Status)
                    );
                    StartResetRecovery_pl(PErrorCode::IO);
                }
                else if (m_CommandStatusWrapper.Status == USB_MSC_CommandStatus::PHASE_ERROR)
                {
                    StartResetRecovery_pl(PErrorCode::IO);
                }
                else
                {
                    const size_t reportedTransferLength = m_TransactionDataLength - residue;
                    if (m_CommandBlockWrapper.Flags == USB_MSC_DataDirection::DATA_IN
                        && m_TransactionTransferredLength != reportedTransferLength)
                    {
                        StartResetRecovery_pl(PErrorCode::IO);
                    }
                    else
                    {
                        m_TransactionTransferredLength = reportedTransferLength;
                        CompleteTransaction_pl(PErrorCode::Success, m_CommandStatusWrapper.Status);
                    }
                }
            }
            break;

        default:
            StartResetRecovery_pl(PErrorCode::IO);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::HandleDataHaltClear_pl(bool result, uint8_t deviceAddress)
{
    kassert(m_Host->GetMutex().IsLocked());
    if (!m_TransactionActive || deviceAddress != m_DeviceAddress || m_TransactionStage != USBMSCTransactionStage::DataHaltClear) {
        return;
    }
    if (!result)
    {
        StartResetRecovery_pl(PErrorCode::IO);
        return;
    }

    if (m_StalledDataPipe != USB_INVALID_PIPE) {
        m_Host->SetDataToggle(m_StalledDataPipe, false);
    }
    m_StalledDataPipe = USB_INVALID_PIPE;
    if (!StartStatusTransfer_pl()) {
        StartResetRecovery_pl(PErrorCode::IO);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::HandleStatusHaltClear_pl(bool result, uint8_t deviceAddress)
{
    kassert(m_Host->GetMutex().IsLocked());
    if (!m_TransactionActive || deviceAddress != m_DeviceAddress || m_TransactionStage != USBMSCTransactionStage::StatusHaltClear) {
        return;
    }
    if (!result)
    {
        StartResetRecovery_pl(PErrorCode::IO);
        return;
    }

    m_Host->SetDataToggle(m_BulkPipeIn, false);
    if (!StartStatusTransfer_pl()) {
        StartResetRecovery_pl(PErrorCode::IO);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::HandleReset_pl(bool result, uint8_t deviceAddress)
{
    kassert(m_Host->GetMutex().IsLocked());
    if (!m_TransactionActive || deviceAddress != m_DeviceAddress || m_TransactionStage != USBMSCTransactionStage::Reset) {
        return;
    }
    if (!result)
    {
        FailResetRecovery_pl();
        return;
    }

    m_TransactionStage = USBMSCTransactionStage::ResetClearIn;
    Ptr<USBHostMSCInterface> self = ptr_tmp_cast(this);
    const bool requestStarted = m_Host->GetControlHandler().ReqClearEndpointHalt(
        m_DeviceAddress,
        m_BulkEndpointIn,
        [self](bool clearResult, uint8_t callbackAddress) { self->HandleResetClearIn_pl(clearResult, callbackAddress); }
    );
    if (!requestStarted && m_TransactionActive && m_TransactionStage == USBMSCTransactionStage::ResetClearIn) {
        FailResetRecovery_pl();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::HandleResetClearIn_pl(bool result, uint8_t deviceAddress)
{
    kassert(m_Host->GetMutex().IsLocked());
    if (!m_TransactionActive || deviceAddress != m_DeviceAddress || m_TransactionStage != USBMSCTransactionStage::ResetClearIn) {
        return;
    }
    if (!result)
    {
        FailResetRecovery_pl();
        return;
    }

    m_Host->SetDataToggle(m_BulkPipeIn, false);
    m_TransactionStage = USBMSCTransactionStage::ResetClearOut;
    Ptr<USBHostMSCInterface> self = ptr_tmp_cast(this);
    const bool requestStarted = m_Host->GetControlHandler().ReqClearEndpointHalt(
        m_DeviceAddress,
        m_BulkEndpointOut,
        [self](bool clearResult, uint8_t callbackAddress) { self->HandleResetClearOut_pl(clearResult, callbackAddress); }
    );
    if (!requestStarted && m_TransactionActive && m_TransactionStage == USBMSCTransactionStage::ResetClearOut) {
        FailResetRecovery_pl();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::HandleResetClearOut_pl(bool result, uint8_t deviceAddress)
{
    kassert(m_Host->GetMutex().IsLocked());
    if (!m_TransactionActive || deviceAddress != m_DeviceAddress || m_TransactionStage != USBMSCTransactionStage::ResetClearOut) {
        return;
    }
    if (!result)
    {
        FailResetRecovery_pl();
        return;
    }

    m_Host->SetDataToggle(m_BulkPipeOut, false);
    m_TransportRecoveryCompleted = true;
    CompleteTransaction_pl(m_RecoveryResult, USB_MSC_CommandStatus::PHASE_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::HandleGetMaximumLogicalUnitNumber_pl(bool result, uint8_t deviceAddress)
{
    kassert(m_Host->GetMutex().IsLocked());
    if (m_ControlRequestPending && deviceAddress == m_DeviceAddress)
    {
        m_ControlRequestResult = result;
        m_ControlRequestPending = false;
        m_TransactionCondition.WakeupAll();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<Ptr<USBHostMSCBlockDevice>> USBHostMSCInterface::CreateDeviceNodes(const std::vector<USBMSCLogicalUnitCapacity>& capacities)
{
    std::vector<Ptr<USBHostMSCBlockDevice>> devices;

    try
    {
        for (size_t logicalUnitIndex = 0; logicalUnitIndex < capacities.size(); ++logicalUnitIndex)
        {
            const USBMSCLogicalUnitCapacity& capacity = capacities[logicalUnitIndex];
            if (capacity.BlockSize == 0 || capacity.SectorCount == 0) {
                continue;
            }

            const off64_t diskSize = off64_t(capacity.SectorCount * capacity.BlockSize);
            Ptr<USBHostMSCBlockDevice> rawDevice = ptr_new<USBHostMSCBlockDevice>(
                ptr_tmp_cast(this),
                uint8_t(logicalUnitIndex),
                capacity.BlockSize,
                0,
                diskSize,
                0
            );

            std::vector<uint8_t> partitionBuffer(capacity.BlockSize);
            device_geometry geometry = {};
            geometry.sector_count = capacity.SectorCount;
            geometry.bytes_per_sector = capacity.BlockSize;
            geometry.read_only = false;
            geometry.removable = true;

            std::vector<disk_partition_desc> partitions;
            try
            {
                partitions = KVFSManager::DecodeDiskPartitions_trw(
                    partitionBuffer.data(),
                    partitionBuffer.size(),
                    geometry,
                    &USBHostMSCInterface::ReadPartitionData,
                    ptr_raw_pointer_cast(rawDevice)
                );
            }
            catch (...)
            {
                partitions.clear();
            }
            if (!IsConnected()) {
                return devices;
            }

            std::sort(
                partitions.begin(),
                partitions.end(),
                [](const disk_partition_desc& lhs, const disk_partition_desc& rhs) { return lhs.p_start < rhs.p_start; }
            );

            const PString pathBase = PString::format_string(
                "usb/bus0/dev{}/msc{}/lun{}/",
                int(m_DeviceAddress),
                int(m_InterfaceNumber),
                logicalUnitIndex
            );

            devices.push_back(rawDevice);
            rawDevice->SetNodeHandle(kregister_device_root_trw((pathBase + "raw").c_str(), rawDevice));

            size_t partitionIndex = 0;
            for (const disk_partition_desc& partition : partitions)
            {
                if (partition.p_type == 0
                    || partition.p_start < 0
                    || partition.p_size <= 0
                    || (partition.p_start % capacity.BlockSize) != 0
                    || (partition.p_size % capacity.BlockSize) != 0
                    || partition.p_start > diskSize
                    || partition.p_size > diskSize - partition.p_start)
                {
                    continue;
                }

                Ptr<USBHostMSCBlockDevice> partitionDevice = ptr_new<USBHostMSCBlockDevice>(
                    ptr_tmp_cast(this),
                    uint8_t(logicalUnitIndex),
                    capacity.BlockSize,
                    partition.p_start,
                    partition.p_size,
                    partition.p_type
                );
                const PString partitionPath = pathBase + PString::format_string("{}", partitionIndex++);
                devices.push_back(partitionDevice);
                partitionDevice->SetNodeHandle(kregister_device_root_trw(partitionPath.c_str(), partitionDevice));
            }
        }
    }
    catch (...)
    {
        std::vector<int> nodeHandles;
        for (const Ptr<USBHostMSCBlockDevice>& device : devices) {
            if (device->GetNodeHandle() != -1) {
                nodeHandles.push_back(device->GetNodeHandle());
            }
        }
        m_ClassDriver->QueueNodeRemovals(std::move(nodeHandles));
        throw;
    }
    return devices;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::AttachDeviceNodes_pl(std::vector<Ptr<USBHostMSCBlockDevice>>&& devices)
{
    kassert(m_Host->GetMutex().IsLocked());
    m_BlockDevices = std::move(devices);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCInterface::ReadPartitionData(void* userData, off64_t position, void* buffer, size_t size)
{
    USBHostMSCBlockDevice* device = static_cast<USBHostMSCBlockDevice*>(userData);
    iovec_t segment;
    segment.iov_base = buffer;
    segment.iov_len = size;
    if (device->Read(nullptr, &segment, 1, position) != size) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostMSCBlockDevice::USBHostMSCBlockDevice(Ptr<USBHostMSCInterface> interface, uint8_t logicalUnitNumber, uint32_t blockSize, off64_t start, off64_t size, int partitionType)
    : KInode(nullptr, nullptr, this, S_IFBLK | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
    , m_Interface(interface)
    , m_LogicalUnitNumber(logicalUnitNumber)
    , m_BlockSize(blockSize)
    , m_Start(start)
    , m_Size(size)
    , m_PartitionType(partitionType)
{
    m_ATime = m_MTime = m_CTime = kget_real_time();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostMSCBlockDevice::~USBHostMSCBlockDevice()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> USBHostMSCBlockDevice::OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags)
{
    if (!m_Interface->IsConnected()) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }
    return KFilesystemFileOps::OpenFile(volume, inode, openFlags);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBHostMSCBlockDevice::Read(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position)
{
    uint64_t startBlock = 0;
    const size_t length = PrepareTransfer(segments, segmentCount, position, &startBlock);
    if (length == 0) {
        return 0;
    }
    return m_Interface->Transfer(false, m_LogicalUnitNumber, m_BlockSize, startBlock, segments, segmentCount, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBHostMSCBlockDevice::Write(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position)
{
    uint64_t startBlock = 0;
    const size_t length = PrepareTransfer(segments, segmentCount, position, &startBlock);
    if (length == 0) {
        return 0;
    }
    return m_Interface->Transfer(true, m_LogicalUnitNumber, m_BlockSize, startBlock, segments, segmentCount, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCBlockDevice::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    if (!m_Interface->IsConnected()) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }

    switch (request)
    {
        case DEVCTL_GET_DEVICE_GEOMETRY:
        {
            if (outData == nullptr || outDataLength < sizeof(device_geometry)) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            device_geometry* geometry = static_cast<device_geometry*>(outData);
            geometry->sector_count = uint64_t(m_Size) / m_BlockSize;
            geometry->bytes_per_sector = m_BlockSize;
            geometry->read_only = false;
            geometry->removable = true;
            return;
        }

        default:
            PERROR_THROW_CODE(PErrorCode::NOSYS);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCBlockDevice::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuffer)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuffer);
    statBuffer->st_size = m_Size;
    statBuffer->st_blksize = m_BlockSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCBlockDevice::Sync(Ptr<KFileNode> file)
{
    m_Interface->SynchronizeCache(m_LogicalUnitNumber);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBHostMSCBlockDevice::GetNodeHandle() const
{
    return m_NodeHandle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostMSCBlockDevice::SetNodeHandle(int nodeHandle)
{
    m_NodeHandle = nodeHandle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBHostMSCBlockDevice::PrepareTransfer(const iovec_t* segments, size_t segmentCount, off64_t position, uint64_t* startBlock) const
{
    if (position < 0 || startBlock == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (segmentCount == 0) {
        return 0;
    }
    if (segments == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    size_t length = 0;
    for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
    {
        if (segments[segmentIndex].iov_len > std::numeric_limits<size_t>::max() - length) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        length += segments[segmentIndex].iov_len;
    }
    if (length == 0 || position >= m_Size) {
        return 0;
    }

    const uint64_t remainingDeviceLength = uint64_t(m_Size - position);
    if (uint64_t(length) > remainingDeviceLength) {
        length = size_t(remainingDeviceLength);
    }
    if ((position % m_BlockSize) != 0 || (length % m_BlockSize) != 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_Start > std::numeric_limits<off64_t>::max() - position) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }

    *startBlock = uint64_t(m_Start + position) / m_BlockSize;
    if (*startBlock > std::numeric_limits<uint32_t>::max()) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    const uint64_t blockCount = length / m_BlockSize;
    if (blockCount != 0 && blockCount - 1 > std::numeric_limits<uint32_t>::max() - *startBlock) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostClassMSC::USBHostClassMSC()
    : KThread("usbh_msc_worker")
    , m_WorkMutex("usbh_msc_work", PEMutexRecursionMode_RaiseError)
    , m_WorkCondition("usbh_msc_work")
{
    SetDeleteOnExit(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostClassMSC::~USBHostClassMSC()
{
    if (m_WorkerStarted) {
        Shutdown();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_ClassCode USBHostClassMSC::GetClassCode() const
{
    return USB_ClassCode::MSC;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const char* USBHostClassMSC::GetName() const
{
    return "MSC";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostClassMSC::Init(USBHost* host)
{
    if (!USBClassDriverHost::Init(host)) {
        return false;
    }

    m_StopRequested = false;
    try {
        Start_trw(KSpawnThreadFlag::None, PThreadDetachState_Joinable);
    }
    catch (...)
    {
        USBClassDriverHost::Shutdown();
        throw;
    }
    m_WorkerStarted = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassMSC::Shutdown()
{
    if (m_HostHandler != nullptr)
    {
        kassert(!m_HostHandler->GetMutex().IsLocked());
        std::vector<int> nodeHandles;
        {
            CRITICAL_SCOPE(m_HostHandler->GetMutex());
            for (const Ptr<USBHostMSCInterface>& interface : m_Interfaces)
            {
                m_HostHandler->GetControlHandler().CancelDeviceRequests(interface->GetDeviceAddress());
                std::vector<int> interfaceNodeHandles = interface->Close_pl();
                nodeHandles.insert(nodeHandles.end(), interfaceNodeHandles.begin(), interfaceNodeHandles.end());
            }
            m_Interfaces.clear();
            m_IsActive = false;
        }
        QueueNodeRemovals(std::move(nodeHandles));
    }

    if (m_WorkerStarted)
    {
        {
            CRITICAL_SCOPE(m_WorkMutex);
            m_StopRequested = true;
            m_WorkCondition.WakeupAll();
        }
        Join_trw();
        m_WorkerStarted = false;
    }
    USBClassDriverHost::Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USB_DescriptorHeader* USBHostClassMSC::Open(uint8_t deviceAddress, const USB_DescInterface* interfaceDesc, const USB_DescInterfaceAssociation* interfaceAssociationDesc, const void* endDesc)
{
    kassert(m_HostHandler->GetMutex().IsLocked());
    Ptr<USBHostMSCInterface> interface = ptr_new<USBHostMSCInterface>(m_HostHandler, this);
    const USB_DescriptorHeader* nextDescriptor = interface->Open_pl(deviceAddress, interfaceDesc, endDesc);
    try {
        m_Interfaces.push_back(interface);
    }
    catch (...)
    {
        interface->Close_pl();
        throw;
    }
    return nextDescriptor;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassMSC::Close()
{
    kassert(m_HostHandler->GetMutex().IsLocked());
    std::vector<int> nodeHandles;
    for (const Ptr<USBHostMSCInterface>& interface : m_Interfaces)
    {
        std::vector<int> interfaceNodeHandles = interface->Close_pl();
        nodeHandles.insert(nodeHandles.end(), interfaceNodeHandles.begin(), interfaceNodeHandles.end());
    }
    m_Interfaces.clear();
    m_IsActive = false;
    QueueNodeRemovals(std::move(nodeHandles));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassMSC::CloseDevice(uint8_t deviceAddress)
{
    kassert(m_HostHandler->GetMutex().IsLocked());
    std::vector<int> nodeHandles;

    for (auto iterator = m_Interfaces.begin(); iterator != m_Interfaces.end(); ) {
        if ((*iterator)->GetDeviceAddress() == deviceAddress)
        {
            std::vector<int> interfaceNodeHandles = (*iterator)->Close_pl();
            nodeHandles.insert(nodeHandles.end(), interfaceNodeHandles.begin(), interfaceNodeHandles.end());
            iterator = m_Interfaces.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
    m_IsActive = HasActiveInterfaces_pl();
    QueueNodeRemovals(std::move(nodeHandles));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassMSC::Startup()
{
    kassert(m_HostHandler->GetMutex().IsLocked());
    for (const Ptr<USBHostMSCInterface>& interface : m_Interfaces) {
        interface->Startup_pl();
    }
    m_IsActive = HasActiveInterfaces_pl();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassMSC::StartupDevice(uint8_t deviceAddress)
{
    kassert(m_HostHandler->GetMutex().IsLocked());
    for (const Ptr<USBHostMSCInterface>& interface : m_Interfaces)
    {
        if (interface->GetDeviceAddress() == deviceAddress) {
            interface->Startup_pl();
        }
    }
    m_IsActive = HasActiveInterfaces_pl();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassMSC::StartOfFrame()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* USBHostClassMSC::Run()
{
    for (;;)
    {
        std::deque<Ptr<USBHostMSCInterface>> initializationQueue;
        std::deque<int> nodeRemovalQueue;
        bool stopRequested;
        {
            CRITICAL_SCOPE(m_WorkMutex);
            while (!m_StopRequested && m_InitializationQueue.empty() && m_NodeRemovalQueue.empty()) {
                m_WorkCondition.Wait(m_WorkMutex);
            }
            stopRequested = m_StopRequested;
            if (stopRequested) {
                m_InitializationQueue.clear();
            } else {
                initializationQueue.swap(m_InitializationQueue);
            }
            nodeRemovalQueue.swap(m_NodeRemovalQueue);
        }

        for (int nodeHandle : nodeRemovalQueue) {
            try {
                kremove_device_root_trw(nodeHandle);
            } catch (...) {}
        }

        if (stopRequested) {
            return nullptr;
        }

        for (const Ptr<USBHostMSCInterface>& interface : initializationQueue) {
            try
            {
                interface->Initialize();
            }
            catch (const std::exception& error)
            {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "MSC initialization failed: {}", error.what());
            }
            catch (...)
            {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "MSC initialization failed.");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostClassMSC::HasActiveInterfaces_pl() const
{
    kassert(m_HostHandler->GetMutex().IsLocked());
    for (const Ptr<USBHostMSCInterface>& interface : m_Interfaces)
    {
        if (interface->IsActive_pl()) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassMSC::QueueInitialization(Ptr<USBHostMSCInterface> interface)
{
    CRITICAL_SCOPE(m_WorkMutex);
    m_InitializationQueue.push_back(interface);
    m_WorkCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassMSC::QueueNodeRemovals(std::vector<int>&& nodeHandles)
{
    if (nodeHandles.empty()) {
        return;
    }

    CRITICAL_SCOPE(m_WorkMutex);
    m_NodeRemovalQueue.insert(m_NodeRemovalQueue.end(), nodeHandles.begin(), nodeHandles.end());
    m_WorkCondition.WakeupAll();
}

} // namespace kernel
