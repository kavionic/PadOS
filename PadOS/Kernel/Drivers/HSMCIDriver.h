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

#include "sd_mmc_protocol.h"

namespace kernel
{

typedef uint32_t offset_t;
/*
#define SD_CTRL_PORT    e_DigitalPortID_E
#define SD_CS           PIN5_bm
#define SD_RAW_SAVE_RAM 1

#define SD_CARD_DETECT_PORT e_DigitalPortID_B
#define SD_CARD_DETECT_PIN  PIN2_bm

#define SD_CLK_DIVIDER_PRE_INIT SPI::e_Divider_128
#define SD_CLK_DIVIDER_POST_INIT SPI::e_Divider_2
#define SD_CLK_DIVIDER_BURST SPI::e_Divider_2
*/



typedef uint8_t sd_mmc_err_t; //!< Type of return error code

//! \name Return error codes
//! @{
#define SD_MMC_OK               0    //! No error
#define SD_MMC_INIT_ONGOING     1    //! Card not initialized
#define SD_MMC_ERR_NO_CARD      2    //! No SD/MMC card inserted
#define SD_MMC_ERR_UNUSABLE     3    //! Unusable card
#define SD_MMC_ERR_SLOT         4    //! Slot unknow
#define SD_MMC_ERR_COMM         5    //! General communication error
#define SD_MMC_ERR_PARAM        6    //! Illeage input parameter
#define SD_MMC_ERR_WP           7    //! Card write protected
//! @}

typedef uint8_t card_type_t; //!< Type of card type

//! \name Card Types
//! @{
#define CARD_TYPE_UNKNOWN   (0)      //!< Unknown type card
#define CARD_TYPE_SD        (1 << 0) //!< SD card
#define CARD_TYPE_MMC       (1 << 1) //!< MMC card
#define CARD_TYPE_SDIO      (1 << 2) //!< SDIO card
#define CARD_TYPE_HC        (1 << 3) //!< High capacity card
//! SD combo card (io + memory)
#define CARD_TYPE_SD_COMBO  (CARD_TYPE_SD | CARD_TYPE_SDIO)
//! @}

typedef uint8_t card_version_t; //!< Type of card version

//! \name Card Versions
//! @{
#define CARD_VER_UNKNOWN   (0)       //! Unknown card version
#define CARD_VER_SD_1_0    (0x10)    //! SD version 1.0 and 1.01
#define CARD_VER_SD_1_10   (0x1A)    //! SD version 1.10
#define CARD_VER_SD_2_0    (0X20)    //! SD version 2.00
#define CARD_VER_SD_3_0    (0X30)    //! SD version 3.0X
#define CARD_VER_MMC_1_2   (0x12)    //! MMC version 1.2
#define CARD_VER_MMC_1_4   (0x14)    //! MMC version 1.4
#define CARD_VER_MMC_2_2   (0x22)    //! MMC version 2.2
#define CARD_VER_MMC_3     (0x30)    //! MMC version 3
#define CARD_VER_MMC_4     (0x40)    //! MMC version 4
//! @}

//! This SD MMC stack uses the maximum block size autorized (512 bytes)
#define SD_MMC_BLOCK_SIZE          512

/**
 * \addtogroup sd_raw
 *
 * @{
 */
/**
 * \file
 * MMC/SD/SDHC raw access header (license: GPLv2 or LGPLv2.1)
 *
 * \author Roland Riegel
 */

/**
 * The card's layout is harddisk-like, which means it contains
 * a master boot record with a partition table.
 */
#define SD_RAW_FORMAT_HARDDISK 0
/**
 * The card contains a single filesystem and no partition table.
 */
#define SD_RAW_FORMAT_SUPERFLOPPY 1
/**
 * The card's layout follows the Universal File Format.
 */
#define SD_RAW_FORMAT_UNIVERSAL 2
/**
 * The card's layout is unknown.
 */
#define SD_RAW_FORMAT_UNKNOWN 3

/**
 * This struct is used by sd_raw_get_info() to return
 * manufacturing and status information of the card.
 */
struct sd_raw_info
{
    // A manufacturer code globally assigned by the SD card organization.
    uint8_t manufacturer;
    // A string describing the card's OEM or content, globally assigned by the SD card organization.
    uint8_t oem[3];
    // A product name.
    uint8_t product[6];
     // The card's revision, coded in packed BCD. For example, the revision value \c 0x32 means "3.2".
    uint8_t revision;
     // A serial number assigned by the manufacturer.
    uint32_t serial;
     // The year of manufacturing.
     // A value of zero means year 2000.
    uint8_t manufacturing_year;
     // The month of manufacturing.
    uint8_t manufacturing_month;
     // The card's total capacity in bytes.
    offset_t capacity;
     // Defines wether the card's content is original or copied.
     // A value of \c 0 means original, \c 1 means copied.
    uint8_t flag_copy;
     // Defines wether the card's content is write-protected.
     //
     // \note This is an internal flag and does not represent the
     //       state of the card's mechanical write-protect switch.
    uint8_t flag_write_protect;
     // Defines whether the card's content is temporarily write-protected.
     //
     // \note This is an internal flag and does not represent the
     //       state of the card's mechanical write-protect switch.
    uint8_t flag_write_protect_temp;
     // The card's data layout.
     //
     // See the \c SD_RAW_FORMAT_* constants for details.
     //
     // \note This value is not guaranteed to match reality.
    uint8_t format;
};


//! SD/MMC card states
enum card_state {
    SD_MMC_CARD_STATE_READY    = 0, //!< Ready to use
    SD_MMC_CARD_STATE_DEBOUNCE = 1, //!< Debounce on going
    SD_MMC_CARD_STATE_INIT     = 2, //!< Initialization on going
    SD_MMC_CARD_STATE_UNUSABLE = 3, //!< Unusable card
    SD_MMC_CARD_STATE_NO_CARD  = 4, //!< No SD/MMC card inserted
};

class HSMCIDriver
{
public:
    HSMCIDriver(const DigitalPin& pinCD);
    uint8_t Initialize();
    
    int8_t ReadBlocks(offset_t offset, uint8_t* buffer, int8_t length);

    bool   StartReadBlocks(offset_t offset, uint16_t blockCount);
    
/*    inline void StartBlock()
    {
        m_SPI.SetDivider(SD_CLK_DIVIDER_POST_INIT);
        // Setup the CRC generator.
        CRC.CTRL = CRC_RESET_RESET0_gc;
        CRC.CTRL = CRC_SOURCE_IO_gc;
        // Wait for data block start byte (0xfe).
        m_SPI.StartBurst(0xff); // Start clocking in the first byte.
        while( m_SPI.ReadBurst(0xff) != 0xfe );
        m_SPI.SetDivider(SD_CLK_DIVIDER_BURST);
    }
    inline uint8_t ReadNextByte()
    {
        uint8_t data = m_SPI.ReadBurst(0xff);
        CRC.DATAIN = data;
        return data;
    }
    inline bool EndBlock()
    {
        // Read CRC.
        CRC.DATAIN = m_SPI.ReadBurst(0xff);
        CRC.DATAIN = m_SPI.EndBurst();

        m_SPI.SetDivider(SD_CLK_DIVIDER_POST_INIT);
        
        // Tell the CRC generator that we are done.
        CRC.STATUS |= CRC_BUSY_bm;
                
        return (CRC.STATUS & CRC_ZERO_bm) != 0 || 1;
    }
    */
    bool ReadNextBlocks(uint8_t* buffer, int32_t length);
    bool EndReadBlocks();
    
    uint8_t sd_raw_available();
    uint8_t sd_raw_locked();

    uint8_t sd_raw_write(offset_t offset, const uint8_t* buffer, uintptr_t length);
    uint8_t sd_raw_sync();

    uint8_t sd_raw_get_info(struct sd_raw_info* info);

//private:
/*    enum CardType_e
    {
        e_CardTypeNone,
        e_CardTypeMMC,
        e_CardTypeSD1,
        e_CardTypeSD2,
        e_CardTypeSDHC
    };*/
//    void select_card();
//    void unselect_card();    
    
    /* private helper functions */
//    void sd_raw_send_byte(uint8_t b);
//    uint8_t sd_raw_rec_byte();
//    uint8_t SendCommand(uint8_t command, uint32_t arg);
    

    bool sd_mci_op_cond(uint8_t v2);
    bool mmc_mci_op_cond();
    bool sdio_op_cond();
    bool sdio_cmd52(uint8_t rw_flag, uint8_t func_nb, uint32_t reg_addr, uint8_t rd_after_wr, uint8_t *io_data);

    bool sdio_cmd52_set_bus_width();
    bool sdio_cmd52_set_high_speed();

    bool sdio_get_max_speed();

    bool sd_cm6_set_high_speed();
    bool mmc_cmd6_set_bus_width(uint8_t bus_width);
    bool mmc_cmd6_set_high_speed();

    bool sd_mmc_cmd9_mci();
    void mmc_decode_csd();
    void sd_decode_csd();

    bool sd_mmc_cmd13();

    sd_mmc_err_t sd_mmc_select_slot(uint8_t slot);
    void sd_mmc_configure_slot();
    bool sd_acmd6();
    bool sd_acmd51();

    void sd_mmc_deselect_slot();

    bool sd_cmd8(uint8_t * v2);
    bool mmc_cmd8(uint8_t *b_authorize_high_speed);
    bool sd_mmc_mci_install_mmc();

    bool sd_mmc_mci_card_init();

    sd_mmc_err_t sd_mmc_check(uint8_t slot);
    card_type_t sd_mmc_get_type(uint8_t slot);
    card_version_t sd_mmc_get_version(uint8_t slot);
    uint32_t sd_mmc_get_capacity(uint8_t slot);



    sd_mmc_err_t sd_mmc_init_read_blocks(uint8_t slot, uint32_t start, uint16_t nb_block);
    sd_mmc_err_t sd_mmc_start_read_blocks(void *dest, uint16_t nb_block);
    sd_mmc_err_t sd_mmc_wait_end_of_read_blocks(bool abort);
    sd_mmc_err_t sd_mmc_init_write_blocks(uint8_t slot, uint32_t start, uint16_t nb_block);
    sd_mmc_err_t sd_mmc_start_write_blocks(const void *src, uint16_t nb_block);
    sd_mmc_err_t sd_mmc_wait_end_of_write_blocks(bool abort);
    sd_mmc_err_t sdio_read_direct(uint8_t slot, uint8_t func_num, uint32_t addr, uint8_t *dest);
    sd_mmc_err_t sdio_write_direct(uint8_t slot, uint8_t func_num, uint32_t addr, uint8_t data);
    sd_mmc_err_t sdio_read_extended(uint8_t slot, uint8_t func_num, uint32_t addr, uint8_t inc_addr, uint8_t *dest, uint16_t size);
    sd_mmc_err_t sdio_write_extended(uint8_t slot, uint8_t func_num, uint32_t addr, uint8_t inc_addr, uint8_t *src, uint16_t size);

    card_type_t m_CardType;
    card_version_t m_CardVersion;
    uint32_t   m_Clock = 0;
    uint32_t   m_Capacity = 0;         //!< Card capacity in KBytes
    enum card_state m_State;     //!< Card state
    //card_type_t m_Type;          //!< Card type
    card_version_t m_Version;    //!< Card version
    uint32_t   m_BusWidth = 1;
    uint8_t    m_CSD[CSD_REG_BSIZE];//!< CSD register
    bool       m_HighSpeed = false;

    uint16_t m_RCA;              //!< Relative card address

    //DigitalPin m_PinCS;
    DigitalPin m_PinCD;
};

extern HSMCIDriver sdcard; // Must be defined by the application code.

} // namespace
