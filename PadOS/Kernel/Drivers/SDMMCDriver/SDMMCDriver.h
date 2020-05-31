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

#pragma once


#include <stdint.h>

#include "Kernel/HAL/DigitalPort.h"
#include "Kernel/VFS/KDeviceNode.h"

#include "SDMMCProtocol.h"
#include "System/Ptr/Ptr.h"
#include "System/Threads/Thread.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"
#include "Kernel/KSemaphore.h"
#include "Kernel/VFS/KINode.h"

namespace kernel
{
class KFileNode;

namespace SDMMCCardType
{
    typedef uint8_t Type;
    static const uint8_t Unknown  = 0x00;      //!< Unknown type card
    static const uint8_t SD       = 0x01; //!< SD card
    static const uint8_t MMC      = 0x02; //!< MMC card
    static const uint8_t SDIO     = 0x04; //!< SDIO card
    static const uint8_t HC       = 0x08; //!< High capacity card
    static const uint8_t SD_COMBO = (SD | SDIO); //!< SD combo card (io + memory)
}

enum class SDMMCCardVersion
{
    Unknown  = 0,      //! Unknown card version
    SD_1_0   = 0x10,   //! SD version 1.0 and 1.01
    SD_1_10  = 0x1A,   //! SD version 1.10
    SD_2_0   = 0X20,   //! SD version 2.00
    SD_3_0   = 0X30,   //! SD version 3.0X
    MMC_1_2  = 0x12,   //! MMC version 1.2
    MMC_1_4  = 0x14,   //! MMC version 1.4
    MMC_2_2  = 0x22,   //! MMC version 2.2
    MMC_3    = 0x30,   //! MMC version 3
    MMC_4    = 0x40    //! MMC version 4
};

class SDMMCINode : public KINode
{
public:
    SDMMCINode(KFilesystemFileOps* fileOps);
    int		bi_nOpenCount = 0;
    int		bi_nNodeHandle = -1;
    int		bi_nPartitionType = 0;
    off64_t	bi_nStart = 0;
    off64_t	bi_nSize = 0;    
};

class SDMMCDriver : public PtrTarget, public KFilesystemFileOps, public os::Thread
{
public:
    SDMMCDriver();
	virtual ~SDMMCDriver();

    bool SetupBase(const os::String& devicePath, const DigitalPin& pinCD);
    
    virtual int Run() override;
    
    virtual Ptr<KFileNode> OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int flags) override;

    virtual int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;
    virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length) override;

protected:
    static constexpr uint32_t BLOCK_SIZE = 512;
//	static constexpr SDMMC_BlockSizePowers BLOCK_SIZE_BITS = SDMMC_BlockSizePowers::SZ512;

    enum class CardState
    {
        NoCard,
        Ready,
        Initializing,
        Unusable
    };

    bool InitializeCard();
    bool InitializeMMCCard();

    static size_t ReadPartitionData(void* userData, off64_t position, void* buffer, size_t size);

    int  DecodePartitions(bool force);

    void SetState(CardState state);
    
    bool IsReady()
    {
        for (;;)
        {
            if (m_CardState == CardState::Ready) {
                return true;
            } else if (m_CardState == CardState::Initializing) {
                m_CardStateCondition.Wait(m_Mutex);
            } else {
                return false;
            }
        }            
    }
    
    
    status_t SDIOReadDirect(uint8_t functionNumber, uint32_t addr, uint8_t *dest);
    status_t SDIOWriteDirect(uint8_t functionNumber, uint32_t addr, uint8_t data);
    ssize_t  SDIOReadExtended(uint8_t functionNumber, uint32_t addr, uint8_t incrementAddr, void* buffer, size_t size);
    ssize_t  SDIOWriteExtended(uint8_t functionNumber, uint32_t addr, uint8_t incrementAddr, const void* buffer, size_t size);

    virtual void     SetClockFrequency(uint32_t frequency) = 0;
    virtual void     SendClock() = 0;
    
    virtual bool		SendCmd(uint32_t cmd, uint32_t arg) = 0;
	virtual uint32_t	GetResponse() = 0;
    virtual void		GetResponse128(uint8_t* response) = 0;
    virtual bool		StartAddressedDataTransCmd(uint32_t cmd, uint32_t arg, uint32_t blockSizePower, uint16_t blockCount, const void* buffer) = 0;
    virtual bool		StopAddressedDataTransCmd(uint32_t cmd, uint32_t arg) = 0;
    
    bool     OperationalConditionMCI_sd(bool v2);
    bool     OperationalConditionMCI_mmc();
    bool     OperationalCondition_sdio();
    
    bool     GetMaxSpeed_sdio();

    bool     SetBusWidth_sdio();
    bool     SetHighSpeed_sdio();

    bool     SetHighSpeed_sd();
    bool     SetBusWidth_mmc(int busWidth);
    bool     SetHighSpeed_mmc();

    void     DecodeCSD_mmc();
    void     DecodeCSD_sd();

    virtual void     ApplySpeedAndBusWidth() = 0;
    
    bool     ACmd6_sd();

    bool     Cmd8_sd(bool* v2);
    bool     Cmd8_mmc(bool* authorizeHighSpeed);

    bool     Cmd9MCI_sdmmc();

    bool     Cmd13_sdmmc();

    bool     ACmd51_sd();

    bool     Cmd52_sdio(uint8_t rwFlag, uint8_t functionNumber, uint32_t registerAddr, uint8_t readAfterWriteFlag, uint8_t* data);
    bool     Cmd53_sdio(uint8_t rwFlag, uint8_t functionNumber, uint32_t registerAddr, uint8_t incrementAddr, uint32_t size, const void* buffer);

    KMutex              m_Mutex;
    KConditionVariable  m_CardStateCondition;
    KConditionVariable  m_IOCondition;
    KSemaphore          m_DeviceSemaphore;
    DigitalPin          m_PinCD;
    
    os::String                   m_DevicePathBase;
    Ptr<SDMMCINode>              m_RawINode;
    std::vector<Ptr<SDMMCINode>> m_PartitionINodes;
    
    bool                m_CardInserted = false;
    SDMMCCardType::Type m_CardType     = SDMMCCardType::Unknown;
    SDMMCCardVersion    m_CardVersion  = SDMMCCardVersion::Unknown;
    uint32_t            m_Clock        = 0;
    uint64_t            m_SectorCount  = 0;
    CardState           m_CardState    = CardState::NoCard;
    int                 m_BusWidth     = 1;
    bool                m_HighSpeed    = false;

	void*				m_CacheAlignedBuffer = nullptr;	// 512 byte cache-line aligned temp-buffer for IO.
    uint16_t            m_RCA;                // Relative card address
    uint8_t             m_CSD[sdmmc::CSD_REG_SIZE_BYTES]; // CSD register

    volatile int        m_IOError       = 0;
};

} // namespace
