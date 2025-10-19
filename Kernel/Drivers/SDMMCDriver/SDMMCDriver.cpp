// This file is part of PadOS.
//
// Copyright (C) 2017-2024 Kurt Skauen <http://kavionic.com/>
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
// Created: 07.12.2017 23:50:32

#include <System/Platform.h>

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/uio.h>

#include <Kernel/KTime.h>
#include <Kernel/Drivers/SDMMCDriver/SDMMCDriver.h>
#include <Kernel/SpinTimer.h>
#include <Kernel/HAL/STM32/Peripherals_STM32H7.h>
#include <Kernel/VFS/KVFSManager.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KFSVolume.h>
#include <System/ExceptionHandling.h>
#include <Utils/String.h>
#include <DeviceControl/SDCARD.h>

using namespace kernel;
using namespace os;
using namespace sdmmc;

// Supported voltages
#define SD_MMC_VOLTAGE_SUPPORT (OCR_VDD_27_28 | OCR_VDD_28_29 | OCR_VDD_29_30 | OCR_VDD_30_31 | OCR_VDD_31_32 | OCR_VDD_32_33)

// SD/MMC transfer rate unit list (speed / 10000)
static constexpr uint32_t g_TransferRateUnits[7] = { 10, 100, 1000, 10000, 0, 0, 0 };
    
// SD transfer multiplier list (multiplier * 10)
static constexpr uint32_t g_TransferRateMultipliers_sd[16] = { 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
    
// MMC transfer multiplier list (multiplier * 10)
static constexpr uint32_t g_TransferRateMultipliers_mmc[16] = { 0, 10, 12, 13, 15, 20, 26, 30, 35, 40, 45, 52, 55, 60, 70, 80 };


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCINode::SDMMCINode(KFilesystemFileOps* fileOps) : KINode(nullptr, nullptr, fileOps, false)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver::SDMMCDriver() : Thread("hsmci_driver"), m_Mutex("hsmci_driver_mutex", PEMutexRecursionMode_RaiseError), m_CardDetectCondition("hsmci_driver_cd"), m_CardStateCondition("hsmci_driver_cstate"), m_IOCondition("hsmci_driver_io"), m_DeviceSemaphore("hsmci_driver_dev_sema", CLOCK_MONOTONIC_COARSE, 1)
{
    m_CardType = 0;
    m_CardState = CardState::Initializing;

	m_CacheAlignedBuffer = memalign(DCACHE_LINE_SIZE, BLOCK_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver::~SDMMCDriver()
{
	free(m_CacheAlignedBuffer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::SetupBase(const String& devicePath, DigitalPinID pinCD)
{
	m_PinCD = pinCD;
	m_PinCD.SetDirection(DigitalPinDirection_e::In);
	m_PinCD.SetPullMode(PinPullMode_e::Up);
	m_PinCD.SetInterruptMode(PinInterruptMode_e::BothEdges);
	m_PinCD.EnableInterrupts();

    m_DevicePathBase = devicePath;

    m_RawINode = ptr_new<SDMMCINode>(this);
    m_RawINode->bi_nNodeHandle = Kernel::RegisterDevice_trw((devicePath + "raw").c_str(), m_RawINode);

    Start(PThreadDetachState_Detached);

    kernel::register_irq_handler(get_peripheral_irq(pinCD), IRQHandler, this);
}

///////////////////////////////////////////////////////////////////////////////
/// Maintenance thread entry point.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* SDMMCDriver::Run()
{
	m_CardState = CardState::NoCard;
	for (;;)
    {
		bool hasCard;

		CRITICAL_BEGIN(CRITICAL_IRQ)
		{
			hasCard = !m_PinCD;
			if (hasCard == m_CardInserted) {
                if (!m_CardInserted || m_CardState == SDMMCDriver::CardState::Ready) {
                    m_CardDetectCondition.IRQWait();
                }
			}
		} CRITICAL_END;

		snooze_ms(100); // De-bounce
		hasCard = !m_PinCD;

		if (hasCard != m_CardInserted || m_CardState != SDMMCDriver::CardState::Ready)
        {
			CRITICAL_SCOPE(m_Mutex);

			m_CardInserted = hasCard;
            m_CardState = CardState::NoCard;

            RestartCard();

            switch(m_CardState)
            {
                case CardState::NoCard:
                    try {
                        DecodePartitions(true);
                    } catch(...) {}
                    break;
                case CardState::Ready:
                    kprintf("SD/MMC card ready: %" PRIu64 "\n", m_SectorCount * BLOCK_SIZE);
                    try {
                        DecodePartitions(true);
                    } catch(...) {}
                    break;
                case CardState::Initializing:
                    kprintf("SD/MMC RestartCard() failed\n");
                    break;
                case CardState::Unusable:
                    kprintf("SD/MMC card initialization failed\n");
                    snooze_ms(500);
                    break;

            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> SDMMCDriver::OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int flags)
{
    CRITICAL_SCOPE(m_Mutex);

    if (!IsReady()) {
        PERROR_THROW_CODE(PErrorCode::NoDevice);
    }
    return KFilesystemFileOps::OpenFile(volume, node, flags);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);

    if (!IsReady()) {
        PERROR_THROW_CODE(PErrorCode::NoDevice);
    }

    switch(request)
    {
        case DEVCTL_GET_DEVICE_GEOMETRY:
        {
            if (outData == nullptr || outDataLength < sizeof(device_geometry)) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            device_geometry* geometry = static_cast<device_geometry*>(outData);
            geometry->bytes_per_sector  = BLOCK_SIZE;
            geometry->sector_count = m_SectorCount;
            geometry->read_only    = false;
            geometry->removable   = true;
            return;
        }
        case DEVCTL_REREAD_PARTITION_TABLE:
            DecodePartitions(false);
            return;
        case SDCDEVCTL_SDIO_READ_DIRECT:
        {
            if (inData == nullptr || outData == nullptr || inDataLength < sizeof(SDCDEVCTL_SDIOReadDirectArgs) || outDataLength != sizeof(uint8_t)) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            const SDCDEVCTL_SDIOReadDirectArgs* args = static_cast<const SDCDEVCTL_SDIOReadDirectArgs*>(inData);
            SDIOReadDirect(args->FunctionNumber, args->Address, static_cast<uint8_t*>(outData));
            return;
        }
        case SDCDEVCTL_SDIO_WRITE_DIRECT:
        {
            if (inData == nullptr || inDataLength < sizeof(SDCDEVCTL_SDIOWriteDirectArgs)) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            const SDCDEVCTL_SDIOWriteDirectArgs* args = static_cast<const SDCDEVCTL_SDIOWriteDirectArgs*>(inData);
            SDIOWriteDirect(args->FunctionNumber, args->Address, args->Data);
            return;
        }
        case SDCDEVCTL_SDIO_READ_EXTENDED:
        {
            if (inData == nullptr || outData == nullptr || inDataLength < sizeof(SDCDEVCTL_SDIOReadExtendedArgs)) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            const SDCDEVCTL_SDIOReadExtendedArgs* args = static_cast<const SDCDEVCTL_SDIOReadExtendedArgs*>(inData);
            SDIOReadExtended(args->FunctionNumber, args->Address, args->IncrementAddr, outData, outDataLength);
            return;
        }
        case SDCDEVCTL_SDIO_WRITE_EXTENDED:        
        {
            if (inData == nullptr || inDataLength < sizeof(SDCDEVCTL_SDIOWriteExtendedArgs)) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            const SDCDEVCTL_SDIOWriteExtendedArgs* args = static_cast<const SDCDEVCTL_SDIOWriteExtendedArgs*>(inData);
            SDIOWriteExtended(args->FunctionNumber, args->Address, args->IncrementAddr, args->Buffer, args->Size);
            return;
        }
    }
    PERROR_THROW_CODE(PErrorCode::NotImplemented);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t SDMMCDriver::Read(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position)
{
    Ptr<SDMMCINode> inode = (file != nullptr) ? ptr_static_cast<SDMMCINode>(file->GetINode()) : nullptr;

    size_t length = 0;

    for (size_t i = 0; i < segmentCount; ++i) length += segments[i].iov_len;

    bool needLocking = false;
    if (inode != nullptr)
    {
        needLocking = true;
        if (position + length > inode->bi_nSize)
        {
            if (position >= inode->bi_nSize) {
                return 0;
            } else {
                length = size_t(inode->bi_nSize - position);
            }
        }
        position += inode->bi_nStart;
    }
    
    if ((position % BLOCK_SIZE) != 0 || (length % BLOCK_SIZE) != 0) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    
    const uint32_t firstBlock = uint32_t(position / BLOCK_SIZE);
    const uint32_t blockCount = length / BLOCK_SIZE;
    
    PErrorCode error;
    for (int retry = 0; retry < 10; ++retry)
    {
        CRITICAL_SCOPE(m_DeviceSemaphore);
        
        error = PErrorCode::Success;
        CRITICAL_SCOPE(m_Mutex, needLocking);

        if (!IsReady()) {
            PERROR_THROW_CODE(PErrorCode::NoDevice);
        }
    
        if (!Cmd13_sdmmc()) {
            error = PErrorCode::IOError;
            continue;
        }

        uint32_t cmd = (blockCount > 1) ? SDMMC_CMD18_READ_MULTIPLE_BLOCK : SDMMC_CMD17_READ_SINGLE_BLOCK;

        // SDSC Card (CCS=0) uses byte unit address,
        // SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit).
        uint32_t start = firstBlock;
        if ((m_CardType & SDMMCCardType::HC) == 0) {
            start *= BLOCK_SIZE;
        }

        if (!StartAddressedDataTransCmd(cmd, start, get_first_bit_index(BLOCK_SIZE), blockCount, segments, segmentCount)) {
            error = PErrorCode::IOError;
            continue;
        }
        uint32_t response = GetResponse();
        if (response & CARD_STATUS_ERR_RD_WR)
        {
            kprintf("ERROR: SDMMCDriver::Read() Read %02d response 0x%08lx CARD_STATUS_ERR_RD_WR\n", int(SDMMC_CMD_GET_INDEX(cmd)), response);
            error = PErrorCode::IOError;
            continue;
        }

        // WORKAROUND for no compliance card: The errors on this command must be ignored and one retry can be necessary in SPI mode for no compliance card.
        if (blockCount > 1 && !StopAddressedDataTransCmd(SDMMC_CMD12_STOP_TRANSMISSION, 0)) {
            StopAddressedDataTransCmd(SDMMC_CMD12_STOP_TRANSMISSION, 0);
        }
        break;
    }
    if (error != PErrorCode::Success)
    {
        PERROR_THROW_CODE(error);
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t SDMMCDriver::Write(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position)
{
    Ptr<SDMMCINode> inode = (file != nullptr) ? ptr_static_cast<SDMMCINode>(file->GetINode()) : nullptr;

    size_t length = 0;

    for (size_t i = 0; i < segmentCount; ++i) length += segments[i].iov_len;

    bool needLocking = false;
    if (inode != nullptr)
    {
        needLocking = true;
        if (position + length > inode->bi_nSize)
        {
            if (position >= inode->bi_nSize) {
                return 0;
            } else {
                length = size_t(inode->bi_nSize - position);
            }
        }
        position += inode->bi_nStart;
    }


    if ((position % BLOCK_SIZE) != 0 || (length % BLOCK_SIZE) != 0) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    uint32_t firstBlock = uint32_t(position / BLOCK_SIZE);
    uint32_t blockCount = length / BLOCK_SIZE;

    PErrorCode error;
    for (int retry = 0; retry < 10; ++retry)
    {
        CRITICAL_SCOPE(m_DeviceSemaphore);
        
        error = PErrorCode::Success;
        CRITICAL_SCOPE(m_Mutex, needLocking);

        if (!IsReady()) {
            PERROR_THROW_CODE(PErrorCode::NoDevice);
        }

        uint32_t cmd = (blockCount > 1) ? SDMMC_CMD25_WRITE_MULTIPLE_BLOCK : SDMMC_CMD24_WRITE_BLOCK;

        // SDSC Card (CCS=0) uses byte unit address,
        // SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit).
        uint32_t start = firstBlock;
        if ((m_CardType & SDMMCCardType::HC) == 0) {
            start *= BLOCK_SIZE;
        }

        if (!StartAddressedDataTransCmd(cmd, start, get_first_bit_index(BLOCK_SIZE), blockCount, segments, segmentCount)) {
            error = PErrorCode::IOError;
            continue;
        }
        uint32_t response = GetResponse();
        if (response & CARD_STATUS_ERR_RD_WR)
        {
            kprintf("ERROR: SDMMCDriver::Write() Write %02d response 0x%08lx CARD_STATUS_ERR_RD_WR\n", int(SDMMC_CMD_GET_INDEX(cmd)), response);
            error = PErrorCode::IOError;
            continue;
        }

		// SPI multi-block writes terminate using a special token, not a STOP_TRANSMISSION request.
        if (blockCount > 1 && !StopAddressedDataTransCmd(SDMMC_CMD12_STOP_TRANSMISSION, 0)) {
            error = PErrorCode::IOError;
            continue;
        }
        break;
    }
    if (error != PErrorCode::Success) {
        PERROR_THROW_CODE(error);
    }
    return length;
}    

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult SDMMCDriver::HandleIRQ()
{
	if (m_PinCD.GetAndClearInterruptStatus())
	{
		m_CardDetectCondition.Wakeup(1);
		return IRQResult::HANDLED;
	}
	return IRQResult::UNHANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::RestartCard()
{
    if (m_CardState == CardState::Initializing) {
        return true;
    }
    if (m_CardInserted)
    {
        m_CardState = CardState::Initializing;
        // Set 1-bit bus width and low clock for initialization
        m_Clock = SDMMC_CLOCK_INIT;
        m_BusWidth = 1;
        m_HighSpeed = false;

        Reset();
        ApplySpeedAndBusWidth();

        // Initialization of the card requested
        if (InitializeCard())
        {
            SetState(CardState::Ready);
            return true;
        }
        else
        {
            SetState(CardState::Unusable);
        }
    }
    else
    {
        SetState(CardState::NoCard);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Initialize the SD card in MCI mode.
///
/// \note
/// This function runs the initialization procedure and the identification
/// process, then it sets the SD/MMC card in transfer state.
/// At last, it will automatically enable maximum bus width and transfer speed.
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::InitializeCard()
{
    // In first, try to install SD/SDIO card
    m_CardType    = SDMMCCardType::SD;
    m_CardVersion = SDMMCCardVersion::Unknown;
    m_RCA         = 0;

    kprintf("Start SD card install\n");

    // Card need of 74 cycles clock minimum to start
    SendClock();

    // CMD52 Reset SDIO
    uint8_t data = 0x08;
    Cmd52_sdio(SDIO_CMD52_WRITE_FLAG, SDIO_CIA,SDIO_CCCR_IOA, 0, &data);

    // CMD0 - Reset all cards to idle state.
    if (!SendCmd(SDMMC_MCI_CMD0_GO_IDLE_STATE, 0)) {
        return false;
    }
    bool v2 = false;
    if (!Cmd8_sd(&v2)) {
        return false;
    }
    // Try to get the SDIO card's operating condition
    if (!OperationalCondition_sdio()) {
        return false;
    }

    if (m_CardType & SDMMCCardType::SD)
    {
        // Try to get the SD card's operating condition
        if (!OperationalConditionMCI_sd(v2))
        {
            // It is not a SD card
            kprintf("Start MMC Install\n");
            m_CardType = SDMMCCardType::MMC;
            return InitializeMMCCard();
        }
    }

    if (m_CardType & SDMMCCardType::SD)
    {
        // SD MEMORY, Put the Card in Identify Mode
        if (!SendCmd(SDMMC_CMD2_ALL_SEND_CID, 0)) {
            return false;
        }
    }
    // Ask the card to publish a new relative address (RCA).
    if (!SendCmd(SD_CMD3_SEND_RELATIVE_ADDR, 0)) {
        return false;
    }
    m_RCA = uint16_t(GetResponse() >> 16);

    // SD MEMORY, Get the Card-Specific Data
    if (m_CardType & SDMMCCardType::SD)
    {
        if (!Cmd9MCI_sdmmc()) {
            return false;
        }
        DecodeCSD_sd();
    }
    // Select the and put it into Transfer Mode
    if (!SendCmd(SDMMC_CMD7_SELECT_CARD_CMD, (uint32_t)m_RCA << 16)) {
        return false;
    }
    // SD MEMORY, Read the SCR to get card version
    if (m_CardType & SDMMCCardType::SD) {
        if (!ACmd51_sd()) {
            return false;
        }
    }
    if (m_CardType & SDMMCCardType::SDIO) {
        if (!GetMaxSpeed_sdio()) {
            return false;
        }
    }
    // TRY to enable 4-bit mode
    if (m_CardType & SDMMCCardType::SDIO) {
        if (!SetBusWidth_sdio()) {
            return false;
        }
    }
    if (m_CardType & SDMMCCardType::SD) {
        if (!ACmd6_sd()) {
            return false;
        }
    }
    ApplySpeedAndBusWidth();
    // Try to enable high-speed mode
    if (m_CardType & SDMMCCardType::SDIO) {
        if (!SetHighSpeed_sdio()) {
            return false;
        }
    }
    if (m_CardType & SDMMCCardType::SD)
    {
        if (m_CardVersion > SDMMCCardVersion::SD_1_0) {
            if (!SetHighSpeed_sd()) {
                return false;
            }
        }
    }
    ApplySpeedAndBusWidth();
    // SD MEMORY, Set default block size
    if (m_CardType & SDMMCCardType::SD)
    {
        if (!SendCmd(SDMMC_CMD16_SET_BLOCKLEN, BLOCK_SIZE)) {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::ReadPartitionData(void* userData, off64_t position, void* buffer, size_t size)
{
    iovec_t segment;
    segment.iov_base = buffer;
    segment.iov_len = size;
    if (static_cast<SDMMCDriver*>(userData)->Read(nullptr, &segment, 1, position) != size)
    {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::DecodePartitions(bool force)
{
    device_geometry diskGeom;

    m_RawINode->bi_nSize = m_SectorCount * BLOCK_SIZE;

    memset(&diskGeom, 0, sizeof(diskGeom));
    diskGeom.sector_count     = m_SectorCount;
    diskGeom.bytes_per_sector = BLOCK_SIZE;
    diskGeom.read_only 	      = false;
    diskGeom.removable 	      = true;

    kprintf("SDMMCDriver::DecodePartitions(): Decoding partition table\n");

    std::vector<disk_partition_desc> partitions = KVFSManager::DecodeDiskPartitions_trw(m_CacheAlignedBuffer, BLOCK_SIZE, diskGeom, &ReadPartitionData, this);

    for (size_t i = 0 ; i < partitions.size() ; ++i)
    {
	    if (partitions[i].p_type != 0 && partitions[i].p_size != 0)
        {
	        kprintf( "   Partition %" PRIu32 " : %10" PRIu64 " -> %10" PRIu64 " %02x (%" PRIu64 ")\n", uint32_t(i), partitions[i].p_start,
		            partitions[i].p_start + partitions[i].p_size - 1LL, partitions[i].p_type,
		            partitions[i].p_size);
	    }
    }

    for (Ptr<SDMMCINode> partition : m_PartitionINodes)
    {
	    bool found = false;
	    for (size_t i = 0 ; i < partitions.size() ; ++i)
        {
	        if (partitions[i].p_start == partition->bi_nStart && partitions[i].p_size == partition->bi_nSize)
            {
    		    found = true;
	    	    break;
	        }
	    }
	    if (!force && !found && partition->bi_nOpenCount > 0)
        {
	        kprintf("ERROR: SDMMCDriver::DecodePartitions() Open partition has changed.\n");
            PERROR_THROW_CODE(PErrorCode::Busy);
	    }
    }
    
    std::vector<Ptr<SDMMCINode>> unusedPartitionINodes; // = std::move(m_PartitionINodes);
        // Remove deleted partitions from /dev/
    for (auto i = m_PartitionINodes.begin(); i != m_PartitionINodes.end(); )
    {
        Ptr<SDMMCINode> partition = *i;
        bool found = false;
        for (size_t i = 0; i < partitions.size(); ++i)
        {
            if (partitions[i].p_start == partition->bi_nStart && partitions[i].p_size == partition->bi_nSize)
            {
                partitions[i].p_size = 0;
                partition->bi_nPartitionType = partitions[i].p_type;
                found = true;
                break;
            }
        }
        if (!found)
        {
            Kernel::RemoveDevice_trw(partition->bi_nNodeHandle);
            partition->bi_nNodeHandle = -1;
            i = m_PartitionINodes.erase(i);
            if (partition->bi_nOpenCount == 0) {
                unusedPartitionINodes.push_back(partition);
            }
        }
        else
        {
            ++i;
        }
    }

        // Create nodes for any new partitions.
    for (size_t i = 0 ; i < partitions.size() ; ++i)
    {
	    if (partitions[i].p_type == 0 || partitions[i].p_size == 0) {
	        continue;
	    }
        Ptr<SDMMCINode> partition;
        if (!unusedPartitionINodes.empty()) {
            partition = unusedPartitionINodes.back();
            unusedPartitionINodes.pop_back();
        } else {
            partition = ptr_new<SDMMCINode>(this);
        }
        m_PartitionINodes.push_back(partition);
	    partition->bi_nStart = partitions[i].p_start;
	    partition->bi_nSize  = partitions[i].p_size;
    }
        
    std::sort(m_PartitionINodes.begin(), m_PartitionINodes.end(), [](Ptr<SDMMCINode> lhs, Ptr<SDMMCINode> rhs) { return lhs->bi_nStart < rhs->bi_nStart; });

        // We now have to rename nodes that might have moved around in the table and
        // got new names. To avoid name-clashes while renaming we first give all
        // nodes a unique temporary name before looping over again giving them their
        // final names

    for (size_t i = 0; i < m_PartitionINodes.size(); ++i)
    {
        Ptr<SDMMCINode> partition = m_PartitionINodes[i];
        if (partition->bi_nNodeHandle != -1)
        {
            String path = m_DevicePathBase + String::format_string("%lu_new", i);
            Kernel::RenameDevice_trw(partition->bi_nNodeHandle, path.c_str());
        }
    }
    for (size_t i = 0; i < m_PartitionINodes.size(); ++i)
    {
        String path = m_DevicePathBase + String::format_string("%lu", i);
        
        Ptr<SDMMCINode> partition = m_PartitionINodes[i];
        if (partition->bi_nNodeHandle != -1) {
            Kernel::RenameDevice_trw(partition->bi_nNodeHandle, path.c_str());
        } else {
            Kernel::RegisterDevice_trw(path.c_str(), partition);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Initialize the MMC card in MCI mode.
///
/// \note
/// This function runs the initialization procedure and the identification
/// process, then it sets the SD/MMC card in transfer state.
/// At last, it will automatically enable maximum bus width and transfer speed.
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::InitializeMMCCard()
{
    // CMD0 - Reset all cards to idle state.
    if (!SendCmd(SDMMC_MCI_CMD0_GO_IDLE_STATE, 0)) {
        return false;
    }

    if (!OperationalConditionMCI_mmc()) {
        return false;
    }

    // Put the Card in Identify Mode
    if (!SendCmd(SDMMC_CMD2_ALL_SEND_CID, 0)) {
        return false;
    }
    // Assign relative address to the card.
    m_RCA = 1;
    if (!SendCmd(MMC_CMD3_SET_RELATIVE_ADDR, uint32_t(m_RCA) << 16)) {
        return false;
    }
    // Get the card-specific data.
    if (!Cmd9MCI_sdmmc()) {
        return false;
    }
    DecodeCSD_mmc();
    // Select the card and put it in transfer mode.
    if (!SendCmd(SDMMC_CMD7_SELECT_CARD_CMD, uint32_t(m_RCA) << 16)) {
        return false;
    }
    if (m_CardVersion >= SDMMCCardVersion::MMC_4)
    {
        // For MMC 4.0 Higher version
        // Get EXT_CSD
        bool authorizeHighSpeed;
        if (!Cmd8_mmc(&authorizeHighSpeed)) {
            return false;
        }
        // Enable 4-bit bus
        if (!SetBusWidth_mmc(4)) {
            return false;
        }
        ApplySpeedAndBusWidth();

        if (authorizeHighSpeed)
        {
            // Enable HS
            if (!SetHighSpeed_mmc()) {
                return false;
            }
            ApplySpeedAndBusWidth();
        }
    }
    else
    {
        ApplySpeedAndBusWidth();
    }

    for (int retry = 0; retry < 10; ++retry)
    {
        // Retry is a WORKAROUND for no compliance card
        // These cards seem not ready immediately after the end of busy of SetHighSpeed_mmc()

        // Set default block size
        if (SendCmd(SDMMC_CMD16_SET_BLOCKLEN, BLOCK_SIZE)) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::SetState(CardState state)
{
    if (state != m_CardState)
    {
        m_CardState = state;
        m_CardStateCondition.Wakeup(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::SDIOReadDirect(uint8_t functionNumber, uint32_t addr, uint8_t *dest)
{
    if (dest == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    if (!Cmd52_sdio(SDIO_CMD52_READ_FLAG, functionNumber, addr, 0, dest)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::SDIOWriteDirect(uint8_t functionNumber, uint32_t addr, uint8_t data)
{
    if (!Cmd52_sdio(SDIO_CMD52_WRITE_FLAG, functionNumber, addr, 0, &data)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SDMMCDriver::SDIOReadExtended(uint8_t functionNumber, uint32_t addr, uint8_t incrementAddr, void* buffer, size_t size)
{
    if ((size == 0) || (size > BLOCK_SIZE)) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

	if (Cmd53_sdio(SDIO_CMD53_READ_FLAG, functionNumber, addr, incrementAddr, size, buffer)) {
		return size;
	} else {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SDMMCDriver::SDIOWriteExtended(uint8_t functionNumber, uint32_t addr, uint8_t incrementAddr, const void* buffer, size_t size)
{
    if ((size == 0) || (size > BLOCK_SIZE)) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    if (!Cmd53_sdio(SDIO_CMD53_WRITE_FLAG, functionNumber, addr, incrementAddr, size, buffer)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Ask all cards to send their operations conditions (MCI only).
/// - ACMD41 sends operation condition command.
/// - ACMD41 reads OCR
///
/// \param v2   Shall be 'true' if it is a SD card V2
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::OperationalConditionMCI_sd(bool v2)
{
    TimeValNanos deadline = kget_monotonic_time() + 1.0;
    for(;;)
    {
        // CMD55 - Tell the card that the next command is an application specific command.
        if (!SendCmd(SDMMC_CMD55_APP_CMD, 0)) {
            kprintf("ERROR: OperationalConditionMCI_sd() CMD55 failed.\n");
            return false;
        }

        // (ACMD41) Sends host OCR register
        uint32_t arg = SD_MMC_VOLTAGE_SUPPORT;
        if (v2) {
            arg |= SD_ACMD41_HCS;
        }
        if (!SendCmd(SD_MCI_ACMD41_SD_SEND_OP_COND, arg)) {
            kprintf("ERROR: OperationalConditionMCI_sd() ACMD41 failed.\n");
            return false;
        }
        uint32_t response = GetResponse();
        if (response & OCR_POWER_UP_BUSY)
        {
            // Card is ready
            if ((response & OCR_CCS) != 0) {
                m_CardType |= SDMMCCardType::HC;
            }
            break;
        }
        if (kget_monotonic_time() > deadline) {
            kprintf("ERROR: OperationalConditionMCI_sd(): Timeout (0x%08lx)\n", response);
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Sends operation condition command and read OCR (MCI only)
/// - CMD1 sends operation condition command
/// - CMD1 reads OCR
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::OperationalConditionMCI_mmc()
{
    TimeValNanos deadline = kget_monotonic_time() + 1.0;
    for(;;)
    {
        if (!SendCmd(MMC_MCI_CMD1_SEND_OP_COND, SD_MMC_VOLTAGE_SUPPORT | OCR_ACCESS_MODE_SECTOR))
        {
            kprintf("ERROR: OperationalConditionMCI_mmc() CMD1 MCI failed.\n");
            return false;
        }
        uint32_t response = GetResponse();
        if (response & OCR_POWER_UP_BUSY)
        {
            // Check OCR value
            if ((response & OCR_ACCESS_MODE_MASK) == OCR_ACCESS_MODE_SECTOR)
            {
                m_CardType |= SDMMCCardType::HC;
            }
            break;
        }
        if (kget_monotonic_time() > deadline) {
            kprintf("ERROR: OperationalConditionMCI_mmc(): Timeout (0x%08lx)\n", response);
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Try to get the SDIO card's operating condition
///    - CMD5 to read OCR NF field
///    - CMD5 to wait OCR power up busy
///    - CMD5 to read OCR MP field
///      m_CardType is updated
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::OperationalCondition_sdio()
{
    // CMD5 - SDIO send operation condition (OCR) command.
    if (!SendCmd(SDIO_CMD5_SEND_OP_COND, 0)) {
        kprintf("ERROR: SDMMCDriver::OperationalCondition_sdio:1() CMD5 Fail\n");
        return true; // No error but card type not updated
    }
    uint32_t response = GetResponse();
    if ((response & OCR_SDIO_NF) == 0) {
        return true; // No error but card type not updated
    }

    TimeValNanos deadline = kget_monotonic_time() + 1.0;
    for (;;)
    {
        // CMD5 - SDIO send operation condition (OCR) command.
        if (!SendCmd(SDIO_CMD5_SEND_OP_COND, response & SD_MMC_VOLTAGE_SUPPORT)) {
            kprintf("ERROR: SDMMCDriver::OperationalCondition_sdio:2() CMD5 Fail\n");
            return false;
        }
        response = GetResponse();
        if ((response & OCR_POWER_UP_BUSY) == OCR_POWER_UP_BUSY) {
            break;
        }
        if (kget_monotonic_time() > deadline) {
            kprintf("ERROR: OperationalCondition_sdio(): Timeout (0x%08lx)\n", response);
            return false;
        }
    }
    if ((response & OCR_SDIO_MP) > 0) {
        m_CardType = SDMMCCardType::SD_COMBO;
    } else {
        m_CardType = SDMMCCardType::SDIO;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Get SDIO max transfer speed in Hz.
///    - CMD53 reads CIS area address in CCCR area.
///    - Nx CMD53 search Fun0 tuple in CIS area
///    - CMD53 reads TPLFE_MAX_TRAN_SPEED in Fun0 tuple
///    - Compute maximum speed of SDIO and update m_Clock
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::GetMaxSpeed_sdio()
{
    uint8_t addr_cis[4];

    // Read CIS area address in CCCR area
    uint32_t addrOld = SDIO_CCCR_CIS_PTR;
    for(int i = 0; i < 4; ++i) {
        Cmd52_sdio(SDIO_CMD52_READ_FLAG, SDIO_CIA, addrOld, 0, &addr_cis[i]);
        addrOld++;
    }
    addrOld = addr_cis[0] + (addr_cis[1] << 8) + (addr_cis[2] << 16) + (addr_cis[3] << 24);
    uint32_t addrNew = addrOld;

    uint8_t buf[6];
    for (;;)
    {
        // Read a sample of CIA area.
        for(int i = 0; i < 3; ++i) {
            Cmd52_sdio(SDIO_CMD52_READ_FLAG, SDIO_CIA, addrNew, 0, &buf[i]);
            addrNew++;
        }
        if (buf[0] == SDIO_CISTPL_END) {
            return false; // Tuple error
        }
        if (buf[0] == SDIO_CISTPL_FUNCE && buf[2] == 0x00) {
            break; // Fun0 tuple found
        }
        if (buf[1] == 0) {
            return false; // Tuple error
        }
        // Next address
        addrNew += buf[1]-1;
        if (addrNew > (addrOld + 256)) {
            return false; // Out off CIS area
        }
    }

    // Read all FN0 tuple fields: fn0_blk_siz & max_tran_speed.
    addrNew -= 3;
    for(int i = 0; i < 6; ++i) {
        Cmd52_sdio(SDIO_CMD52_READ_FLAG, SDIO_CIA, addrNew, 0, &buf[i]);
        addrNew++;
    }

    uint8_t tplfe_max_tran_speed = buf[5];
    if (tplfe_max_tran_speed > 0x32) {
        // Error on SDIO register, the high speed is not activated and the clock can not be more than 25MHz.
        // This error is present on specific SDIO card (H&D wireless card - HDG104 WiFi SIP).
        tplfe_max_tran_speed = 0x32; // 25Mhz
    }

    // Decode transfer speed in Hz.
    uint32_t unit = g_TransferRateUnits[tplfe_max_tran_speed & 0x7];
    uint32_t mul = g_TransferRateMultipliers_sd[(tplfe_max_tran_speed >> 3) & 0xF];
    m_Clock = unit * mul * 1000;
    // Note: A combo card shall be a Full-Speed SDIO card which supports up to 25MHz. A SDIO card alone can be:
    // - A low-speed SDIO card which supports 400kHz minimum
    // - A full-speed SDIO card which supports up to 25MHz
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD52 for SDIO - Switches the bus width mode to 4
///
/// \note m_BusWidth is updated.
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::SetBusWidth_sdio()
{
     // SD, SD/COMBO, SDIO full-speed card always supports 4bit bus.
     // SDIO low-Speed alone can optionally support 4bit bus.

    uint8_t u8_value = 0;

    // Check 4bit support in 4BLS of "Card Capability" register
    if (!Cmd52_sdio(SDIO_CMD52_READ_FLAG, SDIO_CIA, SDIO_CCCR_CAP, 0, &u8_value)) {
        return false;
    }
    if ((u8_value & SDIO_CAP_4BLS) != SDIO_CAP_4BLS) {
        // No supported, it is not a protocol error
        return true;
    }
    // HS mode possible, then enable
    u8_value = SDIO_BUSWIDTH_4B;
    if (!Cmd52_sdio(SDIO_CMD52_WRITE_FLAG, SDIO_CIA, SDIO_CCCR_BUS_CTRL, 1, &u8_value)) {
        return false;
    }
    m_BusWidth = 4;
    kprintf("%d-bit bus width enabled.\n", m_BusWidth);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD52 for SDIO - Enable the high speed mode
///
/// \note m_HighSpeed is updated.
/// \note m_Clock is updated.
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::SetHighSpeed_sdio()
{
    uint8_t u8_value = 0;

    // Check CIA.HS
    if (!Cmd52_sdio(SDIO_CMD52_READ_FLAG, SDIO_CIA, SDIO_CCCR_HS, 0, &u8_value)) {
        return false;
    }
    if ((u8_value & SDIO_SHS) != SDIO_SHS) {
        // No supported, it is not a protocol error
        return true;
    }
    // HS mode possible, then enable
    u8_value = SDIO_EHS;
    if (!Cmd52_sdio(SDIO_CMD52_WRITE_FLAG, SDIO_CIA, SDIO_CCCR_HS, 1, &u8_value)) {
        return false;
    }
    m_HighSpeed = true;
    m_Clock *= 2;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD6 for SD - Switch card in high speed mode
///
/// \note CMD6 for SD is valid under the "trans" state.
/// \note m_HighSpeed is updated.
/// \note m_Clock is updated.
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::SetHighSpeed_sd()
{
	static_assert(SD_SW_STATUS_SIZE_BYTES < BLOCK_SIZE);

	uint8_t* switch_status = reinterpret_cast<uint8_t*>(m_CacheAlignedBuffer);

    iovec_t segment = { .iov_base = switch_status, .iov_len = SD_SW_STATUS_SIZE_BYTES };
    if (!StartAddressedDataTransCmd(SD_CMD6_SWITCH_FUNC,
                                    SD_CMD6_MODE_SWITCH
                                  | SD_CMD6_GRP6_NO_INFLUENCE
                                  | SD_CMD6_GRP5_NO_INFLUENCE
                                  | SD_CMD6_GRP4_NO_INFLUENCE
                                  | SD_CMD6_GRP3_NO_INFLUENCE
                                  | SD_CMD6_GRP2_DEFAULT
                                  | SD_CMD6_GRP1_HIGH_SPEED,
									get_first_bit_index(SD_SW_STATUS_SIZE_BYTES), 1, &segment, 1)) {
        return false;
    }

    if (GetResponse() & CARD_STATUS_SWITCH_ERROR) {
        kprintf("ERROR: SetHighSpeed_sd() CMD6 CARD_STATUS_SWITCH_ERROR\n");
        return false;
    }
    if (SD_SW_STATUS_FUN_GRP1_RC(switch_status) == SD_SW_STATUS_FUN_GRP_RC_ERROR) {
        // Not supported, it is not a protocol error
        return true;
    }
    if (SD_SW_STATUS_FUN_GRP1_BUSY(switch_status)) {
        kprintf("ERROR: SetHighSpeed_sd() CMD6 SD_SW_STATUS_FUN_GRP1_BUSY\n");
        return false;
    }
    // CMD6 function switching period is within 8 clocks after the end bit of status data.
    SendClock();
    m_HighSpeed = true;
    m_Clock *= 2;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD6 for MMC - Switches the bus width mode
///
/// \note CMD6 is valid under the "trans" state.
/// \note m_BusWidth is updated.
///
/// \param bus_width   Bus width to set
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::SetBusWidth_mmc(int busWidth)
{
    uint32_t arg;

    switch (busWidth)
    {
        case 8:
            arg = MMC_CMD6_ACCESS_SET_BITS | MMC_CMD6_INDEX_BUS_WIDTH | MMC_CMD6_VALUE_BUS_WIDTH_8BIT;
            break;
        case 4:
            arg = MMC_CMD6_ACCESS_SET_BITS | MMC_CMD6_INDEX_BUS_WIDTH | MMC_CMD6_VALUE_BUS_WIDTH_4BIT;
            break;
        default:
            arg = MMC_CMD6_ACCESS_SET_BITS | MMC_CMD6_INDEX_BUS_WIDTH | MMC_CMD6_VALUE_BUS_WIDTH_1BIT;
            break;
    }
    if (!SendCmd(MMC_CMD6_SWITCH, arg)) {
        return false;
    }
    if (GetResponse() & CARD_STATUS_SWITCH_ERROR) {
        // Not supported, it is not a protocol error
        kprintf("ERROR: SetBusWidth_mmc() CMD6 CARD_STATUS_SWITCH_ERROR\n");
        return false;
    }
    m_BusWidth = busWidth;
    kprintf("%d-bit bus width enabled.\n", m_BusWidth);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD6 for MMC - Switches in high speed mode
///
/// \note CMD6 is valid under the "trans" state.
/// \note m_HighSpeed is updated.
/// \note m_Clock is updated.
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::SetHighSpeed_mmc()
{
    if (!SendCmd(MMC_CMD6_SWITCH, MMC_CMD6_ACCESS_WRITE_BYTE | MMC_CMD6_INDEX_HS_TIMING | MMC_CMD6_VALUE_HS_TIMING_ENABLE)) {
        return false;
    }
    if (GetResponse() & CARD_STATUS_SWITCH_ERROR) {
        // Not supported, it is not a protocol error
        kprintf("ERROR: SetHighSpeed_mmc() CMD6 CARD_STATUS_SWITCH_ERROR\n");
        return false;
    }
    m_HighSpeed = true;
    m_Clock = 52000000lu;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Decodes MMC CSD register
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::DecodeCSD_mmc()
{
    uint32_t unit;
    uint32_t mul;
    uint32_t tran_speed;

    // Get MMC System Specification version supported by the card
    switch (MMC_CSD_SPEC_VERS(m_CSD))
    {
        default:
        case 0: m_CardVersion = SDMMCCardVersion::MMC_1_2; break;
        case 1: m_CardVersion = SDMMCCardVersion::MMC_1_4; break;
        case 2: m_CardVersion = SDMMCCardVersion::MMC_2_2; break;
        case 3: m_CardVersion = SDMMCCardVersion::MMC_3;   break;
        case 4: m_CardVersion = SDMMCCardVersion::MMC_4;   break;
    }

    // Get MMC memory max transfer speed in Hz.
    tran_speed = CSD_TRAN_SPEED(m_CSD);
    unit       = g_TransferRateUnits[tran_speed & 0x7];
    mul        = g_TransferRateMultipliers_mmc[(tran_speed >> 3) & 0xF];
    m_Clock    = unit * mul * 1000;


     /// Get card capacity.
     /// ----------------------------------------------------
     /// For normal SD/MMC card:
     /// memory capacity = BLOCKNR * BLOCK_LEN
     /// Where
     /// BLOCKNR = (C_SIZE+1) * MULT
     /// MULT = 2 ^ (C_SIZE_MULT+2)       (C_SIZE_MULT < 8)
     /// BLOCK_LEN = 2 ^ READ_BL_LEN      (READ_BL_LEN < 12)
     /// ----------------------------------------------------
     /// For high capacity SD/MMC card:
     /// memory capacity = SEC_COUNT * 512 byte

    if (MMC_CSD_C_SIZE(m_CSD) != 0xfff) {
        uint32_t blockCount = ((MMC_CSD_C_SIZE(m_CSD) + 1) * (1 << (MMC_CSD_C_SIZE_MULT(m_CSD) + 2)));
        m_SectorCount = uint64_t(blockCount) * (1 << MMC_CSD_READ_BL_LEN(m_CSD)) / BLOCK_SIZE;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Decodes SD CSD register
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::DecodeCSD_sd()
{
    uint32_t unit;
    uint32_t mul;
    uint32_t tran_speed;

    // Get SD memory maximum transfer speed in Hz.
    tran_speed = CSD_TRAN_SPEED(m_CSD);
    unit = g_TransferRateUnits[tran_speed & 0x7];
    mul = g_TransferRateMultipliers_sd[(tran_speed >> 3) & 0xF];
    m_Clock = unit * mul * 1000;

    // Get card capacity.
    // ----------------------------------------------------
    // For normal SD/MMC card:
    // memory capacity = BLOCKNR * BLOCK_LEN
    // Where
    // BLOCKNR = (C_SIZE+1) * MULT
    // MULT = 2 ^ (C_SIZE_MULT+2)       (C_SIZE_MULT < 8)
    // BLOCK_LEN = 2 ^ READ_BL_LEN      (READ_BL_LEN < 12)
    // ----------------------------------------------------
    // For high capacity SD card:
    // memory capacity = (C_SIZE+1) * 512K byte

    if (CSD_STRUCTURE_VERSION(m_CSD) >= SD_CSD_VER_2_0) {
        m_SectorCount = (SD_CSD_2_0_C_SIZE(m_CSD) + 1) * 1024;
    } else {
        uint64_t blockCount = ((SD_CSD_1_0_C_SIZE(m_CSD) + 1) * (1 << (SD_CSD_1_0_C_SIZE_MULT(m_CSD) + 2)));
        m_SectorCount = blockCount * (1 << SD_CSD_1_0_READ_BL_LEN(m_CSD)) / BLOCK_SIZE;
    }
}


///////////////////////////////////////////////////////////////////////////////
/// \brief ACMD6 - Define the data bus width to 4 bits bus
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::ACmd6_sd()
{
    // CMD55 - Tell the card that the next command is an application specific command.
    if (!SendCmd(SDMMC_CMD55_APP_CMD, (uint32_t)m_RCA << 16)) {
        return false;
    }
    if (!SendCmd(SD_ACMD6_SET_BUS_WIDTH, 0x2)) { // 10b = 4 bits bus
        return false;
    }
    m_BusWidth = 4;
    kprintf("%d-bit bus width enabled.\n", m_BusWidth);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD8 for SD card - Send Interface Condition Command.
///
/// \note
/// Send SD Memory Card interface condition, which includes host supply
/// voltage information and asks the card whether card supports voltage.
/// Should be performed at initialization time to detect the card type.
///
/// \param v2 Pointer to v2 flag to update
///
/// \return true if success, otherwise false
///         with a update of \ref sd_mmc_err.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::Cmd8_sd(bool* v2)
{
    *v2 = false;
    // Test for SD version 2
    if (!SendCmd(SD_CMD8_SEND_IF_COND, SD_CMD8_PATTERN | SD_CMD8_HIGH_VOLTAGE)) {
        return true; // It is not a V2
    }
    // Check R7 response
    uint32_t response = GetResponse();
    if (response == 0xffffffff) {
        // No compliance R7 value
        return true; // It is not a V2
    }
    if ((response & (SD_CMD8_MASK_PATTERN | SD_CMD8_MASK_VOLTAGE)) != (SD_CMD8_PATTERN | SD_CMD8_HIGH_VOLTAGE)) {
        kprintf("ERROR: Cmd8_sd() CMD8 resp32 0x%08lx UNUSABLE CARD\n", response);
        return false;
    }
    kprintf("SD card V2\n");
    *v2 = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD8 - The card sends its EXT_CSD register as a block of data.
///
/// \param authorizeHighSpeed Pointer to update with the high speed
/// support information
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::Cmd8_mmc(bool* authorizeHighSpeed)
{
	static_assert(EXT_CSD_SIZE_BYTES <= BLOCK_SIZE);
	
	uint32_t* buffer = reinterpret_cast<uint32_t*>(m_CacheAlignedBuffer);

	// Read and decode Extended Card Specific Data.
    iovec_t segment = { .iov_base = buffer, .iov_len = EXT_CSD_SIZE_BYTES };
    if (!StartAddressedDataTransCmd(MMC_CMD8_SEND_EXT_CSD, 0, get_first_bit_index(EXT_CSD_SIZE_BYTES), 1, &segment, 1)) {
		return false;
	}
    *authorizeHighSpeed = ((buffer[EXT_CSD_CARD_TYPE_INDEX / 4] >> ((EXT_CSD_CARD_TYPE_INDEX % 4) * 8)) & MMC_CTYPE_52MHZ) != 0;

    if (MMC_CSD_C_SIZE(m_CSD) == 0xfff) {
        // For high capacity SD/MMC card,
        // memory capacity = SEC_COUNT * 512 byte
        m_SectorCount = buffer[EXT_CSD_SEC_COUNT_INDEX / 4];
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD9: Addressed card sends its card-specific
///        data (CSD) on the CMD line mci.
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::Cmd9MCI_sdmmc()
{
    if (!SendCmd(SDMMC_MCI_CMD9_SEND_CSD, uint32_t(m_RCA) << 16)) {
        return false;
    }
    GetResponse128(m_CSD);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD13 - Addressed card sends its status register.
/// This function waits the clear of the busy flag
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::Cmd13_sdmmc()
{
    TimeValNanos deadline = kget_monotonic_time() + 1.0;
    for(;;)
    {
        if (!SendCmd(SDMMC_MCI_CMD13_SEND_STATUS, (uint32_t)m_RCA << 16)) {
            return false;
        }
        if (GetResponse() & CARD_STATUS_READY_FOR_DATA) {
            return true;
        }
        if (kget_monotonic_time() > deadline) {
            kprintf("ERROR: Cmd13_sdmmc() timeout\n");
            return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief ACMD51 - Read the SD Configuration Register.
///
/// \note
/// SD Card Configuration Register (SCR) provides information on the SD Memory
/// Card's special features that were configured into the given card. The size
/// of SCR register is 64 bits.
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::ACmd51_sd()
{
	static_assert(SD_SCR_REG_SIZE_BYTES <= BLOCK_SIZE);
	uint8_t* scr = reinterpret_cast<uint8_t*>(m_CacheAlignedBuffer);

    int retries = 0;
    for (;;)
    {
		// CMD55 - Tell the card that the next command is an application specific command.
		if (!SendCmd(SDMMC_CMD55_APP_CMD, uint32_t(m_RCA) << 16)) {
			return false;
		}
        iovec_t segment(scr, SD_SCR_REG_SIZE_BYTES);
        if (StartAddressedDataTransCmd(SD_ACMD51_SEND_SCR, 0, get_first_bit_index(SD_SCR_REG_SIZE_BYTES), 1, &segment, 1)) {
            break;
        } else if (++retries > 5) {
            return false;
        }
    }
    // Get SD Memory Card - Spec. Version
    switch (SD_SCR_SD_SPEC(scr))
    {
        case SD_SCR_SD_SPEC_1_0_01: m_CardVersion = SDMMCCardVersion::SD_1_0; break;
        case SD_SCR_SD_SPEC_1_10:   m_CardVersion = SDMMCCardVersion::SD_1_10; break;
        case SD_SCR_SD_SPEC_2_00:   m_CardVersion = (SD_SCR_SD_SPEC3(scr) == SD_SCR_SD_SPEC_3_00) ? SDMMCCardVersion::SD_3_0 : SDMMCCardVersion::SD_2_0; break;
        default:                    m_CardVersion = SDMMCCardVersion::SD_1_0; break;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD52 - SDIO IO_RW_DIRECT command
///
/// \param rwFlag   Direction, 1:write, 0:read.
/// \param functionNumber   Number of the function.
/// \param readAfterWriteFlag Read after Write flag.
/// \param registerAddr  register address.
/// \param data   Pointer to input argument and response buffer.
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::Cmd52_sdio(uint8_t rwFlag, uint8_t functionNumber, uint32_t registerAddr, uint8_t readAfterWriteFlag, uint8_t* data)
{
    if (data == nullptr) {
        kprintf("ERROR: Cmd52_sdio() called with nullptr data\n");
        return false;
    }
    if (!SendCmd(SDIO_CMD52_IO_RW_DIRECT,
          ((uint32_t)*data << SDIO_CMD52_WR_DATA)
        | ((uint32_t)rwFlag << SDIO_CMD52_RW_FLAG)
        | ((uint32_t)functionNumber << SDIO_CMD52_FUNCTION_NUM)
        | ((uint32_t)readAfterWriteFlag << SDIO_CMD52_RAW_FLAG)
        | ((uint32_t)registerAddr << SDIO_CMD52_REG_ADRR))) {
        return false;
    }
    *data = GetResponse() & 0xff;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief CMD53 - SDIO IO_RW_EXTENDED command
///    This implementation support only the SDIO multi-byte transfer mode which
///    is similar to the single block transfer on memory.
///    Note: The SDIO block transfer mode is optional for SDIO card.
///
/// \param rwFlag   Direction, 1:write, 0:read.
/// \param functionNumber   Number of the function.
/// \param registerAddr  Register address.
/// \param incrementAddr  1:Incrementing address, 0: fixed.
/// \param size      Transfer data size.
/// \param access_block  true, if the block access (DMA) is used
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::Cmd53_sdio(uint8_t rwFlag, uint8_t functionNumber, uint32_t registerAddr, uint8_t incrementAddr, uint32_t size, const void* buffer)
{
    if (size == 0 || size > BLOCK_SIZE) {
        kprintf("ERROR: Cmd53_sdio() invalid size %" PRIu32 "\n", size);
        return false;
    }
    
    uint32_t cmdArgs = ((size % BLOCK_SIZE) << SDIO_CMD53_COUNT)
                     | ((uint32_t)registerAddr << SDIO_CMD53_REG_ADDR)
                     | ((uint32_t)incrementAddr << SDIO_CMD53_OP_CODE)
                     | ((uint32_t)0 << SDIO_CMD53_BLOCK_MODE)
                     | ((uint32_t)functionNumber << SDIO_CMD53_FUNCTION_NUM)
                     | ((uint32_t)rwFlag << SDIO_CMD53_RW_FLAG);


    iovec_t segment(const_cast<void*>(buffer), size);
    return StartAddressedDataTransCmd((rwFlag == SDIO_CMD53_READ_FLAG) ? SDIO_CMD53_IO_R_BYTE_EXTENDED : SDIO_CMD53_IO_W_BYTE_EXTENDED, cmdArgs, get_first_bit_index(1), size, &segment, 1);
}
