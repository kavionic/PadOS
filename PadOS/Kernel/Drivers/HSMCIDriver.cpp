// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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

#include "Platform.h"

#include <string.h>
#include <stdio.h>

#include "HSMCIDriver.h"
#include "Kernel/SpinTimer.h"
#include "Kernel/HAL/SAME70System.h"
#include "ASF/sam/drivers/xdmac/xdmac.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/VFS/KFSVolume.h"
#include "System/String.h"
#include "DeviceControl/DeviceControl.h"
#include "DeviceControl/SDCARD.h"
#include "Kernel/VFS/KVFSManager.h"

using namespace kernel;
using namespace os;

HSMCIINode::HSMCIINode(KFilesystemFileOps* fileOps) : KINode(nullptr, nullptr, fileOps, false)
{
}

static const uint32_t CONF_HSMCI_XDMAC_CHANNEL = 0;

// Supported voltages
#define SD_MMC_VOLTAGE_SUPPORT (OCR_VDD_27_28 | OCR_VDD_28_29 | OCR_VDD_29_30 | OCR_VDD_30_31 | OCR_VDD_31_32 | OCR_VDD_32_33)

// SD/MMC transfer rate unit list (speed / 10000)
static const uint32_t g_TransferRateUnits[7] = { 10, 100, 1000, 10000, 0, 0, 0 };
    
// SD transfer multiplier list (multiplier * 10)
static const uint32_t g_TransferRateMultipliers_sd[16] = { 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
    
// MMC transfer multiplier list (multiplier * 10)
static const uint32_t g_TransferRateMultipliers_mmc[16] = { 0, 10, 12, 13, 15, 20, 26, 30, 35, 40, 45, 52, 55, 60, 70, 80 };

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static String GetStatusFlagNames(uint32_t flags)
{
    String result;
    
#define ADD_FLAG(name) if (flags & HSMCI_SR_##name##_Msk) { if (!result.empty()) result += ", "; result += #name; }
    
    ADD_FLAG(CMDRDY);             /**< (HSMCI_SR) Command Ready (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RXRDY);              /**< (HSMCI_SR) Receiver Ready (cleared by reading HSMCI_RDR) Mask */
    ADD_FLAG(TXRDY)               /**< (HSMCI_SR) Transmit Ready (cleared by writing in HSMCI_TDR) Mask */
    ADD_FLAG(BLKE)                /**< (HSMCI_SR) Data Block Ended (cleared on read) Mask */
    ADD_FLAG(DTIP)                /**< (HSMCI_SR) Data Transfer in Progress (cleared at the end of CRC16 calculation) Mask */
    ADD_FLAG(NOTBUSY)             /**< (HSMCI_SR) HSMCI Not Busy Mask */
    ADD_FLAG(SDIOIRQA)            /**< (HSMCI_SR) SDIO Interrupt for Slot A (cleared on read) Mask */
    ADD_FLAG(SDIOWAIT)            /**< (HSMCI_SR) SDIO Read Wait Operation Status Mask */
    ADD_FLAG(CSRCV)               /**< (HSMCI_SR) CE-ATA Completion Signal Received (cleared on read) Mask */
    ADD_FLAG(RINDE)               /**< (HSMCI_SR) Response Index Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RDIRE)               /**< (HSMCI_SR) Response Direction Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RCRCE)               /**< (HSMCI_SR) Response CRC Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RENDE)               /**< (HSMCI_SR) Response End Bit Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RTOE)                /**< (HSMCI_SR) Response Time-out Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(DCRCE)               /**< (HSMCI_SR) Data CRC Error (cleared on read) Mask */
    ADD_FLAG(DTOE)                /**< (HSMCI_SR) Data Time-out Error (cleared on read) Mask */
    ADD_FLAG(CSTOE)               /**< (HSMCI_SR) Completion Signal Time-out Error (cleared on read) Mask */
    ADD_FLAG(BLKOVRE)             /**< (HSMCI_SR) DMA Block Overrun Error (cleared on read) Mask */
    ADD_FLAG(FIFOEMPTY)           /**< (HSMCI_SR) FIFO empty flag Mask */
    ADD_FLAG(XFRDONE)             /**< (HSMCI_SR) Transfer Done flag Mask */
    ADD_FLAG(ACKRCV)              /**< (HSMCI_SR) Boot Operation Acknowledge Received (cleared on read) Mask */
    ADD_FLAG(ACKRCVE)             /**< (HSMCI_SR) Boot Operation Acknowledge Error (cleared on read) Mask */
    ADD_FLAG(OVRE)                /**< (HSMCI_SR) Overrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) Mask */
    ADD_FLAG(UNRE)                /**< (HSMCI_SR) Underrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) Mask */

#undef ADD_FLAG

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

HSMCIDriver::HSMCIDriver(const DigitalPin& pinCD) : Thread("hsmci_driver"), m_Mutex("hsmci_driver_mutex", false), m_CardStateCondition("hsmci_driver_cstate"), m_IOCondition("hsmci_driver_io"), m_DeviceSemaphore("hsmci_driver_dev_sema", 1), m_PinCD(pinCD)
{    
    m_CardType = 0;
    m_PinCD.SetPullMode(PinPullMode_e::Up);

    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN28_bm, DigitalPinPeripheralID::C);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN25_bm, DigitalPinPeripheralID::D);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN30_bm, DigitalPinPeripheralID::C);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN31_bm, DigitalPinPeripheralID::C);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN26_bm, DigitalPinPeripheralID::C);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN27_bm, DigitalPinPeripheralID::C);

    DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN28_bm);
    DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN25_bm);
    DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN30_bm);
    DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN31_bm);
    DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN26_bm);
    DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN27_bm);
    
    m_CardState = CardState::Initializing;

    SAME70System::EnablePeripheralClock(ID_HSMCI);
    // Enable clock for DMA controller
    SAME70System::EnablePeripheralClock(ID_XDMAC);
    
    HSMCI->HSMCI_DTOR  = HSMCI_DTOR_DTOMUL_1048576 | HSMCI_DTOR_DTOCYC(2);     // Set the Data Timeout Register to 2 Mega Cycles
    HSMCI->HSMCI_CSTOR = HSMCI_CSTOR_CSTOMUL_1048576 | HSMCI_CSTOR_CSTOCYC(2); // Set Completion Signal Timeout to 2 Mega Cycles
    HSMCI->HSMCI_CFG   = HSMCI_CFG_FIFOMODE | HSMCI_CFG_FERRCTRL;              // Set Configuration Register
    HSMCI->HSMCI_MR    = HSMCI_MR_PWSDIV_Msk;                                  // Set power saving to maximum value
    HSMCI->HSMCI_CR    = HSMCI_CR_MCIEN_Msk | HSMCI_CR_PWSEN_Msk;              // Enable the HSMCI and the Power Saving

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::Setup(const String& devicePath)
{
    kernel::Kernel::RegisterIRQHandler(HSMCI_IRQn, IRQCallback, this);
    
    Start(true);

    m_DevicePathBase = devicePath;

    m_RawINode = ptr_new<HSMCIINode>(this);
    m_RawINode->bi_nNodeHandle = Kernel::RegisterDevice((devicePath + "raw").c_str(), m_RawINode);
    return true;    
}

///////////////////////////////////////////////////////////////////////////////
/// Maintenance thread entry point.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int HSMCIDriver::Run()
{
	snooze_ms(250);
    
    for(;;)
    {
        bool hasCard = !m_PinCD;
        
        if (hasCard != m_CardInserted) {
			snooze_ms(100); // De-bounce
            hasCard = !m_PinCD;
        }            
        if (hasCard != m_CardInserted)
        {
            m_CardInserted = hasCard;
            CRITICAL_SCOPE(m_Mutex);
            if (m_CardInserted)
            {
                m_CardState = CardState::Initializing;
                // Set 1-bit bus width and low clock for initialization
                m_Clock = SDMMC_CLOCK_INIT;
                m_BusWidth = 1;
                m_HighSpeed = false;

                ApplySpeedAndBusWidth();
            
                // Initialization of the card requested
                if (InitializeCard())
                {
                    kprintf("SD/MMC card ready: %" PRIu64 "\n", m_SectorCount * BLOCK_SIZE);
                    SetState(CardState::Ready);
                    DecodePartitions(true);
                }
                else
                {
                    kprintf("SD/MMC card initialization failed\n");
                    SetState(CardState::Unusable);
                }            
            }
            else
            {
                SetState(CardState::NoCard);
                DecodePartitions(true);
            }
        }
		snooze_ms(100);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> HSMCIDriver::OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int flags)
{
    CRITICAL_SCOPE(m_Mutex);

    if (!IsReady()) {
        set_last_error(ENODEV);
        return nullptr;
    }
    return KFilesystemFileOps::OpenFile(volume, node, flags);
//    return ptr_new<KFileHandle>();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int HSMCIDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_Mutex);

    if (!IsReady()) {
        set_last_error(ENODEV);
        return -1;
    }

    switch(request)
    {
        case DEVCTL_GET_DEVICE_GEOMETRY:
        {
            if (outData == nullptr || outDataLength < sizeof(device_geometry)) {
                set_last_error(EINVAL);
                return -1;
            }
            device_geometry* geometry = static_cast<device_geometry*>(outData);
            geometry->bytes_per_sector  = BLOCK_SIZE;
            geometry->sector_count = m_SectorCount;
            geometry->read_only    = false;
            geometry->removable   = true;
            return 0;
        }
        case DEVCTL_REREAD_PARTITION_TABLE:
            return DecodePartitions(false);
            
        case SDCDEVCTL_SDIO_READ_DIRECT:
        {
            if (inData == nullptr || outData == nullptr || inDataLength < sizeof(SDCDEVCTL_SDIOReadDirectArgs) || outDataLength != sizeof(uint8_t)) {
                set_last_error(EINVAL);
                return -1;
            }
            const SDCDEVCTL_SDIOReadDirectArgs* args = static_cast<const SDCDEVCTL_SDIOReadDirectArgs*>(inData);
            return SDIOReadDirect(args->FunctionNumber, args->Address, static_cast<uint8_t*>(outData));
        }
        case SDCDEVCTL_SDIO_WRITE_DIRECT:
        {
            if (inData == nullptr || inDataLength < sizeof(SDCDEVCTL_SDIOWriteDirectArgs)) {
                set_last_error(EINVAL);
                return -1;
            }
            const SDCDEVCTL_SDIOWriteDirectArgs* args = static_cast<const SDCDEVCTL_SDIOWriteDirectArgs*>(inData);
            return SDIOWriteDirect(args->FunctionNumber, args->Address, args->Data);
        }
        case SDCDEVCTL_SDIO_READ_EXTENDED:
        {
            if (inData == nullptr || outData == nullptr || inDataLength < sizeof(SDCDEVCTL_SDIOReadExtendedArgs)) {
                set_last_error(EINVAL);
                return -1;
            }
            const SDCDEVCTL_SDIOReadExtendedArgs* args = static_cast<const SDCDEVCTL_SDIOReadExtendedArgs*>(inData);
            return SDIOReadExtended(args->FunctionNumber, args->Address, args->IncrementAddr, outData, outDataLength);
        }
        case SDCDEVCTL_SDIO_WRITE_EXTENDED:        
        {
            if (inData == nullptr || inDataLength < sizeof(SDCDEVCTL_SDIOWriteExtendedArgs)) {
                set_last_error(EINVAL);
                return -1;
            }
            const SDCDEVCTL_SDIOWriteExtendedArgs* args = static_cast<const SDCDEVCTL_SDIOWriteExtendedArgs*>(inData);
            return SDIOWriteExtended(args->FunctionNumber, args->Address, args->IncrementAddr, args->Buffer, args->Size);
        }
    }
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t HSMCIDriver::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
    Ptr<HSMCIINode> inode = (file != nullptr) ? ptr_static_cast<HSMCIINode>(file->GetINode()) : nullptr;
    
    bool needLocking = false;
    if (inode != nullptr)
    {
        needLocking = true;
        if (position + length > inode->bi_nSize)
        {
            if (position >= inode->bi_nSize) {
                return 0;
            } else {
                length = inode->bi_nSize - position;
            }
        }
        position += inode->bi_nStart;
    }
    
    if ((position % BLOCK_SIZE) != 0 || (length % BLOCK_SIZE) != 0)
    {
        set_last_error(EINVAL);
        return -1;
    }
    
    uint32_t firstBlock = position / BLOCK_SIZE;
    uint32_t blockCount = length / BLOCK_SIZE;
    
    int error;
    for (int retry = 0; retry < 10; ++retry)
    {
        CRITICAL_SCOPE(m_DeviceSemaphore);
        
        error = 0;
        CRITICAL_SCOPE(m_Mutex, needLocking);

        if (!IsReady()) {
            set_last_error(ENODEV);
            return -1;
        }
    
        if (!Cmd13_sdmmc()) {
            error = EIO;
            continue;
        }

        uint32_t cmd = (blockCount > 1) ? SDMMC_CMD18_READ_MULTIPLE_BLOCK : SDMMC_CMD17_READ_SINGLE_BLOCK;

        // SDSC Card (CCS=0) uses byte unit address,
        // SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit).
        uint32_t start = firstBlock;
        if ((m_CardType & HSMCICardType::HC) == 0) {
            start *= BLOCK_SIZE;
        }

        bool useDMA = (intptr_t(buffer) & DCACHE_LINE_SIZE_MASK) == 0;
        
        if (!StartAddressedDataTransCmd(cmd, start, BLOCK_SIZE, blockCount, useDMA)) {
            error = EIO;
            continue;
        }
        uint32_t response = GetResponse();
        if (response & CARD_STATUS_ERR_RD_WR)
        {
            kprintf("ERROR: HSMCIDriver::Read() Read %02d response 0x%08lx CARD_STATUS_ERR_RD_WR\n", int(SDMMC_CMD_GET_INDEX(cmd)), response);
            error = EIO;
            continue;
        }

        bool result;
        if (useDMA && retry < 50) {
//            kprintf("ReadDMA %ld at %Ld\n", length, position);
            result = ReadDMA(buffer, BLOCK_SIZE, blockCount);
        } else {
//            kprintf("ReadNoDMA %ld at %Ld\n", length, position);
            result = ReadNoDMA(buffer, BLOCK_SIZE, blockCount);
        }
        if (!result) {
            error = EIO;
            continue;
        }
        // WORKAROUND for no compliance card: The errors on this command must be ignored and one retry can be necessary in SPI mode for no compliance card.
        if (blockCount > 1 && !StopAddressedDataTransCmd(SDMMC_CMD12_STOP_TRANSMISSION, 0)) {
            StopAddressedDataTransCmd(SDMMC_CMD12_STOP_TRANSMISSION, 0);
        }
        break;
    }
    if (error == 0) {
        return length;
    } else {        
        set_last_error(error);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t HSMCIDriver::Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length)
{
    Ptr<HSMCIINode> inode = (file != nullptr) ? ptr_static_cast<HSMCIINode>(file->GetINode()) : nullptr;
    
    bool needLocking = false;
    if (inode != nullptr)
    {
        needLocking = true;
        if (position + length > inode->bi_nSize)
        {
            if (position >= inode->bi_nSize) {
                return 0;
            } else {
                length = inode->bi_nSize - position;
            }
        }
        position += inode->bi_nStart;
    }

    if ((position % BLOCK_SIZE) != 0 || (length % BLOCK_SIZE) != 0) {
        set_last_error(EINVAL);
        return -1;
    }

    uint32_t firstBlock = position / BLOCK_SIZE;
    uint32_t blockCount = length / BLOCK_SIZE;

    int error;
    for (int retry = 0; retry < 10; ++retry)
    {
        CRITICAL_SCOPE(m_DeviceSemaphore);
        
        error = 0;
        CRITICAL_SCOPE(m_Mutex, needLocking);

        if (!IsReady()) {
            set_last_error(ENODEV);
            return -1;
        }

        uint32_t cmd = (blockCount > 1) ? SDMMC_CMD25_WRITE_MULTIPLE_BLOCK : SDMMC_CMD24_WRITE_BLOCK;

        // SDSC Card (CCS=0) uses byte unit address,
        // SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit).
        uint32_t start = firstBlock;
        if ((m_CardType & HSMCICardType::HC) == 0) {
            start *= BLOCK_SIZE;
        }

        if (!StartAddressedDataTransCmd(cmd, start, BLOCK_SIZE, blockCount, true)) {
            error = EIO;
            continue;
        }
        uint32_t response = GetResponse();
        if (response & CARD_STATUS_ERR_RD_WR)
        {
            kprintf("ERROR: HSMCIDriver::Write() Write %02d response 0x%08lx CARD_STATUS_ERR_RD_WR\n", int(SDMMC_CMD_GET_INDEX(cmd)), response);
            error = EIO;
            continue;
        }
        if (!WriteDMA(buffer, BLOCK_SIZE, blockCount)) {
            error = EIO;
            continue;
        }
        // SPI multi-block writes terminate using a special token, not a STOP_TRANSMISSION request.
        if (blockCount > 1 && !StopAddressedDataTransCmd(SDMMC_CMD12_STOP_TRANSMISSION, 0)) {
            error = EIO;
            continue;
        }
        break;
    }
    if (error == 0) {
        return length;
    } else {        
        set_last_error(error);
        return -1;
    }
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

bool HSMCIDriver::InitializeCard()
{
    // In first, try to install SD/SDIO card
    m_CardType    = HSMCICardType::SD;
    m_CardVersion = HSMCICardVersion::Unknown;
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

    if (m_CardType & HSMCICardType::SD)
    {
        // Try to get the SD card's operating condition
        if (!OperationalConditionMCI_sd(v2))
        {
            // It is not a SD card
            kprintf("Start MMC Install\n");
            m_CardType = HSMCICardType::MMC;
            return InitializeMMCCard();
        }
    }

    if (m_CardType & HSMCICardType::SD)
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
    m_RCA = GetResponse() >> 16;

    // SD MEMORY, Get the Card-Specific Data
    if (m_CardType & HSMCICardType::SD)
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
    if (m_CardType & HSMCICardType::SD) {
        if (!ACmd51_sd()) {
            return false;
        }
    }
    if (m_CardType & HSMCICardType::SDIO) {
        if (!GetMaxSpeed_sdio()) {
            return false;
        }
    }
    // TRY to enable 4-bit mode
    if (m_CardType & HSMCICardType::SDIO) {
        if (!SetBusWidth_sdio()) {
            return false;
        }
    }
    if (m_CardType & HSMCICardType::SD) {
        if (!ACmd6_sd()) {
            return false;
        }
    }
    ApplySpeedAndBusWidth();
    // Try to enable high-speed mode
    if (m_CardType & HSMCICardType::SDIO) {
        if (!SetHighSpeed_sdio()) {
            return false;
        }
    }
    if (m_CardType & HSMCICardType::SD)
    {
        if (m_CardVersion > HSMCICardVersion::SD_1_0) {
            if (!SetHighSpeed_sd()) {
                return false;
            }
        }
    }
    ApplySpeedAndBusWidth();
    // SD MEMORY, Set default block size
    if (m_CardType & HSMCICardType::SD)
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

size_t HSMCIDriver::ReadPartitionData(void* userData, off64_t position, void* buffer, size_t size)
{
    return static_cast<HSMCIDriver*>(userData)->Read(nullptr, position, buffer, size );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int HSMCIDriver::DecodePartitions(bool force)
{
    try
    {
        device_geometry diskGeom;
        std::vector<disk_partition_desc> partitions;

        m_RawINode->bi_nSize = m_SectorCount * BLOCK_SIZE;

        memset(&diskGeom, 0, sizeof(diskGeom));
        diskGeom.sector_count     = m_SectorCount;
        diskGeom.bytes_per_sector = BLOCK_SIZE;
        diskGeom.read_only 	  = false;
        diskGeom.removable 	  = true;

        kprintf("HSMCIDriver::DecodePartitions(): Decoding partition table\n");

        if (KVFSManager::DecodeDiskPartitions(diskGeom, &partitions, &ReadPartitionData, this) < 0) {
	    kprintf( "   Invalid partition table\n" );
	    return -1;
        }
        for (size_t i = 0 ; i < partitions.size() ; ++i)
        {
	    if ( partitions[i].p_type != 0 && partitions[i].p_size != 0 )
            {
	        kprintf( "   Partition %" PRIu32 " : %10" PRIu64 " -> %10" PRIu64 " %02x (%" PRIu64 ")\n", uint32_t(i), partitions[i].p_start,
		         partitions[i].p_start + partitions[i].p_size - 1LL, partitions[i].p_type,
		         partitions[i].p_size);
	    }
        }

        for (Ptr<HSMCIINode> partition : m_PartitionINodes)
        {
	    bool found = false;
	    for (size_t i = 0 ; i < partitions.size() ; ++i)
            {
	        if ( partitions[i].p_start == partition->bi_nStart && partitions[i].p_size == partition->bi_nSize ) {
		    found = true;
		    break;
	        }
	    }
	    if (!force && !found && partition->bi_nOpenCount > 0) {
	        kprintf("ERROR: HSMCIDriver::DecodePartitions() Open partition has changed.\n");
	        set_last_error(EBUSY);
	        return -1;
	    }
        }
    
        std::vector<Ptr<HSMCIINode>> unusedPartitionINodes; // = std::move(m_PartitionINodes);
          // Remove deleted partitions from /dev/
        for (auto i = m_PartitionINodes.begin(); i != m_PartitionINodes.end(); )
        {
            Ptr<HSMCIINode> partition = *i;
	    bool found = false;
	    for (size_t i = 0 ; i < partitions.size() ; ++i)
            {
	        if (partitions[i].p_start == partition->bi_nStart && partitions[i].p_size == partition->bi_nSize )
                {
		    partitions[i].p_size = 0;
		    partition->bi_nPartitionType = partitions[i].p_type;
		    found = true;
		    break;
	        }
	    }
	    if (!found)
            {
	        Kernel::RemoveDevice(partition->bi_nNodeHandle);
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
	    if ( partitions[i].p_type == 0 || partitions[i].p_size == 0 ) {
	        continue;
	    }
            Ptr<HSMCIINode> partition;
            if (!unusedPartitionINodes.empty()) {
                partition = unusedPartitionINodes.back();
                unusedPartitionINodes.pop_back();
            } else {
                partition = ptr_new<HSMCIINode>(this);
            }
            m_PartitionINodes.push_back(partition);
	    partition->bi_nStart = partitions[i].p_start;
	    partition->bi_nSize  = partitions[i].p_size;
        }
        
        std::sort(m_PartitionINodes.begin(), m_PartitionINodes.end(), [](Ptr<HSMCIINode> lhs, Ptr<HSMCIINode> rhs) { return lhs->bi_nStart < rhs->bi_nStart; });

          // We now have to rename nodes that might have moved around in the table and
          // got new names. To avoid name-clashes while renaming we first give all
          // nodes a unique temporary name before looping over again giving them their
          // final names

        for (size_t i = 0; i < m_PartitionINodes.size(); ++i)    
        {
            Ptr<HSMCIINode> partition = m_PartitionINodes[i];
            if (partition->bi_nNodeHandle != -1)
            {
                String path = m_DevicePathBase + String::format_string("%lu_new", i);
	        Kernel::RenameDevice(partition->bi_nNodeHandle, path.c_str());
            }            
        }
        for (size_t i = 0; i < m_PartitionINodes.size(); ++i)
        {
            String path = m_DevicePathBase + String::format_string("%lu", i);
        
            Ptr<HSMCIINode> partition = m_PartitionINodes[i];
            if (partition->bi_nNodeHandle != -1) {
                Kernel::RenameDevice(partition->bi_nNodeHandle, path.c_str());
            } else {
                Kernel::RegisterDevice(path.c_str(), partition);
            }
        }
        return 0;
    }
    catch (const std::bad_alloc&) {
	kprintf( "Error: bdd_decode_partitions() no memory for partition inode\n" );
        set_last_error(ENOMEM);
        return -1;            
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

bool HSMCIDriver::InitializeMMCCard()
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
    if (m_CardVersion >= HSMCICardVersion::MMC_4)
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

void HSMCIDriver::SetState(CardState state)
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

IRQResult HSMCIDriver::HandleIRQ()
{
    for (;;)
    {
        uint32_t status = HSMCI->HSMCI_SR & HSMCI->HSMCI_IMR;
        
        static const uint32_t eventFlags = HSMCI_SR_XFRDONE_Msk | HSMCI_SR_CMDRDY_Msk | HSMCI_SR_NOTBUSY_Msk;
        static const uint32_t errorFlags = ~(eventFlags | HSMCI_SR_RXRDY_Msk);
        
        if (status & errorFlags)
        {
            HSMCI->HSMCI_IDR = ~0;
            m_IOError = EIO;
            kprintf("ERROR: HSMCIDriver::HandleIRQ() error: %s\n", GetStatusFlagNames(status).c_str());
            m_IOCondition.Wakeup(0);
            return IRQResult::HANDLED;
        }
        if (status & HSMCI_SR_RXRDY_Msk)
        {
            if (m_BytesToRead == 0)
            {
                HSMCI->HSMCI_IDR = ~0;
                m_IOError = EIO;
                kprintf("ERROR: HSMCIDriver::HandleIRQ() to many bytes received: %s\n", GetStatusFlagNames(status).c_str());
                m_IOCondition.Wakeup(0);
                return IRQResult::HANDLED;
            }
            if (HSMCI->HSMCI_MR & HSMCI_MR_FBYTE_Msk)
            {
                *reinterpret_cast<uint8_t*>(m_CurrentBuffer) = HSMCI->HSMCI_RDR;
                m_CurrentBuffer = reinterpret_cast<uint8_t*>(m_CurrentBuffer) + 1;
                m_BytesToRead--;

                if ((intptr_t(m_CurrentBuffer) & 3) == 0 && m_BytesToRead >= 4) {
                    HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE_Msk;
                }
            }
            else
            {
                *reinterpret_cast<uint32_t*>(m_CurrentBuffer) = HSMCI->HSMCI_RDR;
                m_CurrentBuffer = reinterpret_cast<uint32_t*>(m_CurrentBuffer) + 1;
                m_BytesToRead -= 4;
                
                if (m_BytesToRead < 4 && m_BytesToRead != 0) {
                    HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE_Msk;
                }
            }
            continue;
        }
        if (status & eventFlags) {
            HSMCI->HSMCI_IDR = ~0;
            m_IOError = 0;
            m_IOCondition.Wakeup(0);
        }
        break;
    }
	return IRQResult::HANDLED
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::WaitIRQ(uint32_t flags)
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        HSMCI->HSMCI_IER = flags;
        while(!m_IOCondition.IRQWait())
        {
            if (get_last_error() != EINTR) {
                HSMCI->HSMCI_IDR = ~0;
                m_IOError = get_last_error();
                break;
            }
        }
    } CRITICAL_END;
    if (m_IOError != 0) {
        Reset();
        set_last_error(m_IOError);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::WaitIRQ(uint32_t flags, bigtime_t timeout)
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        HSMCI->HSMCI_IER = flags;
        while(!m_IOCondition.IRQWaitTimeout(timeout))
        {
            if (get_last_error() != EINTR) {
                HSMCI->HSMCI_IDR = ~0;
                m_IOError = get_last_error();
                break;
            }
        }
    } CRITICAL_END;
    if (m_IOError != 0) {
        Reset();
        set_last_error(m_IOError);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Reset the HSMCI interface
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HSMCIDriver::Reset()
{
    uint32_t mr    = HSMCI->HSMCI_MR;
    uint32_t dtor  = HSMCI->HSMCI_DTOR;
    uint32_t sdcr  = HSMCI->HSMCI_SDCR;
    uint32_t cstor = HSMCI->HSMCI_CSTOR;
    uint32_t cfg   = HSMCI->HSMCI_CFG;
    
    HSMCI->HSMCI_CR    = HSMCI_CR_SWRST;
    HSMCI->HSMCI_MR    = mr;
    HSMCI->HSMCI_DTOR  = dtor;
    HSMCI->HSMCI_SDCR  = sdcr;
    HSMCI->HSMCI_CSTOR = cstor;
    HSMCI->HSMCI_CFG   = cfg;
    HSMCI->HSMCI_DMA   = 0;
    HSMCI->HSMCI_CR    = HSMCI_CR_PWSEN | HSMCI_CR_MCIEN;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::ReadNoDMA(void* buffer, uint16_t blockSize, size_t blockCount)
{
    m_CurrentBuffer = buffer;
    m_BytesToRead   = blockSize * blockCount;
    m_IOError       = 0;
    
    if ((intptr_t(buffer) & 3) || (m_BytesToRead < 4)) {
        HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE;        
    } else {
        HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE;
    }

    return WaitIRQ(HSMCI_IER_RXRDY_Msk | HSMCI_IER_XFRDONE_Msk | HSMCI_IER_UNRE_Msk | HSMCI_IER_OVRE_Msk | HSMCI_IER_DTOE_Msk | HSMCI_IER_DCRCE_Msk);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::ReadDMA(void* buffer, uint16_t blockSize, size_t blockCount)
{
    if (blockCount == 0) {
        return true;
    }
    if (buffer == nullptr) {
        set_last_error(EINVAL);
        return false;
    }

    uint32_t bytesToRead = blockSize * blockCount;
    
    if ((intptr_t(buffer) & DCACHE_LINE_SIZE_MASK) || (bytesToRead & DCACHE_LINE_SIZE_MASK)) {
        kprintf("ERROR: HSMCIDriver::ReadDMA() called with unaligned buffer or size\n");
        set_last_error(EINVAL);
        return false;
    }
    
    HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE;        
    xdmac_channel_config_t dmaCfg = {0, 0, 0, 0, 0, 0, 0, 0};
        
    dmaCfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
                    | XDMAC_CC_MBSIZE_SINGLE
                    | XDMAC_CC_DSYNC_PER2MEM
                    | XDMAC_CC_CSIZE_CHK_1
                    | XDMAC_CC_SIF_AHB_IF1
                    | XDMAC_CC_DIF_AHB_IF0
                    | XDMAC_CC_SAM_FIXED_AM
                    | XDMAC_CC_DAM_INCREMENTED_AM
                    | XDMAC_CC_DWIDTH_WORD
                    | XDMAC_CC_PERID(CONF_HSMCI_XDMAC_CHANNEL);

    dmaCfg.mbr_ubc = bytesToRead / 4;
    dmaCfg.mbr_sa  = intptr_t(&HSMCI->HSMCI_FIFO[0]);
    dmaCfg.mbr_da  = intptr_t(buffer);
    
    xdmac_configure_transfer(XDMAC, CONF_HSMCI_XDMAC_CHANNEL, &dmaCfg);
    xdmac_channel_enable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
    bool result = WaitIRQ(HSMCI_IER_XFRDONE_Msk | HSMCI_IER_UNRE_Msk | HSMCI_IER_OVRE_Msk | HSMCI_IER_DTOE_Msk | HSMCI_IER_DCRCE_Msk);
    xdmac_channel_disable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::WriteDMA(const void* buffer, uint16_t blockSize, size_t blockCount)
{
    if (blockCount == 0) {
        return true;
    }
    if (buffer == nullptr) {
        set_last_error(EINVAL);
        return false;
    }

    uint32_t bytesToWRite = blockSize * blockCount;

    xdmac_channel_config_t dmaCfg = {0, 0, 0, 0, 0, 0, 0, 0};

    dmaCfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
                   | XDMAC_CC_MBSIZE_SINGLE
                   | XDMAC_CC_DSYNC_MEM2PER
                   | XDMAC_CC_CSIZE_CHK_1
                   | XDMAC_CC_SIF_AHB_IF0
                   | XDMAC_CC_DIF_AHB_IF1
                   | XDMAC_CC_SAM_INCREMENTED_AM
                   | XDMAC_CC_DAM_FIXED_AM
                   | XDMAC_CC_PERID(CONF_HSMCI_XDMAC_CHANNEL);

    if(intptr_t(buffer) & 3)
    {
        dmaCfg.mbr_cfg |= XDMAC_CC_DWIDTH_BYTE;
        dmaCfg.mbr_ubc = bytesToWRite;
        HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE;
    }
    else
    {
        dmaCfg.mbr_cfg |= XDMAC_CC_DWIDTH_WORD;
        dmaCfg.mbr_ubc = bytesToWRite / 4;
        HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE;
    }
    
    dmaCfg.mbr_sa = intptr_t(buffer);
    dmaCfg.mbr_da = intptr_t(&HSMCI->HSMCI_FIFO[0]);

    xdmac_configure_transfer(XDMAC, CONF_HSMCI_XDMAC_CHANNEL, &dmaCfg);
    xdmac_channel_enable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
    bool result = WaitIRQ(HSMCI_IER_XFRDONE_Msk | HSMCI_IER_UNRE_Msk | HSMCI_IER_OVRE_Msk | HSMCI_IER_DTOE_Msk | HSMCI_IER_DCRCE_Msk);
    xdmac_channel_disable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t HSMCIDriver::SDIOReadDirect(uint8_t functionNumber, uint32_t addr, uint8_t *dest)
{
    if (dest == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }
    if (!Cmd52_sdio(SDIO_CMD52_READ_FLAG, functionNumber, addr, 0, dest)) {
        set_last_error(EIO);
        return -1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t HSMCIDriver::SDIOWriteDirect(uint8_t functionNumber, uint32_t addr, uint8_t data)
{
    if (!Cmd52_sdio(SDIO_CMD52_WRITE_FLAG, functionNumber, addr, 0, &data)) {
        set_last_error(EIO);
        return -1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t HSMCIDriver::SDIOReadExtended(uint8_t functionNumber, uint32_t addr, uint8_t incrementAddr, void* buffer, size_t size)
{
    if ((size == 0) || (size > BLOCK_SIZE)) {
        set_last_error(EINVAL);
        return -1;
    }
    bool useDMA = (intptr_t(buffer) & DCACHE_LINE_SIZE_MASK) == 0 && (size & DCACHE_LINE_SIZE_MASK) == 0;
    if (!Cmd53_sdio(SDIO_CMD53_READ_FLAG, functionNumber, addr, incrementAddr, size, useDMA)) {
        set_last_error(EIO);
        return -1;
    }
    bool result;
    if (useDMA) {
        result = ReadDMA(buffer, size, 1);
    } else {
        result = ReadNoDMA(buffer, size, 1);
    }        
    return (result) ? size : -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t HSMCIDriver::SDIOWriteExtended(uint8_t functionNumber, uint32_t addr, uint8_t incrementAddr, const void* buffer, size_t size)
{
    if ((size == 0) || (size > BLOCK_SIZE)) {
        set_last_error(EINVAL);
        return -1;
    }
    if (!Cmd53_sdio(SDIO_CMD53_WRITE_FLAG, functionNumber, addr, incrementAddr, size, true)) {
        set_last_error(EIO);
        return -1;
    }
    if (!WriteDMA(buffer, size, 1)) {
        return -1;
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Set HSMCI clock frequency.
///
/// \param frequency    HSMCI clock frequency in Hz.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HSMCIDriver::SetClockFrequency(uint32_t frequency)
{
    uint32_t peripheralClock = SAME70System::GetFrequencyPeripheral();
    uint32_t divider = 0;

    // frequency = peripheralClock / (divider + 2)
    if (frequency * 2 < peripheralClock) {
        divider = (peripheralClock + frequency - 1) / frequency - 2;
    }
    
    uint32_t clockCfg = HSMCI_MR_CLKDIV(divider >> 1);
    if (divider & 1) clockCfg |= HSMCI_MR_CLKODD;
    
    HSMCI->HSMCI_MR = (HSMCI->HSMCI_MR & ~(HSMCI_MR_CLKDIV_Msk | HSMCI_MR_CLKODD)) | clockCfg;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HSMCIDriver::SendClock()
{
    // Configure command
    HSMCI->HSMCI_MR &= ~(HSMCI_MR_WRPROOF_Msk | HSMCI_MR_RDPROOF_Msk | HSMCI_MR_FBYTE_Msk);
    // Write argument
    HSMCI->HSMCI_ARGR = 0;
    // Write and start initialization command
    HSMCI->HSMCI_CMDR = HSMCI_CMDR_RSPTYP_NORESP | HSMCI_CMDR_SPCMD_INIT | HSMCI_CMDR_OPDCMD_OPENDRAIN;
    // Wait end of initialization command
    WaitIRQ(HSMCI_SR_CMDRDY_Msk);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Send a command
///
/// \param cmdr       CMDR register bit to use for this command
/// \param cmd        Command definition
/// \param arg        Argument of the command
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::ExecuteCmd(uint32_t cmdr, uint32_t cmd, uint32_t arg)
{
    cmdr |= HSMCI_CMDR_CMDNB(cmd) | HSMCI_CMDR_SPCMD_STD;
    if (cmd & SDMMC_RESP_PRESENT)
    {
        cmdr |= HSMCI_CMDR_MAXLAT;
        if (cmd & SDMMC_RESP_136) {
            cmdr |= HSMCI_CMDR_RSPTYP_136_BIT;
        } else if (cmd & SDMMC_RESP_BUSY) {
            cmdr |= HSMCI_CMDR_RSPTYP_R1B;
        } else {
            cmdr |= HSMCI_CMDR_RSPTYP_48_BIT;
        }
    }
    if (cmd & SDMMC_CMD_OPENDRAIN) {
        cmdr |= HSMCI_CMDR_OPDCMD_OPENDRAIN;
    }

    HSMCI->HSMCI_ARGR = arg;  // Write argument
    HSMCI->HSMCI_CMDR = cmdr; // Write and start command

    uint32_t errorFlags = HSMCI_SR_CSTOE_Msk | HSMCI_SR_RTOE_Msk | HSMCI_SR_RENDE_Msk | HSMCI_SR_RDIRE_Msk | HSMCI_SR_RINDE_Msk;
    if (cmd & SDMMC_RESP_CRC) {
        errorFlags |= HSMCI_SR_RCRCE_Msk;
    }
    // Wait end of command
    if (!WaitIRQ(HSMCI_SR_CMDRDY_Msk | errorFlags)) {
        return false;
    }
    if (cmd & SDMMC_RESP_BUSY)
    {
        // Should we have checked HSMCI_SR_DTIP_Msk to? Atmel do, but don't tell why.
        if (!WaitIRQ(HSMCI_SR_NOTBUSY_Msk, bigtime_from_s(1))) {
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::SendCmd(uint32_t cmd, uint32_t arg)
{
    // Configure command
    HSMCI->HSMCI_MR &= ~(HSMCI_MR_WRPROOF_Msk | HSMCI_MR_RDPROOF_Msk | HSMCI_MR_FBYTE_Msk);
    // Disable DMA for HSMCI
    HSMCI->HSMCI_DMA = 0;
    HSMCI->HSMCI_BLKR = 0;
    return ExecuteCmd(0, cmd, arg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HSMCIDriver::GetResponse128(uint8_t* response)
{
    for (int i = 0; i < 4; ++i)
    {
        uint32_t response32 = GetResponse();
        *response++ = (response32 >> 24) & 0xff;
        *response++ = (response32 >> 16) & 0xff;
        *response++ = (response32 >>  8) & 0xff;
        *response++ = (response32 >>  0) & 0xff;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::StartAddressedDataTransCmd(uint32_t cmd, uint32_t arg, uint16_t block_size, uint16_t blockCount, bool setupDMA)
{
    uint32_t cmdr;

    // Enable DMA for HSMCI
    HSMCI->HSMCI_DMA = (setupDMA) ? HSMCI_DMA_DMAEN : 0;

    // Enabling Read/Write Proof allows to stop the HSMCI Clock during
    // read/write  access if the internal FIFO is full.
    // This will guarantee data integrity, not bandwidth.
    HSMCI->HSMCI_MR |= HSMCI_MR_WRPROOF_Msk | HSMCI_MR_RDPROOF_Msk;
    // Force byte transfer if needed
    if (block_size & 0x3) {
        HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE_Msk;
    } else {
        HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE_Msk;
    }

    if (cmd & SDMMC_CMD_WRITE) {
        cmdr = HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRDIR_WRITE;
    } else {
        cmdr = HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRDIR_READ;
    }

    if (cmd & SDMMC_CMD_SDIO_BYTE)
    {
        cmdr |= HSMCI_CMDR_TRTYP_BYTE;
        // Value 0 corresponds to a 512-byte transfer
        HSMCI->HSMCI_BLKR = ((block_size % BLOCK_SIZE) << HSMCI_BLKR_BCNT_Pos);
    }
    else
    {
        HSMCI->HSMCI_BLKR = (block_size << HSMCI_BLKR_BLKLEN_Pos) | (blockCount << HSMCI_BLKR_BCNT_Pos);
        if (cmd & SDMMC_CMD_SDIO_BLOCK) {
            cmdr |= HSMCI_CMDR_TRTYP_BLOCK;
        } else if (cmd & SDMMC_CMD_STREAM) {
            cmdr |= HSMCI_CMDR_TRTYP_STREAM;
        } else if (cmd & SDMMC_CMD_SINGLE_BLOCK) {
            cmdr |= HSMCI_CMDR_TRTYP_SINGLE;
        } else if (cmd & SDMMC_CMD_MULTI_BLOCK) {
            cmdr |= HSMCI_CMDR_TRTYP_MULTIPLE;
        } else {
            kprintf("ERROR: StartAddressedDataTransCmd() invalid command flags: %lx\n", cmd);
            return false;
        }
    }
    return ExecuteCmd(cmdr, cmd, arg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::StopAddressedDataTransCmd(uint32_t cmd, uint32_t arg)
{
    return ExecuteCmd(HSMCI_CMDR_TRCMD_STOP_DATA, cmd, arg);
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

bool HSMCIDriver::OperationalConditionMCI_sd(bool v2)
{
    bigtime_t deadline = get_system_time() + bigtime_from_s(1);
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
                m_CardType |= HSMCICardType::HC;
            }
            break;
        }
        if (get_system_time() > deadline) {
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

bool HSMCIDriver::OperationalConditionMCI_mmc()
{
    bigtime_t deadline = get_system_time() + bigtime_from_s(1);
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
                m_CardType |= HSMCICardType::HC;
            }
            break;
        }
        if (get_system_time() > deadline) {
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

bool HSMCIDriver::OperationalCondition_sdio()
{
    // CMD5 - SDIO send operation condition (OCR) command.
    if (!SendCmd(SDIO_CMD5_SEND_OP_COND, 0)) {
        kprintf("ERROR: HSMCIDriver::OperationalCondition_sdio:1() CMD5 Fail\n");
        return true; // No error but card type not updated
    }
    uint32_t response = GetResponse();
    if ((response & OCR_SDIO_NF) == 0) {
        return true; // No error but card type not updated
    }

    bigtime_t deadline = get_system_time() + bigtime_from_s(1);
    for (;;)
    {
        // CMD5 - SDIO send operation condition (OCR) command.
        if (!SendCmd(SDIO_CMD5_SEND_OP_COND, response & SD_MMC_VOLTAGE_SUPPORT)) {
            kprintf("ERROR: HSMCIDriver::OperationalCondition_sdio:2() CMD5 Fail\n");
            return false;
        }
        response = GetResponse();
        if ((response & OCR_POWER_UP_BUSY) == OCR_POWER_UP_BUSY) {
            break;
        }
        if (get_system_time() > deadline) {
            kprintf("ERROR: OperationalCondition_sdio(): Timeout (0x%08lx)\n", response);
            return false;
        }
    }
    if ((response & OCR_SDIO_MP) > 0) {
        m_CardType = HSMCICardType::SD_COMBO;
    } else {
        m_CardType = HSMCICardType::SDIO;
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

bool HSMCIDriver::GetMaxSpeed_sdio()
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

bool HSMCIDriver::SetBusWidth_sdio()
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

bool HSMCIDriver::SetHighSpeed_sdio()
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

bool HSMCIDriver::SetHighSpeed_sd()
{
    uint8_t switch_status[SD_SW_STATUS_BSIZE] = {0};

    if (!StartAddressedDataTransCmd(SD_CMD6_SWITCH_FUNC,
                                    SD_CMD6_MODE_SWITCH
                                  | SD_CMD6_GRP6_NO_INFLUENCE
                                  | SD_CMD6_GRP5_NO_INFLUENCE
                                  | SD_CMD6_GRP4_NO_INFLUENCE
                                  | SD_CMD6_GRP3_NO_INFLUENCE
                                  | SD_CMD6_GRP2_DEFAULT
                                  | SD_CMD6_GRP1_HIGH_SPEED,
                                    SD_SW_STATUS_BSIZE, 1, false)) {
        return false;
    }
    if (!ReadNoDMA(switch_status, SD_SW_STATUS_BSIZE, 1)) {
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

bool HSMCIDriver::SetBusWidth_mmc(int busWidth)
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

bool HSMCIDriver::SetHighSpeed_mmc()
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

void HSMCIDriver::DecodeCSD_mmc()
{
    uint32_t unit;
    uint32_t mul;
    uint32_t tran_speed;

    // Get MMC System Specification version supported by the card
    switch (MMC_CSD_SPEC_VERS(m_CSD))
    {
        default:
        case 0: m_CardVersion = HSMCICardVersion::MMC_1_2; break;
        case 1: m_CardVersion = HSMCICardVersion::MMC_1_4; break;
        case 2: m_CardVersion = HSMCICardVersion::MMC_2_2; break;
        case 3: m_CardVersion = HSMCICardVersion::MMC_3;   break;
        case 4: m_CardVersion = HSMCICardVersion::MMC_4;   break;
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

void HSMCIDriver::DecodeCSD_sd()
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
/// \brief Configures the driver with the selected card configuration
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HSMCIDriver::ApplySpeedAndBusWidth()
{
    uint32_t busWidthCfg = HSMCI_SDCR_SDCBUS_1;

    if (m_HighSpeed) {
        HSMCI->HSMCI_CFG |= HSMCI_CFG_HSMODE;
    } else {
        HSMCI->HSMCI_CFG &= ~HSMCI_CFG_HSMODE;
    }

    SetClockFrequency(m_Clock);

    switch (m_BusWidth)
    {
        case 1: busWidthCfg = HSMCI_SDCR_SDCBUS_1; break;
        case 4: busWidthCfg = HSMCI_SDCR_SDCBUS_4; break;
        case 8: busWidthCfg = HSMCI_SDCR_SDCBUS_8; break;
        default:
            kprintf("ERROR: HSMCIDriver invalid bus width (%d) using 1-bit.\n", m_BusWidth);
            break;
    }
    HSMCI->HSMCI_SDCR = HSMCI_SDCR_SDCSEL_SLOTA | busWidthCfg;    
}

///////////////////////////////////////////////////////////////////////////////
/// \brief ACMD6 - Define the data bus width to 4 bits bus
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool HSMCIDriver::ACmd6_sd()
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

bool HSMCIDriver::Cmd8_sd(bool* v2)
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

bool HSMCIDriver::Cmd8_mmc(bool* authorizeHighSpeed)
{
    if (!StartAddressedDataTransCmd(MMC_CMD8_SEND_EXT_CSD, 0, EXT_CSD_BSIZE, 1, true)) {
        return false;
    }
    std::vector<uint8_t> bufferAllocator;
    uint32_t* buffer;
    try {
        bufferAllocator.resize(EXT_CSD_BSIZE + DCACHE_LINE_SIZE - 1);
        buffer = reinterpret_cast<uint32_t*>((intptr_t(bufferAllocator.data()) + DCACHE_LINE_SIZE_MASK) & ~DCACHE_LINE_SIZE_MASK);
    } catch (std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }
    // Read and decode Extended Card Specific Data.
    if (!ReadDMA(buffer, EXT_CSD_BSIZE, 1)) {
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

bool HSMCIDriver::Cmd9MCI_sdmmc()
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

bool HSMCIDriver::Cmd13_sdmmc()
{
    bigtime_t deadline = get_system_time() + bigtime_from_s(1);
    for(;;)
    {
        if (!SendCmd(SDMMC_MCI_CMD13_SEND_STATUS, (uint32_t)m_RCA << 16)) {
            return false;
        }
        if (GetResponse() & CARD_STATUS_READY_FOR_DATA) {
            return true;
        }
        if (get_system_time() > deadline) {
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

bool HSMCIDriver::ACmd51_sd()
{
    uint8_t scr[SD_SCR_REG_BSIZE];

    // CMD55 - Tell the card that the next command is an application specific command.
    if (!SendCmd(SDMMC_CMD55_APP_CMD, (uint32_t)m_RCA << 16)) {
        return false;
    }
    if (!StartAddressedDataTransCmd(SD_ACMD51_SEND_SCR, 0, SD_SCR_REG_BSIZE, 1, false)) {
        return false;
    }
    if (!ReadNoDMA(scr, SD_SCR_REG_BSIZE, 1)) {
        return false;
    }

    // Get SD Memory Card - Spec. Version
    switch (SD_SCR_SD_SPEC(scr))
    {
        case SD_SCR_SD_SPEC_1_0_01: m_CardVersion = HSMCICardVersion::SD_1_0; break;
        case SD_SCR_SD_SPEC_1_10:   m_CardVersion = HSMCICardVersion::SD_1_10; break;
        case SD_SCR_SD_SPEC_2_00:   m_CardVersion = (SD_SCR_SD_SPEC3(scr) == SD_SCR_SD_SPEC_3_00) ? HSMCICardVersion::SD_3_0 : HSMCICardVersion::SD_2_0; break;
        default:                    m_CardVersion = HSMCICardVersion::SD_1_0; break;
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

bool HSMCIDriver::Cmd52_sdio(uint8_t rwFlag, uint8_t functionNumber, uint32_t registerAddr, uint8_t readAfterWriteFlag, uint8_t* data)
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

bool HSMCIDriver::Cmd53_sdio(uint8_t rwFlag, uint8_t functionNumber, uint32_t registerAddr, uint8_t incrementAddr, uint32_t size, bool useDMA)
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


    return StartAddressedDataTransCmd((rwFlag == SDIO_CMD53_READ_FLAG) ? SDIO_CMD53_IO_R_BYTE_EXTENDED : SDIO_CMD53_IO_W_BYTE_EXTENDED, cmdArgs, size, 1, useDMA);
}
