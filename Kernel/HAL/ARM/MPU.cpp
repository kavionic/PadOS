/**
 * \file
 *
 * \brief SAMV70/SAMV71/SAME70/SAMS70-XULTRA board mpu config.
 *
 * Copyright (c) 2015-2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include "System/Platform.h"
#include "Kernel/Scheduler.h"
#include "Kernel/HAL/ARM/MPU.h"

/**
 * \defgroup sam_drivers_mpu_group MPU - Memory Protect Unit
 * @{
 */

/** \file */

/**
 * \addtogroup mmu MMU Initialization
 *
 * \section Usage
 *
 * Translation Look-aside Buffers (TLBs) are an implementation technique that
 * caches translations or translation table entries. TLBs avoid the requirement
 * for every memory access to perform a translation table lookup.
 * The ARM architecture does not specify the exact form of the TLB structures
 * for any design. In a similar way to the requirements for caches, the
 * architecture only defines certain principles for TLBs:
 *
 * The MMU supports memory accesses based on memory sections or pages:
 * Super-sections Consist of 16MB blocks of memory. Support for Super sections
 * is optional.
 * -# Sections Consist of 1MB blocks of memory.
 * -# Large pages Consist of 64KB blocks of memory.
 * -# Small pages Consist of 4KB blocks of memory.
 *
 * Access to a memory region is controlled by the access permission bits and
 * the domain field in the TLB entry.
 * Memory region attributes
 * Each TLB entry has an associated set of memory region attributes. These
 * control accesses to the caches,
 * how the write buffer is used, and if the memory region is Shareable and
 * therefore must be kept coherent.
 *
 * Related files:\n
 * \ref mmu.c\n
 * \ref mmu.h \n
 */

/*----------------------------------------------------------------------------
 *        Exported functions

 *----------------------------------------------------------------------------*/
/**
 * \brief Enables the MPU module.
 *
 * \param dwMPUEnable  Enable/Disable the memory region.
 */
IFLASHC void mpu_enable(uint32_t dw_mpu_enable)
{
    MPU->CTRL = dw_mpu_enable ;
}

/**
 * \brief Set active memory region.
 *
 * \param dwRegionNum  The memory region to be active.
 */
IFLASHC void mpu_set_region_num(uint32_t dw_region_num)
{
    MPU->RNR = dw_region_num;
}

/**
 * \brief Disable the current active region.
 */
IFLASHC void mpu_disable_region(void)
{
    MPU->RASR &= 0xfffffffe;
}

/**
 * \brief Setup a memory region.
 *
 * \param dwRegionBaseAddr  Memory region base address.
 * \param dwRegionAttr  Memory region attributes.
 */
IFLASHC void mpu_set_region(uint32_t dw_region_base_addr, uint32_t dw_region_attr)
{
    MPU->RBAR = dw_region_base_addr;
    MPU->RASR = dw_region_attr;
}


/**
 * \brief Calculate region size for the RASR.
 */
IFLASHC uint32_t mpu_cal_mpu_region_size(uint32_t dw_actual_size_in_bytes)
{
    const uint32_t dwReturnValue = std::max(4, std::bit_width(dw_actual_size_in_bytes - 1) - 1);
    return dwReturnValue << MPU_RASR_SIZE_Pos;
}

/**
 *  \brief Update MPU regions.
 *
 *  \return Unused (ANSI-C compatibility).
 */
IFLASHC void mpu_update_regions(uint32_t dw_region_num, uint32_t dw_region_base_addr, uint32_t dw_region_attr)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    /* Clean up data and instruction buffer */
    __DSB();
    __ISB();

    /* Set active region */
    mpu_set_region_num(dw_region_num);

    /* Disable region */
    mpu_disable_region();

    /* Update region attribute */
    mpu_set_region( dw_region_base_addr, dw_region_attr);

    /* Clean up data and instruction buffer to make the new region taking
       effect at once */
    __DSB();
    __ISB();
}

//@}
