// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 03.05.2018 21:28:43

#include "System/Platform.h"

#include <sys/uio.h>
#include <sys/pados_syscalls.h>
#include <malloc.h>
#include <string.h>

#include <algorithm>
#include <bit>
#include <functional>
#include <limits>
#include <map>
#include <new>

#include <Kernel/KTime.h>
#include <Kernel/VFS/KBlockCache.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KVFSManager.h>
#include <Kernel/KThread.h>
#include <Kernel/KLogging.h>
#include <Utils/Utils.h>
#include <System/ExceptionHandling.h>


namespace kernel
{

static constexpr size_t KBLOCK_CACHE_MEMORY_SIZE = 2 * 1024 * 1024;
static constexpr size_t KBLOCK_CACHE_BUFFER_COUNT = KBLOCK_CACHE_MEMORY_SIZE / KBlockCache::CACHE_BUFFER_SIZE;
static constexpr size_t KBLOCK_CACHE_BLOCKS_PER_BUFFER = KBlockCache::CACHE_BUFFER_SIZE / KBlockCache::MIN_BLOCK_SIZE;
static constexpr size_t KBLOCK_CACHE_BLOCK_HEADER_COUNT = KBLOCK_CACHE_BUFFER_COUNT * KBLOCK_CACHE_BLOCKS_PER_BUFFER;

static uint8_t* gk_BCacheBuffer;
static KCacheBuffer* gk_BCacheBuffers;
static KCacheBlockHeader gk_BCacheHeaders[KBLOCK_CACHE_BLOCK_HEADER_COUNT];

std::map<int, KBlockCache*>         KBlockCache::s_DeviceMap;
PIntrusiveList<KCacheBuffer>        KBlockCache::s_FreeBufferList;
PIntrusiveList<KCacheBuffer>        KBlockCache::s_BufferLRUList;
PIntrusiveList<KCacheBlockHeader>   KBlockCache::s_FreeBlockLists[KBlockCache::BLOCK_SIZE_ORDER_COUNT];
PIntrusiveList<KCacheBlockHeader>   KBlockCache::s_BlockLRULists[KBlockCache::BLOCK_SIZE_ORDER_COUNT];
KMutex                              KBlockCache::s_Mutex("bcache_mutex", PEMutexRecursionMode_RaiseError);
KConditionVariable                  KBlockCache::s_FlushingRequestConditionVar("bcache_flush_req");
KConditionVariable                  KBlockCache::s_FlushingDoneConditionVar("bcache_flush_done");
std::atomic_int                     KBlockCache::s_DirtyBlockCount;
std::atomic_size_t                  KBlockCache::s_DirtyByteCount;
size_t                              KBlockCache::s_PendingReadOnlySignalCount;
size_t                              KBlockCache::s_NextFlushBlockSizeOrder;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KBlockCache::~KBlockCache()
{
    if (!SetDevice(-1, 0, 0))
    {
        CRITICAL_SCOPE(s_Mutex);

        if (m_Device != -1)
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_BlockCache, "KBlockCache::~KBlockCache() forcibly discarding blocks for device {}.", m_Device);
            DetachBlocks(true, true);

            auto registeredDevice = s_DeviceMap.find(m_Device);
            if (registeredDevice != s_DeviceMap.end() && registeredDevice->second == this) {
                s_DeviceMap.erase(registeredDevice);
            }
            if (m_ReadOnlySignalPending)
            {
                kassert(s_PendingReadOnlySignalCount != 0);
                --s_PendingReadOnlySignalCount;
            }
            m_Device = -1;
            m_IsReadOnly.store(true, std::memory_order_relaxed);
            m_WriteError = PErrorCode::Success;
            m_ReadOnlySignalPending = false;
            m_ReadOnlySignalInProgress = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KBlockCache* KBlockCache::GetDeviceCache(int device)
{
    auto i = s_DeviceMap.find(device);
    if (i != s_DeviceMap.end()) {
        return i->second;
    } else {
        kernel_log<PLogSeverity::ERROR>(LogCatKernel_BlockCache, "KBlockCache::GetDeviceCache() device {} not registered!", device);
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::SetDevice(int device, off64_t blockCount, size_t blockSize, bool readOnly)
{
    if (device != -1 &&
        (blockCount < 0 || blockSize < MIN_BLOCK_SIZE || blockSize > MAX_BLOCK_SIZE ||
            !std::has_single_bit(blockSize) || CACHE_BUFFER_SIZE % blockSize != 0 ||
            blockCount > std::numeric_limits<off64_t>::max() / off64_t(blockSize)))
    {
        kernel_log<PLogSeverity::ERROR>(
            LogCatKernel_BlockCache,
            "KBlockCache::SetDevice() invalid geometry for device {}: {} blocks of {} bytes.",
            device,
            blockCount,
            blockSize);
        return false;
    }

    if (m_Device != -1) {
        Sync();
    }

    CRITICAL_SCOPE(s_Mutex);

    if (device != -1)
    {
        auto registeredDevice = s_DeviceMap.find(device);
        if (registeredDevice != s_DeviceMap.end() && registeredDevice->second != this)
        {
            kernel_log<PLogSeverity::ERROR>(LogCatKernel_BlockCache, "KBlockCache::SetDevice() device {} already registered!", device);
            return false;
        }
    }

    if (m_Device != -1)
    {
        if (!DetachBlocks(device == -1, false)) {
            return false;
        }

        auto registeredDevice = s_DeviceMap.find(m_Device);
        if (registeredDevice != s_DeviceMap.end() && registeredDevice->second == this) {
            s_DeviceMap.erase(registeredDevice);
        } else {
            kernel_log<PLogSeverity::ERROR>(LogCatKernel_BlockCache, "KBlockCache::SetDevice() previous device {} not registered to this cache!", m_Device);
        }
        if (m_ReadOnlySignalPending)
        {
            kassert(s_PendingReadOnlySignalCount != 0);
            --s_PendingReadOnlySignalCount;
        }
    }

    m_Device = device;
    m_IsReadOnly.store(device == -1 || readOnly, std::memory_order_relaxed);
    m_WriteError = PErrorCode::Success;
    m_ReadOnlySignalPending = false;
    m_ReadOnlySignalInProgress = false;
    m_BlockCount = 0;
    m_BlockSize = 0;
    m_BlockSizeOrder = 0;

    if (m_Device == -1) {
        return true;
    }

    s_DeviceMap[m_Device] = this;
    
    m_BlockCount     = blockCount;
    m_BlockSize      = blockSize;
    m_BlockSizeOrder = GetBlockSizeOrder(blockSize);
    return true;
}

void KBlockCache::Initialize()
{
    size_t bufferMetadataOffset = KBLOCK_CACHE_MEMORY_SIZE;
#ifdef PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
    bufferMetadataOffset += GetFlushDiagnosticBufferSize();
#endif // PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
    bufferMetadataOffset = align_up(bufferMetadataOffset, alignof(KCacheBuffer));
    const size_t allocationSize = bufferMetadataOffset + sizeof(KCacheBuffer) * KBLOCK_CACHE_BUFFER_COUNT;

    gk_BCacheBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, allocationSize));
    kassert(gk_BCacheBuffer != nullptr);
#ifdef PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
    InitializeFlushDiagnostics(gk_BCacheBuffer + KBLOCK_CACHE_MEMORY_SIZE);
#endif // PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
    gk_BCacheBuffers = reinterpret_cast<KCacheBuffer*>(gk_BCacheBuffer + bufferMetadataOffset);

    for (size_t bufferIndex = 0; bufferIndex < KBLOCK_CACHE_BUFFER_COUNT; ++bufferIndex)
    {
        KCacheBuffer* buffer = new (&gk_BCacheBuffers[bufferIndex]) KCacheBuffer;
        buffer->m_Buffer = gk_BCacheBuffer + bufferIndex * CACHE_BUFFER_SIZE;
        buffer->m_Blocks = &gk_BCacheHeaders[bufferIndex * KBLOCK_CACHE_BLOCKS_PER_BUFFER];
        for (size_t blockIndex = 0; blockIndex < KBLOCK_CACHE_BLOCKS_PER_BUFFER; ++blockIndex) {
            buffer->m_Blocks[blockIndex].CacheBuffer = buffer;
        }
        s_FreeBufferList.Append(buffer);
    }
    PThreadAttribs attrs("disk_cache_flusher", 0, PThreadDetachState_Detached, 4096);
    kthread_spawn_trw(
        &attrs,
        nullptr,
#ifdef PADOS_MODULE_USER_SPACE
        nullptr,
#endif // PADOS_MODULE_USER_SPACE
        KSpawnThreadFlag::Privileged,
        nullptr,
        DiskCacheFlusher,
        nullptr
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KCacheBlockDesc KBlockCache::GetBlock_trw(off64_t blockNum, bool doLoad)
{
    CRITICAL_SCOPE(s_Mutex);

    if (m_Device == -1 || blockNum < 0 || blockNum >= m_BlockCount) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    for (;;)
    {
        auto i = m_BlockMap.find(blockNum);
        if (i != m_BlockMap.end())
        {
            KCacheBlockHeader* block = i->second;
            kassert(block->m_BlockCache == this);
            if (block->IsDiscardRequested())
            {
                if (block->m_UseCount == 0 && !block->IsFlushing()) {
                    ReleaseBlock(block);
                } else {
                    s_FlushingDoneConditionVar.Wait(s_Mutex);
                }
                continue;
            }
            block->AddRef();
            PIntrusiveList<KCacheBlockHeader>& blockLRUList = s_BlockLRULists[m_BlockSizeOrder];
            if (blockLRUList.GetLast() != block)
            {
                blockLRUList.Remove(block);
                blockLRUList.Append(block);
            }
            TouchBuffer(block->CacheBuffer);
            return KCacheBlockDesc(block);
        }

        KCacheBlockHeader* block = AllocateBlock(m_BlockSize, m_BlockSizeOrder);
        kassert(block != nullptr);
        kassert(block->m_BlockCache == nullptr);
        PScopeFail contextCleanup([block]() { ReleaseUnusedBlock(block); });
        if (doLoad)
        {
            const size_t bytesRead = kpread_trw(m_Device, block->m_Buffer, m_BlockSize, blockNum * off64_t(m_BlockSize));
            if (bytesRead != m_BlockSize) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }
        }
        const bool didInsert = m_BlockMap.emplace(blockNum, block).second;
        kassert(didInsert);

        block->m_BlockCache   = this;
        block->m_bufferNumber = blockNum;
        block->m_UseCount     = 1;
        block->m_Flags        = 0;
        s_BlockLRULists[m_BlockSizeOrder].Append(block);
        TouchBuffer(block->CacheBuffer);

        return KCacheBlockDesc(block);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KBlockCache::MarkBlockDirty(off64_t blockNum)
{
    CRITICAL_SCOPE(s_Mutex);

    auto i = m_BlockMap.find(blockNum);
    if (i != m_BlockMap.end())
    {
        KCacheBlockHeader* block = i->second;
        return block->SetDirty(true) ? PErrorCode::Success : PErrorCode::ROFS;
    }
    return PErrorCode::NOENT;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::CachedRead_trw(off64_t blockNum, void* buffer, size_t blockCount)
{
    for (size_t i = 0 ; i < blockCount ; ++i)
    {
        KCacheBlockDesc block = GetBlock_trw(blockNum + i, true);
        memcpy(reinterpret_cast<uint8_t*>(buffer) + i * m_BlockSize, block.m_Buffer, m_BlockSize);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::CachedWrite_trw(off64_t blockNum, const void* buffer, size_t blockCount)
{
    if (blockCount == 0) {
        return;
    }
    if (IsReadOnly()) {
        PERROR_THROW_CODE(PErrorCode::ROFS);
    }

    for (size_t i = 0 ; i < blockCount ; ++i)
    {
        KCacheBlockDesc block = GetBlock_trw(blockNum + i, false);
        memcpy(block.m_Buffer, reinterpret_cast<const uint8_t*>(buffer) + i * m_BlockSize, m_BlockSize);
        CRITICAL_SCOPE(s_Mutex);
        if (!block.m_Block->SetDirty(true)) {
            PERROR_THROW_CODE(PErrorCode::ROFS);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KBlockCache::GetBlockSizeOrder(size_t blockSize)
{
    kassert(blockSize >= MIN_BLOCK_SIZE);
    kassert(blockSize <= MAX_BLOCK_SIZE);
    kassert(std::has_single_bit(blockSize));
    kassert(CACHE_BUFFER_SIZE % blockSize == 0);

    const size_t blockSizeOrder = size_t(std::countr_zero(blockSize) - std::countr_zero(MIN_BLOCK_SIZE));
    kassert(blockSizeOrder < BLOCK_SIZE_ORDER_COUNT);
    return blockSizeOrder;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::ConfigureBuffer(KCacheBuffer* buffer, size_t blockSize, size_t blockSizeOrder)
{
    kassert(s_Mutex.IsLocked());
    kassert(buffer != nullptr);
    kassert(buffer->IsListMember(&s_FreeBufferList));
    kassert(buffer->m_BlockSize == 0);
    kassert(buffer->m_BlockCount == 0);
    kassert(buffer->m_UsedBlockCount == 0);

    s_FreeBufferList.Remove(buffer);

    buffer->m_BlockSize = blockSize;
    buffer->m_BlockCount = CACHE_BUFFER_SIZE / blockSize;

    for (size_t blockIndex = 0; blockIndex < buffer->m_BlockCount; ++blockIndex)
    {
        KCacheBlockHeader* block = &buffer->m_Blocks[blockIndex];
        kassert(!block->IsListMember());
        kassert(block->CacheBuffer == buffer);

        block->m_BlockCache = nullptr;
        block->m_bufferNumber = 0;
        block->m_UseCount = 0;
        block->m_Buffer = buffer->m_Buffer + blockIndex * blockSize;
        block->m_Flags = 0;
        s_FreeBlockLists[blockSizeOrder].Append(block);
    }
    s_BufferLRUList.Append(buffer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::ReleaseBuffer(KCacheBuffer* buffer)
{
    kassert(s_Mutex.IsLocked());
    kassert(buffer != nullptr);
    kassert(buffer->IsListMember(&s_BufferLRUList));
    kassert(buffer->m_UsedBlockCount == 0);

    const size_t blockSizeOrder = GetBlockSizeOrder(buffer->m_BlockSize);
    for (size_t blockIndex = 0; blockIndex < buffer->m_BlockCount; ++blockIndex)
    {
        KCacheBlockHeader* block = &buffer->m_Blocks[blockIndex];
        kassert(block->m_BlockCache == nullptr);
        kassert(block->m_UseCount == 0);
        kassert(!block->IsDirty());
        kassert(!block->IsFlushing());

        if (block->IsListMember())
        {
            kassert(block->IsListMember(&s_FreeBlockLists[blockSizeOrder]));
            s_FreeBlockLists[blockSizeOrder].Remove(block);
        }
        block->m_bufferNumber = 0;
        block->m_Buffer = nullptr;
        block->m_Flags = 0;
    }

    buffer->m_BlockSize = 0;
    buffer->m_BlockCount = 0;
    s_BufferLRUList.Remove(buffer);
    s_FreeBufferList.Append(buffer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::ReleaseUnusedBlock(KCacheBlockHeader* block)
{
    kassert(s_Mutex.IsLocked());
    kassert(block != nullptr);
    kassert(block->m_BlockCache == nullptr);
    kassert(!block->IsListMember());
    kassert(block->m_UseCount == 0);
    kassert(block->m_Flags == 0);

    KCacheBuffer* buffer = GetBlockBuffer(block);
    kassert(buffer->m_BlockSize != 0);
    kassert(buffer->m_UsedBlockCount != 0);
    --buffer->m_UsedBlockCount;

    block->m_bufferNumber = 0;
    s_FreeBlockLists[GetBlockSizeOrder(buffer->m_BlockSize)].Append(block);

    if (buffer->m_UsedBlockCount == 0) {
        ReleaseBuffer(buffer);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::ReuseBlock(KCacheBlockHeader* block)
{
    kassert(s_Mutex.IsLocked());
    kassert(block != nullptr);
    kassert(block->m_BlockCache != nullptr);
    kassert(block->m_UseCount == 0);
    kassert(!block->IsDirty());
    kassert(!block->IsFlushing());

    KBlockCache* ownerCache = block->m_BlockCache;
    PIntrusiveList<KCacheBlockHeader>& blockLRUList = s_BlockLRULists[ownerCache->m_BlockSizeOrder];
    kassert(block->IsListMember(&blockLRUList));
    const size_t removedBlockCount = ownerCache->m_BlockMap.erase(block->m_bufferNumber);
    kassert(removedBlockCount == 1);
    blockLRUList.Remove(block);

    block->m_BlockCache = nullptr;
    block->m_bufferNumber = 0;
    block->m_Flags = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KCacheBuffer* KBlockCache::FindReclaimableBuffer(
    size_t requestedBlockSize, bool hasSameSizeCandidate, bool& shouldWaitForFlushing)
{
    kassert(s_Mutex.IsLocked());

    shouldWaitForFlushing = false;
    for (KCacheBuffer* buffer : s_BufferLRUList)
    {
        kassert(buffer->m_UsedBlockCount != 0);
        if (buffer->m_BlockSize == requestedBlockSize)
        {
            if (hasSameSizeCandidate) {
                return nullptr;
            }
            continue;
        }

        bool hasReferencedBlocks = false;
        bool hasFlushingBlocks = false;
        for (size_t blockIndex = 0; blockIndex < buffer->m_BlockCount; ++blockIndex)
        {
            KCacheBlockHeader* block = &buffer->m_Blocks[blockIndex];
            if (block->m_BlockCache == nullptr)
            {
                kassert(block->IsListMember(&s_FreeBlockLists[GetBlockSizeOrder(buffer->m_BlockSize)]));
                continue;
            }
            if (block->m_UseCount != 0)
            {
                hasReferencedBlocks = true;
                break;
            }
            if (block->IsFlushing()) {
                hasFlushingBlocks = true;
            }
        }
        if (hasReferencedBlocks) {
            continue;
        }
        if (hasFlushingBlocks)
        {
            shouldWaitForFlushing = true;
            return nullptr;
        }
        return buffer;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::ReclaimBuffer(KCacheBuffer* buffer)
{
    kassert(s_Mutex.IsLocked());
    kassert(buffer != nullptr);
    kassert(buffer->IsListMember(&s_BufferLRUList));

    const size_t blockSizeOrder = GetBlockSizeOrder(buffer->m_BlockSize);
    PIntrusiveList<KCacheBlockHeader>& blockLRUList = s_BlockLRULists[blockSizeOrder];

    for (size_t blockIndex = 0; blockIndex < buffer->m_BlockCount; ++blockIndex)
    {
        KCacheBlockHeader* block = &buffer->m_Blocks[blockIndex];
        if (block->m_BlockCache == nullptr) {
            continue;
        }

        kassert(block->m_UseCount == 0);
        kassert(!block->IsDirty());
        kassert(!block->IsFlushing());

        KBlockCache* ownerCache = block->m_BlockCache;
        kassert(ownerCache->m_BlockSizeOrder == blockSizeOrder);
        kassert(block->IsListMember(&blockLRUList));
        const size_t removedBlockCount = ownerCache->m_BlockMap.erase(block->m_bufferNumber);
        kassert(removedBlockCount == 1);
        blockLRUList.Remove(block);

        block->m_BlockCache = nullptr;
        block->m_bufferNumber = 0;
        block->m_Flags = 0;
        kassert(buffer->m_UsedBlockCount != 0);
        --buffer->m_UsedBlockCount;
    }
    kassert(buffer->m_UsedBlockCount == 0);
    ReleaseBuffer(buffer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KCacheBlockHeader* KBlockCache::AllocateBlock(size_t blockSize, size_t blockSizeOrder)
{
    kassert(s_Mutex.IsLocked());

    PIntrusiveList<KCacheBlockHeader>& blockLRUList = s_BlockLRULists[blockSizeOrder];
    for (;;)
    {
        KCacheBlockHeader* block = s_FreeBlockLists[blockSizeOrder].GetLast();
        if (block != nullptr)
        {
            s_FreeBlockLists[blockSizeOrder].Remove(block);
            KCacheBuffer* buffer = GetBlockBuffer(block);
            kassert(buffer->m_BlockSize == blockSize);
            ++buffer->m_UsedBlockCount;
            return block;
        }

        KCacheBuffer* freeBuffer = s_FreeBufferList.GetLast();
        if (freeBuffer != nullptr)
        {
            ConfigureBuffer(freeBuffer, blockSize, blockSizeOrder);
            continue;
        }

        KCacheBlockHeader* sameSizeCandidate = nullptr;
        bool sameSizeCandidateIsFlushing = false;
        for (KCacheBlockHeader* candidate : blockLRUList)
        {
            kassert(candidate->m_BlockCache != nullptr);
            kassert(GetBlockBuffer(candidate)->m_BlockSize == blockSize);
            if (candidate->m_UseCount == 0)
            {
                sameSizeCandidateIsFlushing = candidate->IsFlushing();
                if (!sameSizeCandidateIsFlushing) {
                    sameSizeCandidate = candidate;
                }
                break;
            }
        }

        const bool hasSameSizeCandidate = sameSizeCandidate != nullptr || sameSizeCandidateIsFlushing;
        bool shouldWaitForFlushing = false;
        KCacheBuffer* reclaimableBuffer = nullptr;
        KCacheBuffer* oldestBuffer = s_BufferLRUList.GetFirst();
        kassert(oldestBuffer != nullptr);
        if (!hasSameSizeCandidate || oldestBuffer->m_BlockSize != blockSize) {
            reclaimableBuffer = FindReclaimableBuffer(blockSize, hasSameSizeCandidate, shouldWaitForFlushing);
        }
        if (reclaimableBuffer != nullptr)
        {
            bool needsWriteback = false;
            for (size_t blockIndex = 0; blockIndex < reclaimableBuffer->m_BlockCount; ++blockIndex)
            {
                KCacheBlockHeader* candidate = &reclaimableBuffer->m_Blocks[blockIndex];
                if (candidate->m_BlockCache != nullptr && candidate->IsDirty())
                {
                    candidate->SetFlushRequested(true);
                    needsWriteback = true;
                }
            }
            if (needsWriteback)
            {
                s_FlushingRequestConditionVar.WakeupAll();
                s_FlushingDoneConditionVar.Wait(s_Mutex);
                continue;
            }

            ReclaimBuffer(reclaimableBuffer);
            ConfigureBuffer(reclaimableBuffer, blockSize, blockSizeOrder);
            continue;
        }

        if (shouldWaitForFlushing || sameSizeCandidateIsFlushing)
        {
            s_FlushingDoneConditionVar.Wait(s_Mutex);
            continue;
        }

        if (sameSizeCandidate != nullptr)
        {
            if (sameSizeCandidate->IsDirty())
            {
                sameSizeCandidate->SetFlushRequested(true);
                s_FlushingRequestConditionVar.WakeupAll();
                s_FlushingDoneConditionVar.Wait(s_Mutex);
                continue;
            }
            ReuseBlock(sameSizeCandidate);
            return sameSizeCandidate;
        }

        s_FlushingDoneConditionVar.Wait(s_Mutex);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::DetachBlocks(bool waitForBusyBlocks, bool discardDirtyBlocks)
{
    kassert(s_Mutex.IsLocked());

    for (;;)
    {
        bool hasBusyBlocks = m_ReadOnlySignalInProgress;
        for (const auto& blockEntry : m_BlockMap)
        {
            KCacheBlockHeader* block = blockEntry.second;
            kassert(block->m_BlockCache == this);

            if (block->m_UseCount != 0 || block->IsFlushing())
            {
                hasBusyBlocks = true;
                break;
            }
        }

        if (!hasBusyBlocks) {
            break;
        }
        if (!waitForBusyBlocks)
        {
            kernel_log<PLogSeverity::ERROR>(LogCatKernel_BlockCache, "KBlockCache::DetachBlocks() device {} still has blocks in use.", m_Device);
            return false;
        }
        s_FlushingDoneConditionVar.Wait(s_Mutex);
    }

    const size_t dirtyBlockCount = m_DirtyBlockCount.load(std::memory_order_relaxed);
    if (dirtyBlockCount != 0 && !discardDirtyBlocks)
    {
        kernel_log<PLogSeverity::ERROR>(
            LogCatKernel_BlockCache,
            "KBlockCache::DetachBlocks() device {} still has {} dirty blocks.",
            m_Device,
            dirtyBlockCount);
        return false;
    }
    if (dirtyBlockCount != 0)
    {
        kernel_log<PLogSeverity::CRITICAL>(
            LogCatKernel_BlockCache,
            "KBlockCache::DetachBlocks() discarding {} dirty blocks from device {}.",
            dirtyBlockCount,
            m_Device);
    }

    while (!m_BlockMap.empty())
    {
        KCacheBlockHeader* block = m_BlockMap.begin()->second;
        if (block->IsDirty()) {
            block->SetDirty(false);
        }
        ReleaseBlock(block);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::AbandonBlockWriteback(KCacheBlockHeader* block)
{
    kassert(s_Mutex.IsLocked());
    kassert(block->m_BlockCache == this);

    block->SetDirty(false);
    block->ClearDirtyPending();
    block->SetFlushRequested(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::DiscardBlock(KCacheBlockHeader* block)
{
    kassert(s_Mutex.IsLocked());
    kassert(block->m_BlockCache == this);

    block->SetDirty(false);
    block->ClearDirtyPending();
    block->SetFlushRequested(false);
    block->SetDiscardRequested(true);

    if (block->m_UseCount == 0 && !block->IsFlushing()) {
        ReleaseBlock(block);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::ReleaseBlock(KCacheBlockHeader* block)
{
    kassert(s_Mutex.IsLocked());
    kassert(block->m_BlockCache == this);
    kassert(block->m_UseCount == 0);
    kassert(!block->IsFlushing());
    kassert(!block->IsDirty());

    const size_t removedBlockCount = m_BlockMap.erase(block->m_bufferNumber);
    kassert(removedBlockCount == 1);
    kassert(block->IsListMember(&s_BlockLRULists[m_BlockSizeOrder]));
    s_BlockLRULists[m_BlockSizeOrder].Remove(block);
    block->m_BlockCache = nullptr;
    block->m_bufferNumber = 0;
    block->m_Flags = 0;
    ReleaseUnusedBlock(block);
    s_FlushingDoneConditionVar.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::SetReadOnlyAfterWriteError(PErrorCode error)
{
    kassert(s_Mutex.IsLocked());

    const bool wasReadOnly = m_IsReadOnly.exchange(true, std::memory_order_relaxed);
    if (m_WriteError == PErrorCode::Success) {
        m_WriteError = error;
    }
    if (!wasReadOnly)
    {
        m_ReadOnlySignalPending = true;
        ++s_PendingReadOnlySignalCount;
        kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_BlockCache, "Block cache for device {} became read-only after a write failure: {}.", m_Device, p_strerror(error));
    }

    for (const auto& blockEntry : m_BlockMap)
    {
        KCacheBlockHeader* block = blockEntry.second;
        if (block->IsDirty()) {
            block->SetFlushRequested(true);
        }
    }
    s_FlushingRequestConditionVar.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::FlushInternal()
{
    kassert(s_Mutex.IsLocked());

    if (m_DirtyBlockCount.load(std::memory_order_relaxed) != 0)
    {
        for (const auto& blockEntry : m_BlockMap)
        {
            KCacheBlockHeader* block = blockEntry.second;
            if (block->IsDirty()) {
                block->SetFlushRequested(true);
            }
        }
        s_FlushingRequestConditionVar.WakeupAll();
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::CompareCacheBlockOrder(const KCacheBlockHeader* lhs, const KCacheBlockHeader* rhs)
{
    if (lhs->m_BlockCache != rhs->m_BlockCache) {
        return std::less<KBlockCache*>()(lhs->m_BlockCache, rhs->m_BlockCache);
    }
    return lhs->m_bufferNumber < rhs->m_bufferNumber;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::Flush()
{
    CRITICAL_SCOPE(KBlockCache::s_Mutex);

    return FlushInternal() && m_WriteError == PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::Sync()
{
    CRITICAL_SCOPE(KBlockCache::s_Mutex);

    FlushInternal();

    while (m_DirtyBlockCount.load(std::memory_order_relaxed) != 0) {
        s_FlushingDoneConditionVar.Wait(s_Mutex);
    }
    return m_WriteError == PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KCacheBlockHeader::AddRef()
{
    kassert(KBlockCache::s_Mutex.IsLocked());
    m_UseCount++;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KCacheBlockHeader::RemoveRef()
{
    kassert(KBlockCache::s_Mutex.IsLocked());
    m_UseCount--;
    if (m_UseCount == 0)
    {
        if (IsDiscardRequested() && !IsFlushing()) {
            m_BlockCache->ReleaseBlock(this);
        } else {
            KBlockCache::s_FlushingDoneConditionVar.WakeupAll();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KCacheBlockHeader::SetDirty(bool isDirty)
{
    kassert(KBlockCache::s_Mutex.IsLocked());
    kassert(m_BlockCache != nullptr);

    if (isDirty)
    {
        if (m_BlockCache->IsReadOnly())
        {
            m_BlockCache->DiscardBlock(this);
            return false;
        }
        if ((m_Flags & BCF_DIRTY) == 0)
        {
            m_Flags |= BCF_DIRTY;
            ++KBlockCache::s_DirtyBlockCount;
            KBlockCache::s_DirtyByteCount.fetch_add(m_BlockCache->m_BlockSize, std::memory_order_relaxed);
            m_BlockCache->m_DirtyBlockCount.fetch_add(1, std::memory_order_relaxed);

            m_DirtyTime = kget_monotonic_time();
            if (KBlockCache::s_DirtyBlockCount == 1) {
                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_BlockCache, "Cache dirty.");
            }
        }
        else
        {
            m_Flags |= BCF_DIRTY_PENDING;
        }
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_BlockCache, "Block {} from device {} dirty.", m_bufferNumber, m_BlockCache->m_Device);
        if (KBlockCache::s_DirtyByteCount.load(std::memory_order_relaxed) >= KBlockCache::MIN_FLUSH_WAKEUP_SIZE) {
            KBlockCache::s_FlushingRequestConditionVar.WakeupAll();
        }
    }
    else
    {
        if (m_Flags & BCF_DIRTY)
        {
            m_Flags &= ~BCF_DIRTY;
            kassert(m_BlockCache->m_DirtyBlockCount.load(std::memory_order_relaxed) != 0);
            kassert(KBlockCache::s_DirtyByteCount.load(std::memory_order_relaxed) >= m_BlockCache->m_BlockSize);
            --KBlockCache::s_DirtyBlockCount;
            KBlockCache::s_DirtyByteCount.fetch_sub(m_BlockCache->m_BlockSize, std::memory_order_relaxed);
            m_BlockCache->m_DirtyBlockCount.fetch_sub(1, std::memory_order_relaxed);
            if (KBlockCache::s_DirtyBlockCount == 0) {
                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_BlockCache, "Cache clean.");
            }
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KCacheBlockDesc::~KCacheBlockDesc()
{
    if (m_Block != nullptr)
    {
        CRITICAL_SCOPE(KBlockCache::s_Mutex);
        m_Block->RemoveRef();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KCacheBlockDesc& KCacheBlockDesc::operator=(KCacheBlockDesc&& src)
{
    if (m_Block != nullptr)
    {
        CRITICAL_SCOPE(KBlockCache::s_Mutex);
        m_Block->RemoveRef();
    }
    m_Block = src.m_Block;
    m_Buffer = src.m_Buffer;
    src.m_Block = nullptr;
    src.m_Buffer = nullptr;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KCacheBlockDesc::MarkDirty()
{
    if (m_Block != nullptr)
    {
        CRITICAL_SCOPE(KBlockCache::s_Mutex);
        if (!m_Block->SetDirty(true)) {
            PERROR_THROW_CODE(PErrorCode::ROFS);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KCacheBlockDesc::Reset()
{
    if (m_Block != nullptr)
    {
        CRITICAL_SCOPE(KBlockCache::s_Mutex);
        m_Block->RemoveRef();
        m_Block = nullptr;
        m_Buffer = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::FlushBlockList(KCacheBlockHeader** blockList, size_t blockCount)
{
#ifdef PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
    return FlushBlockListWithDiagnostics(blockList, blockCount);
#endif // PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_BlockCache, "KBlockCache::FlushBlockList() flushing {} blocks.", blockCount);

    std::sort(blockList, blockList + blockCount, CompareCacheBlockOrder);

    static iovec_t segments[MAX_FLUSH_BLOCK_COUNT];
    const TimeValNanos currentTime = kget_monotonic_time();
    bool anythingProcessed = false;
    size_t start = 0;
    while (start < blockCount)
    {
        KBlockCache* blockCache = blockList[start]->m_BlockCache;
        kassert(blockCache != nullptr);

        size_t end = start + 1;
        while (end < blockCount &&
            blockList[end]->m_BlockCache == blockCache &&
            blockList[end - 1]->m_bufferNumber + 1 == blockList[end]->m_bufferNumber) {
            ++end;
        }

        bool requiredSegment = false;
        bool hasTimedOutBlocks = false;
        for (size_t blockIndex = start; blockIndex < end; ++blockIndex)
        {
            requiredSegment = requiredSegment || blockList[blockIndex]->IsFlushRequested();
            hasTimedOutBlocks =
                hasTimedOutBlocks || (currentTime - blockList[blockIndex]->m_DirtyTime) >= FLUSH_PERIOD;
        }

        const size_t segmentCount = end - start;
        const size_t blockSize = blockCache->m_BlockSize;
        if (requiredSegment || hasTimedOutBlocks || segmentCount * blockSize >= MIN_FLUSH_SIZE)
        {
            for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
            {
                KCacheBlockHeader* block = blockList[start + segmentIndex];
                block->ClearDirtyPending();
                segments[segmentIndex].iov_base = block->m_Buffer;
                segments[segmentIndex].iov_len = blockSize;
            }

            PErrorCode writeError = PErrorCode::Success;
            s_Mutex.Unlock();
            try
            {
                const size_t bytesWritten = kpwritev_trw(
                    blockCache->m_Device,
                    segments,
                    segmentCount,
                    blockList[start]->m_bufferNumber * off64_t(blockSize));
                if (bytesWritten != segmentCount * blockSize) {
                    PERROR_THROW_CODE(PErrorCode::IO);
                }
            }
            PERROR_CATCH(([&writeError](PErrorCode error)
                {
                    writeError = error;
                }
            ));
            s_Mutex.Lock();
            anythingProcessed = true;

            if (writeError == PErrorCode::Success)
            {
                for (size_t blockIndex = start; blockIndex < end; ++blockIndex)
                {
                    if (!blockList[blockIndex]->IsDirtyPending())
                    {
                        blockList[blockIndex]->SetDirty(false);
                        blockList[blockIndex]->SetFlushRequested(false);
                    }
                }
            }
            else
            {
                blockCache->SetReadOnlyAfterWriteError(writeError);
                for (size_t blockIndex = start; blockIndex < end; ++blockIndex) {
                    blockCache->AbandonBlockWriteback(blockList[blockIndex]);
                }
            }
        }
        start = end;
    }
    return anythingProcessed;
}
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KBlockCache::DiskCacheFlusher(void* arg)
{
    for (;;)
    {
        KBlockCache* notificationCache = nullptr;
        PErrorCode notificationError = PErrorCode::Success;
        bool anythingProcessed = false;

        KVFSManager::FlushInodes();
        CRITICAL_BEGIN(s_Mutex)
        {
            try
            {
                if (s_DirtyBlockCount > 0)
                {
                    static KCacheBlockHeader* blockList[MAX_FLUSH_BLOCK_COUNT];
                    size_t blocksFlushed = 0;
                    KBlockCache* targetCache = nullptr;
                    size_t selectedBlockSizeOrder = BLOCK_SIZE_ORDER_COUNT;
                    for (size_t blockSizeOffset = 0;
                        blockSizeOffset < BLOCK_SIZE_ORDER_COUNT && targetCache == nullptr;
                        ++blockSizeOffset)
                    {
                        const size_t blockSizeOrder =
                            (s_NextFlushBlockSizeOrder + blockSizeOffset) % BLOCK_SIZE_ORDER_COUNT;
                        PIntrusiveList<KCacheBlockHeader>& blockLRUList = s_BlockLRULists[blockSizeOrder];
                        for (auto block = blockLRUList.begin();
                            block != blockLRUList.end() && blocksFlushed < MAX_FLUSH_BLOCK_COUNT;
                            ++block)
                        {
                            kassert(block->m_BlockCache != nullptr);
                            kassert(block->m_BlockCache->m_BlockSizeOrder == blockSizeOrder);
                            if (block->IsDirty() && !block->IsFlushing())
                            {
                                if (targetCache == nullptr)
                                {
                                    targetCache = block->m_BlockCache;
                                    selectedBlockSizeOrder = blockSizeOrder;
                                }
                                if (block->m_BlockCache == targetCache)
                                {
                                    block->SetIsFlushing(true);
                                    blockList[blocksFlushed++] = *block;
                                }
                            }
                        }
                    }
                    if (selectedBlockSizeOrder != BLOCK_SIZE_ORDER_COUNT) {
                        s_NextFlushBlockSizeOrder = (selectedBlockSizeOrder + 1) % BLOCK_SIZE_ORDER_COUNT;
                    }
                    {
                        PScopeExit finishFlushing([&blocksFlushed]()
                        {
                            for (size_t blockIndex = 0; blockIndex < blocksFlushed; ++blockIndex)
                            {
                                KCacheBlockHeader* block = blockList[blockIndex];
                                block->SetIsFlushing(false);
                                if (block->IsDiscardRequested() && block->m_UseCount == 0) {
                                    block->m_BlockCache->ReleaseBlock(block);
                                }
                            }
                            s_FlushingDoneConditionVar.WakeupAll();
                        });

                        anythingProcessed = FlushBlockList(blockList, blocksFlushed);
                    }
                }

                if (s_PendingReadOnlySignalCount != 0)
                {
                    for (const auto& deviceEntry : s_DeviceMap)
                    {
                        KBlockCache* blockCache = deviceEntry.second;
                        if (blockCache->m_ReadOnlySignalPending && blockCache->m_DirtyBlockCount.load(std::memory_order_relaxed) == 0)
                        {
                            blockCache->m_ReadOnlySignalPending = false;
                            blockCache->m_ReadOnlySignalInProgress = true;
                            --s_PendingReadOnlySignalCount;
                            notificationCache = blockCache;
                            notificationError = blockCache->m_WriteError;
                            break;
                        }
                    }
                }

                if (!anythingProcessed && notificationCache == nullptr) {
                    s_FlushingRequestConditionVar.WaitTimeout(s_Mutex, FLUSH_PERIOD);
                }
            }
            PERROR_CATCH(([](PErrorCode error) { kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_BlockCache, "Exception caught during disk cache flushing."); }));
        } CRITICAL_END;

        if (notificationCache != nullptr)
        {
            notificationCache->SignalBecameReadOnly(notificationError);

            CRITICAL_SCOPE(s_Mutex);
            notificationCache->m_ReadOnlySignalInProgress = false;
            s_FlushingDoneConditionVar.WakeupAll();
        }
    }
}

size_t kget_dirty_disk_cache_blocks()
{
    return KBlockCache::GetDirtyBlockCount();
}

} // namespace kernel
