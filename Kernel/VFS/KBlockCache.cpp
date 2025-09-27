// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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

#include <sys/pados_syscalls.h>
#include <malloc.h>
#include <string.h>

#include <map>

#include "Kernel/VFS/KBlockCache.h"
#include "Kernel/VFS/FileIO.h"
#include "Kernel/VFS/KVFSManager.h"
#include <Utils/Utils.h>

using namespace kernel;
using namespace os;

static constexpr TimeValMicros FLUSH_PERIODE = TimeValMicros::FromMilliseconds(1000);
static constexpr int KBLOCK_CACHE_BLOCK_COUNT = 64;
static constexpr int BC_FLUSH_COUNT         = 64;
static constexpr int BC_MIN_WAKEUP_COUNT    = 32;
static constexpr int BC_MIN_FLUSH_COUNT = 48;

static uint8_t* gk_BCacheBuffer;
static KCacheBlockHeader gk_BCacheHeaders[KBLOCK_CACHE_BLOCK_COUNT];

std::map<int, KBlockCache*>         KBlockCache::s_DeviceMap;
IntrusiveList<KCacheBlockHeader>    KBlockCache::s_FreeList;
IntrusiveList<KCacheBlockHeader>    KBlockCache::s_MRUList;
KMutex                              KBlockCache::s_Mutex("bcache_mutex", PEMutexRecursionMode_RaiseError);
KConditionVariable                  KBlockCache::s_FlushingRequestConditionVar("bcache_flush_req");
KConditionVariable                  KBlockCache::s_FlushingDoneConditionVar("bcache_flush_done");
std::atomic_int                     KBlockCache::s_DirtyBlockCount;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KBlockCache::KBlockCache() : m_Device(-1), m_BlockSize(0), m_BlockCount(0), m_BlocksPerBuffer(1), m_BlockToBufferShift(0), m_BufferOffsetMask(0x00)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KBlockCache::~KBlockCache()
{
    Flush();
    SetDevice(-1, 0, 0);
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
        kprintf("ERROR: KBlockCache::GetDeviceCache() device %d not registered!\n", device);
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::SetDevice(int device, off64_t blockCount, size_t blockSize)
{
    if (m_Device != -1 )
    {
        auto i = s_DeviceMap.find(m_Device);
        if (i != s_DeviceMap.end()) {
            s_DeviceMap.erase(i);
        } else {
            kprintf("ERROR: KBlockCache::SetDevice() previous device %d not registered!\n", m_Device);
        }
        m_Device = -1;
    }
    if (s_DeviceMap.find(device) != s_DeviceMap.end()) {
        kprintf("ERROR: KBlockCache::SetDevice() device %d already registered!\n", device);
        return false;        
    }
    m_Device     = device;
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::Initialize()
{
    gk_BCacheBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, KBlockCache::BUFFER_BLOCK_SIZE * KBLOCK_CACHE_BLOCK_COUNT));
    uint8_t* buffer = gk_BCacheBuffer;
    for (int i = 0; i < KBLOCK_CACHE_BLOCK_COUNT; ++i)
    {
        gk_BCacheHeaders[i].m_Buffer = buffer;
        buffer += BUFFER_BLOCK_SIZE;
        s_FreeList.Append(&gk_BCacheHeaders[i]);
    }
    PThreadAttribs attrs("disk_cache_flusher", 0, PThreadDetachState_Detached, 4096);
    sys_thread_spawn(nullptr, &attrs, DiskCacheFlusher, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KCacheBlockDesc KBlockCache::GetBlock(off64_t blockNum, bool doLoad)
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
            block->AddRef();
            return KCacheBlockDesc(block, blockOffset);
        }
        else
        {
            KCacheBlockHeader* block = nullptr;
            if (s_FreeList.m_Last != nullptr)
            {
                block = s_FreeList.m_Last;
                s_FreeList.Remove(block);
            }
            else if (s_MRUList.m_First != nullptr)
            {
                for (block = s_MRUList.m_First; block != nullptr && (block->m_UseCount != 0 /*|| block->IsFlushing() || block->IsDirty()*/); block = block->m_Next) /*EMPTY*/;
                if (block == nullptr) {
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
                s_MRUList.Remove(block);
                auto i = m_BlockMap.find(block->m_bufferNumber);
                if (i != m_BlockMap.end()) {
                    m_BlockMap.erase(i);
                }
            }
            else
            {
                printf("ERROR: KBlockCache::GetBlock() all cache blocks locked!\n");
                return KCacheBlockDesc();
            }
            if (block != nullptr)
            {
                if (doLoad)
                {
                    if (FileIO::Read(m_Device, bufferNum * BUFFER_BLOCK_SIZE, block->m_Buffer, BUFFER_BLOCK_SIZE) < 0) {
                        printf("ERROR: KBlockCache::GetBlock() failed to read block %" PRIu64 " from disk: %s\n", bufferNum * BUFFER_BLOCK_SIZE, strerror(get_last_error()));
                        s_FreeList.Append(block);
                        return KCacheBlockDesc();
                    }
                }
                s_MRUList.Append(block);
                block->m_UseCount     = 1;
                block->m_Flags        = 0;
                block->m_Device       = m_Device;
                block->m_bufferNumber = bufferNum;
                m_BlockMap[bufferNum] = block;
                
//                kprintf("Block %" PRIu64 " read\n", bufferNum);
                
                return KCacheBlockDesc(block, blockOffset);
            }
            return KCacheBlockDesc();
        }
    }
    printf("ERROR: KBlockCache::GetBlock() to many retries. All blocks stuck in busy state.\n");
    return KCacheBlockDesc();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::MarkBlockDirty(off64_t blockNum)
{
    CRITICAL_SCOPE(s_Mutex);

    off64_t bufferNum   = blockNum >> m_BlockToBufferShift;
//    size_t  blockOffset = (blockNum & m_BufferOffsetMask) * m_BlockSize;
    
    auto i = m_BlockMap.find(bufferNum);
    if (i != m_BlockMap.end())
    {
        KCacheBlockHeader* block = i->second;
        block->SetDirty(true);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int  KBlockCache::CachedRead(off64_t blockNum, void* buffer, size_t blockCount)
{
    for (size_t i = 0 ; i < blockCount ; ++i)
    {
        KCacheBlockDesc block = GetBlock(blockNum + i, true);

        if (block.m_Buffer != nullptr) {
            memcpy(reinterpret_cast<uint8_t*>(buffer) + i * m_BlockSize, block.m_Buffer, m_BlockSize);
        } else {
            printf( "KBlockCache::CachedRead() failed to find block %d / %" PRIu64 "\n", m_Device, blockNum + i);
            return -1;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KBlockCache::CachedWrite(off64_t blockNum, const void* buffer, size_t blockCount)
{
    for (size_t i = 0 ; i < blockCount ; ++i)
    {
        KCacheBlockDesc block = GetBlock(blockNum + i, false);
//        KCacheBlockDesc block = GetBlock(blockNum + i, true);

        if (block.m_Buffer != nullptr)
        {
//            kprintf("Block %" PRIu64 " written\n", blockNum + i);
            memcpy(block.m_Buffer, reinterpret_cast<const uint8_t*>(buffer) + i * m_BlockSize, m_BlockSize);
            CRITICAL_SCOPE(s_Mutex);
            block.m_Block->SetDirty(true);
        }
        else
        {
            printf("KBlockCache::CachedWrite() failed to find block %d / %" PRIu64 "\n", m_Device, blockNum + i);
            return -1;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::FlushInternal()
{
    kassert(s_Mutex.IsLocked());

    if (s_DirtyBlockCount != 0)
    {
        for (KCacheBlockHeader* block = s_MRUList.m_First; block != nullptr; block = block->m_Next)
        {
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

bool KBlockCache::Flush()
{
    CRITICAL_SCOPE(KBlockCache::s_Mutex);

    return FlushInternal();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::Sync()
{
    CRITICAL_SCOPE(KBlockCache::s_Mutex);

    if (!FlushInternal()) {
        return false;
    }

    if (s_DirtyBlockCount != 0) {
        s_FlushingDoneConditionVar.Wait(s_Mutex);
    }
    return true;
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
    if (m_UseCount == 0) {
        KBlockCache::s_FlushingDoneConditionVar.WakeupAll();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KCacheBlockHeader::SetDirty(bool isDirty)
{
    kassert(KBlockCache::s_Mutex.IsLocked());
    if (isDirty)
    {
        if ((m_Flags & BCF_DIRTY) == 0)
        {
            m_Flags |= BCF_DIRTY;
            KBlockCache::s_DirtyBlockCount++;

            m_DirtyTime = get_system_time();
            if (KBlockCache::s_DirtyBlockCount == 1) {
                kernel_log(LogCatKernel_BlockCache, KLogSeverity::INFO_HIGH_VOL, "Cache dirty\n");
            }
        }
        else
        {
            m_Flags |= BCF_DIRTY_PENDING;
        }
        kernel_log(LogCatKernel_BlockCache, KLogSeverity::INFO_HIGH_VOL, "Block %" PRIu64 " from device %d dirty\n", m_bufferNumber, m_Device);
        if (KBlockCache::s_DirtyBlockCount >= BC_MIN_WAKEUP_COUNT) {
            KBlockCache::s_FlushingRequestConditionVar.WakeupAll();
        }
    }
    else
    {
        if (m_Flags & BCF_DIRTY)
        {
            m_Flags &= ~BCF_DIRTY;
            KBlockCache::s_DirtyBlockCount--;
            if (KBlockCache::s_DirtyBlockCount == 0) {
                kernel_log(LogCatKernel_BlockCache, KLogSeverity::INFO_HIGH_VOL, "Cache clean\n");
            }
        }
    }
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
        m_Block->SetDirty(true);
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
    kernel_log(LogCatKernel_BlockCache, KLogSeverity::INFO_HIGH_VOL, "KBlockCache::FlushBlockList() flushing %d blocks.\n", blockCount);

    std::sort(blockList, blockList + blockCount, [](const KCacheBlockHeader* lhs, const KCacheBlockHeader* rhs) { return std::tie(lhs->m_Device, lhs->m_bufferNumber) < std::tie(rhs->m_Device, rhs->m_bufferNumber); });

    static IOSegment segments[BC_FLUSH_COUNT];

//    printf("Flush %d blocks\n", blockCount);

    TimeValMicros curTime = get_system_time();

    size_t start = 0;
    bool    requiredSegment = false;
    bool    hasTimedOutBlocks = false;

    bool anythingFlushed = false;
    for (size_t i = 0; i <= blockCount; ++i)
    {
//        if (i < blockCount)
//        {
//            printf("    %" PRId64 "\n", blockList[i]->m_bufferNumber);
//        }

        if (i < blockCount)
        {
            if (!requiredSegment && blockList[i]->IsFlushRequested()) {
                requiredSegment = true;
            }
            if (!requiredSegment && (curTime - blockList[i]->m_DirtyTime) >= FLUSH_PERIODE) {
                hasTimedOutBlocks = true;
            }
        }
        if (i == blockCount || (i > start && (blockList[i-1]->m_Device != blockList[i]->m_Device || (blockList[i-1]->m_bufferNumber + 1) != blockList[i]->m_bufferNumber)))
        {
            size_t segmentCount = i - start;
            if (requiredSegment || hasTimedOutBlocks || segmentCount >= BC_MIN_FLUSH_COUNT)
            {
                for (size_t j = start; j < i; ++j) {
                    blockList[j]->ClearDirtyPending();
                }

//                printf("  %" PRId64 ":%d\n", blockList[start]->m_bufferNumber, segmentCount);

                s_Mutex.Unlock();
                if (FileIO::Write(blockList[start]->m_Device, blockList[start]->m_bufferNumber * KBlockCache::BUFFER_BLOCK_SIZE, segments, segmentCount) < 0) {
                    kernel_log(LogCatKernel_BlockCache, KLogSeverity::CRITICAL, "Failed to flush block %" PRId64 ":%d from device %d\n", blockList[start]->m_bufferNumber, segmentCount, blockList[start]->m_Device);
                }
                anythingFlushed = true;
                s_Mutex.Lock();

                for (size_t j = start; j < i; ++j)
                {
                    if (!blockList[j]->IsDirtyPending())
                    {
                        blockList[j]->SetDirty(false);
                        blockList[j]->SetFlushRequested(false);
                        blockList[j]->SetIsFlushing(false);
                    }
                }

                if (requiredSegment)
                {
                    s_FlushingDoneConditionVar.WakeupAll();
                }
            }
            start = i;
            requiredSegment = false;
            hasTimedOutBlocks = false;
        }
        if (i < blockCount)
        {
            segments[i - start].Buffer = blockList[i]->m_Buffer;
            segments[i - start].Length = KBlockCache::BUFFER_BLOCK_SIZE;
        }
    }
    return anythingFlushed;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KBlockCache::DiskCacheFlusher(void* arg)
{
    for (;;)
    {
        KVFSManager::FlushInodes();
        CRITICAL_BEGIN(s_Mutex)
        {
            if (s_DirtyBlockCount > 0)
            {
                static KCacheBlockHeader* blockList[BC_FLUSH_COUNT];
                int blocksFlushed = 0;
                int deviceID = -1;
                for (KCacheBlockHeader* block = s_MRUList.m_First; block != nullptr && blocksFlushed < BC_FLUSH_COUNT && s_DirtyBlockCount > 0; block = block->m_Next)
                {
                    if (block->IsDirty() && !block->IsFlushing())
                    {
                        if (deviceID == -1) deviceID = block->m_Device;
                        if (block->m_Device == deviceID)
                        {
                            block->SetIsFlushing(true);
                            blockList[blocksFlushed++] = block;
                        }
                    }
                }
                bool anythingFlushed = FlushBlockList(blockList, blocksFlushed);
                for (size_t i = 0; i < blocksFlushed; ++i)
                {
                    KCacheBlockHeader* block = blockList[i];
                    block->SetIsFlushing(false);
                }
                if (anythingFlushed) {
                    s_FlushingDoneConditionVar.WakeupAll();
                } else {
                    s_FlushingRequestConditionVar.WaitTimeout(s_Mutex, FLUSH_PERIODE);
                }
            }
            else
            {
                s_FlushingRequestConditionVar.WaitTimeout(s_Mutex, FLUSH_PERIODE);
            }
        } CRITICAL_END;
    }
}

