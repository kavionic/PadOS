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
#include <functional>
#include <map>

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

#ifdef PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
static constexpr int KBLOCK_CACHE_BLOCK_COUNT = 4080;
#else
static constexpr int KBLOCK_CACHE_BLOCK_COUNT = 4096;
#endif // PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS

static uint8_t* gk_BCacheBuffer;
static KCacheBlockHeader gk_BCacheHeaders[KBLOCK_CACHE_BLOCK_COUNT];

std::map<int, KBlockCache*>         KBlockCache::s_DeviceMap;
PIntrusiveList<KCacheBlockHeader>   KBlockCache::s_FreeList;
PIntrusiveList<KCacheBlockHeader>   KBlockCache::s_MRUList;
KMutex                              KBlockCache::s_Mutex("bcache_mutex", PEMutexRecursionMode_RaiseError);
KConditionVariable                  KBlockCache::s_FlushingRequestConditionVar("bcache_flush_req");
KConditionVariable                  KBlockCache::s_FlushingDoneConditionVar("bcache_flush_done");
std::atomic_int                     KBlockCache::s_DirtyBlockCount;
size_t                              KBlockCache::s_PendingReadOnlySignalCount;


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

    if (m_Device == -1) {
        return true;
    }

    s_DeviceMap[m_Device] = this;
    
    m_BlockCount = blockCount;
    m_BlockSize  = blockSize;
    
    switch(m_BlockSize)
    {
        case 512:
//            m_BlocksPerBuffer = 8;
//            m_BlockToBufferShift = 3;
//            m_BufferOffsetMask = 0x07;
            m_BlocksPerBuffer = 1;
            m_BlockToBufferShift = 0;
            m_BufferOffsetMask = 0x00;
            break;
        case 1024:
            m_BlocksPerBuffer = 4;
            m_BlockToBufferShift = 2;
            m_BufferOffsetMask = 0x03;
            break;
        case 2048:
            m_BlocksPerBuffer = 2;
            m_BlockToBufferShift = 1;
            m_BufferOffsetMask = 0x01;
            break;
        case 4096:
            m_BlocksPerBuffer = 1;
            m_BlockToBufferShift = 0;
            m_BufferOffsetMask = 0x00;
            break;
        default:
            m_BlocksPerBuffer = 1;
            m_BlockToBufferShift = 0;
            m_BufferOffsetMask = 0x00;
            return false;
    }
    return true;
}

void KBlockCache::Initialize()
{
    size_t bufferSize = KBlockCache::BUFFER_BLOCK_SIZE * KBLOCK_CACHE_BLOCK_COUNT;
#ifdef PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
    bufferSize += GetFlushDiagnosticBufferSize();
#endif // PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS

    gk_BCacheBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, bufferSize));
#ifdef PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
    InitializeFlushDiagnostics(gk_BCacheBuffer + KBlockCache::BUFFER_BLOCK_SIZE * KBLOCK_CACHE_BLOCK_COUNT);
#endif // PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS

    uint8_t* buffer = gk_BCacheBuffer;
    for (int i = 0; i < KBLOCK_CACHE_BLOCK_COUNT; ++i)
    {
        gk_BCacheHeaders[i].m_Buffer = buffer;
        buffer += BUFFER_BLOCK_SIZE;
        s_FreeList.Append(&gk_BCacheHeaders[i]);
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
//    ProfileTimer pt("GetBlock", 2.0e-3);
    CRITICAL_SCOPE(s_Mutex);
    
    
    doLoad = true; // Until we properly handle partially loaded blocks.
    
    off64_t bufferNum   = blockNum >> m_BlockToBufferShift;
    size_t  blockOffset = size_t((blockNum & m_BufferOffsetMask) * m_BlockSize);
    
    for (int retry = 0; retry < 10; ++retry)
    {
        auto i = m_BlockMap.find(bufferNum);
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
            if (s_MRUList.GetLast() != block)
            {
                s_MRUList.Remove(block);
                s_MRUList.Append(block);
            }
            return KCacheBlockDesc(block, blockOffset);
        }

        KCacheBlockHeader* block = nullptr;

        if (s_FreeList.GetLast() != nullptr)
        {
            block = s_FreeList.GetLast();
            s_FreeList.Remove(block);
        }
        else if (s_MRUList.GetFirst() != nullptr)
        {
            for (auto i : s_MRUList)
            {
                if (i->m_UseCount == 0 /*&& !i->IsFlushing() && !i->IsDirty()*/)
                {
                    block = i;
                    break;
                }
            }
            if (block == nullptr)
            {
                s_FlushingRequestConditionVar.WakeupAll();
                s_FlushingDoneConditionVar.Wait(s_Mutex);
                continue;
            }
            if (block->IsFlushing())
            {
                s_FlushingDoneConditionVar.Wait(s_Mutex);
                continue;
            }
            if (block->IsDirty())
            {
                block->SetFlushRequested(true);
                s_FlushingRequestConditionVar.WakeupAll();
                s_FlushingDoneConditionVar.Wait(s_Mutex);
                continue;
            }

            KBlockCache* ownerCache = block->m_BlockCache;
            kassert(ownerCache != nullptr);
            s_MRUList.Remove(block);
            const size_t removedBlockCount = ownerCache->m_BlockMap.erase(block->m_bufferNumber);
            kassert(removedBlockCount == 1);
            block->m_BlockCache = nullptr;
        }
        else
        {
            kernel_log<PLogSeverity::ERROR>(LogCatKernel_BlockCache, "KBlockCache::GetBlock() all cache blocks locked!");
            PERROR_THROW_CODE(PErrorCode::AGAIN);
        }
        kassert(block != nullptr);
        if (block == nullptr) {
            PERROR_THROW_CODE(PErrorCode::AGAIN);
        }
        kassert(block->m_BlockCache == nullptr);
        PScopeFail contextCleanup([&block]() { s_FreeList.Append(block); });
        if (doLoad)
        {
            const ssize_t bytesRead = kpread_trw(m_Device, block->m_Buffer, BUFFER_BLOCK_SIZE, bufferNum * BUFFER_BLOCK_SIZE);
            if (bytesRead != BUFFER_BLOCK_SIZE) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }
        }
        block->m_BlockCache   = this;
        block->m_bufferNumber = bufferNum;
        block->m_UseCount     = 1;
        block->m_Flags        = 0;
        m_BlockMap[bufferNum] = block;
        s_MRUList.Append(block);
                
//                kernel_log<PLogSeverity::ERROR>(LogCatKernel_BlockCache, "Block {} read.", bufferNum);
                
        return KCacheBlockDesc(block, blockOffset);
    }
    kernel_log<PLogSeverity::ERROR>(LogCatKernel_BlockCache, "KBlockCache::GetBlock() to many retries. All blocks stuck in busy state.");
    PERROR_THROW_CODE(PErrorCode::AGAIN);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KBlockCache::MarkBlockDirty(off64_t blockNum)
{
    CRITICAL_SCOPE(s_Mutex);

    off64_t bufferNum   = blockNum >> m_BlockToBufferShift;
//    size_t  blockOffset = (blockNum & m_BufferOffsetMask) * m_BlockSize;
    
    auto i = m_BlockMap.find(bufferNum);
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
//            kernel_log<PLogSeverity::ERROR>(LogCatKernel_BlockCache, "Block {} written", blockNum + i);
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

    for (const auto& blockEntry : m_BlockMap)
    {
        KCacheBlockHeader* block = blockEntry.second;
        if (block->IsDirty()) {
            block->SetDirty(false);
        }
        s_MRUList.Remove(block);
        block->m_BlockCache = nullptr;
        block->m_bufferNumber = 0;
        block->m_Flags = 0;
        s_FreeList.Append(block);
    }
    m_BlockMap.clear();
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
    s_MRUList.Remove(block);
    block->m_BlockCache = nullptr;
    block->m_bufferNumber = 0;
    block->m_Flags = 0;
    s_FreeList.Append(block);
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
            if (block->IsDirty() /*&& !block->IsFlushing()*/) {
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
        if (KBlockCache::s_DirtyBlockCount >= KBlockCache::MIN_FLUSH_WAKEUP_BLOCK_COUNT) {
            KBlockCache::s_FlushingRequestConditionVar.WakeupAll();
        }
    }
    else
    {
        if (m_Flags & BCF_DIRTY)
        {
            m_Flags &= ~BCF_DIRTY;
            kassert(m_BlockCache->m_DirtyBlockCount.load(std::memory_order_relaxed) != 0);
            --KBlockCache::s_DirtyBlockCount;
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

//    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_BlockCache, "Flush {} blocks.", blockCount);

    TimeValNanos curTime = kget_monotonic_time();

    size_t start = 0;
    bool    requiredSegment = false;
    bool    hasTimedOutBlocks = false;

    bool anythingProcessed = false;
    for (size_t i = 0; i <= blockCount; ++i)
    {
//        if (i < blockCount)
//        {
//            kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_BlockCache, "    {}", blockList[i]->m_bufferNumber);
//        }

        if (i < blockCount)
        {
            if (!requiredSegment && blockList[i]->IsFlushRequested()) {
                requiredSegment = true;
            }
            if (!requiredSegment && (curTime - blockList[i]->m_DirtyTime) >= FLUSH_PERIOD) {
                hasTimedOutBlocks = true;
            }
        }
        if (
            i == blockCount ||
            (i > start &&
                (blockList[i - 1]->m_BlockCache != blockList[i]->m_BlockCache ||
                    blockList[i - 1]->m_bufferNumber + 1 != blockList[i]->m_bufferNumber)))
        {
            const size_t segmentCount = i - start;
            if (requiredSegment || hasTimedOutBlocks || segmentCount >= MIN_FLUSH_BLOCK_COUNT)
            {
                KBlockCache* blockCache = blockList[start]->m_BlockCache;
                kassert(blockCache != nullptr);
                const int device = blockCache->m_Device;

                for (size_t blockIndex = start; blockIndex < i; ++blockIndex) {
                    blockList[blockIndex]->ClearDirtyPending();
                }

//                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_BlockCache, "  {}:{}", blockList[start]->m_bufferNumber, segmentCount);

                PErrorCode writeError = PErrorCode::Success;
                s_Mutex.Unlock();
                try
                {
                    const ssize_t bytesWritten = kpwritev_trw(
                        device,
                        segments,
                        segmentCount,
                        blockList[start]->m_bufferNumber * KBlockCache::BUFFER_BLOCK_SIZE);
                    if (bytesWritten != segmentCount * KBlockCache::BUFFER_BLOCK_SIZE) {
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
                    for (size_t blockIndex = start; blockIndex < i; ++blockIndex)
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
                    for (size_t blockIndex = start; blockIndex < i; ++blockIndex) {
                        blockCache->AbandonBlockWriteback(blockList[blockIndex]);
                    }
                }
            }
            start = i;
            requiredSegment = false;
            hasTimedOutBlocks = false;
        }
        if (i < blockCount)
        {
            segments[i - start].iov_base = blockList[i]->m_Buffer;
            segments[i - start].iov_len = KBlockCache::BUFFER_BLOCK_SIZE;
        }
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
                    for (auto block = s_MRUList.begin(); block != s_MRUList.end() && blocksFlushed < MAX_FLUSH_BLOCK_COUNT && s_DirtyBlockCount > 0; ++block)
                    {
                        if (block->IsDirty() && !block->IsFlushing())
                        {
                            if (targetCache == nullptr) {
                                targetCache = block->m_BlockCache;
                            }
                            if (block->m_BlockCache == targetCache)
                            {
                                block->SetIsFlushing(true);
                                blockList[blocksFlushed++] = *block;
                            }
                        }
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
