// This file is part of PadOS.
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <malloc.h>
#include <Kernel/HAL/STM32/USB_STM32.h>
#include <Kernel/USB/ClassDrivers/USBCDCBuffers.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBCDCBuffers::USBCDCBuffers(size_t receivePacketSize, size_t transmitPacketSize)
    : m_Storage(AllocateStorage(receivePacketSize, transmitPacketSize), &std::free)
    , m_ReceiveQueue(m_Storage.get(), std::max<size_t>(receivePacketSize, __SCB_DCACHE_LINE_SIZE),
        RECEIVE_STORAGE_SIZE / std::max<size_t>(receivePacketSize, __SCB_DCACHE_LINE_SIZE))
    , m_TransmitQueue(m_Storage.get() + RECEIVE_STORAGE_SIZE, TRANSMIT_BLOCK_SIZE, TRANSMIT_BLOCK_COUNT)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t* USBCDCBuffers::AllocateStorage(size_t receivePacketSize, size_t transmitPacketSize)
{
    if (receivePacketSize == 0 || receivePacketSize > 512 || (receivePacketSize & (receivePacketSize - 1)) != 0
        || transmitPacketSize == 0 || transmitPacketSize > 512 || (transmitPacketSize & (transmitPacketSize - 1)) != 0)
    {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    constexpr size_t allocationSize = RECEIVE_STORAGE_SIZE + TRANSMIT_BLOCK_SIZE * TRANSMIT_BLOCK_COUNT;
    auto* storage = static_cast<uint8_t*>(memalign(__SCB_DCACHE_LINE_SIZE, allocationSize));
    if (storage == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOMEM);
    }
    if (!USB_STM32::IsDirectDMAReceiveBuffer(storage, allocationSize))
    {
        std::free(storage);
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    return storage;
}

} // namespace kernel
