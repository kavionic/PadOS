// This file is part of PadOS.
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdlib>
#include <memory>
#include <Kernel/USB/ClassDrivers/USBCDCQueue.h>

namespace kernel
{

// Both CDC roles use the same cache-line-isolated heap storage and queue layout.
// The owning channel serializes queue access and stops DMA before destruction.
class USBCDCBuffers
{
public:
    USBCDCBuffers(size_t receivePacketSize, size_t transmitPacketSize);

    USBCDCQueue& GetReceiveQueue() { return m_ReceiveQueue; }
    USBCDCQueue& GetTransmitQueue() { return m_TransmitQueue; }

private:
    static constexpr size_t RECEIVE_STORAGE_SIZE = 1024;
    static constexpr size_t TRANSMIT_BLOCK_SIZE = 1024;
    static constexpr size_t TRANSMIT_BLOCK_COUNT = 2;

    static uint8_t* AllocateStorage(size_t receivePacketSize, size_t transmitPacketSize);

    std::unique_ptr<uint8_t, void (*)(void*)> m_Storage;
    USBCDCQueue m_ReceiveQueue;
    USBCDCQueue m_TransmitQueue;
};

} // namespace kernel
