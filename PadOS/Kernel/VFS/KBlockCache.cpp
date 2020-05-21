// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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

#include "Platform.h"

#include <string.h>

#include <map>

#include "KBlockCache.h"
//#include "SystemSetup.h"
#include "FileIO.h"
#include "KVFSManager.h"

using namespace kernel;
using namespace os;

//static const int KBLOCK_CACHE_BLOCK_COUNT = 65536;
static const int KBLOCK_CACHE_BLOCK_COUNT = 32;
static const int BC_FLUSH_COUNT = 4;

static uint8_t           gk_BCacheBuffer[KBlockCache::BUFFER_BLOCK_SIZE * KBLOCK_CACHE_BLOCK_COUNT + DCACHE_LINE_SIZE];
static KCacheBlockHeader gk_BCacheHeaders[KBLOCK_CACHE_BLOCK_COUNT];
//static uint8_t*           gk_BCacheBuffer;
//static KCacheBlockHeader* gk_BCacheHeaders;

std::map<int, KBlockCache*>                  KBlockCache::s_DeviceMap;
IntrusiveList<KCacheBlockHeader>             KBlockCache::s_FreeList;
IntrusiveList<KCacheBlockHeader>             KBlockCache::s_MRUList;
KMutex                                       KBlockCache::s_Mutex("bcache_mutex", false);
KConditionVariable                           KBlockCache::s_FlushingConditionVar("bcache_flush_cond");
std::atomic_int                              KBlockCache::s_DirtyBlockCount;


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
//    gk_BCacheBuffer = new uint8_t[KBlockCache::BUFFER_BLOCK_SIZE * KBLOCK_CACHE_BLOCK_COUNT + DCACHE_LINE_SIZE];
//    gk_BCacheHeaders = new KCacheBlockHeader[KBLOCK_CACHE_BLOCK_COUNT];
    
    
    uint8_t* buffer = reinterpret_cast<uint8_t*>((reinterpret_cast<intptr_t>(&gk_BCacheBuffer[0]) + DCACHE_LINE_SIZE_MASK) & ~DCACHE_LINE_SIZE_MASK);
    for (int i = 0; i < KBLOCK_CACHE_BLOCK_COUNT; ++i)
    {
        gk_BCacheHeaders[i].m_Buffer = buffer;
        buffer += BUFFER_BLOCK_SIZE;
        s_FreeList.Append(&gk_BCacheHeaders[i]);
    }
    spawn_thread("disk_cache_flusher", DiskCacheFlusher, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KCacheBlockDesc KBlockCache::GetBlock(off64_t blockNum, bool doLoad)
{
    CRITICAL_SCOPE(s_Mutex);
    
    
    doLoad = true; // Until we properly handle partially loaded blocks.
    
    off64_t bufferNum   = blockNum >> m_BlockToBufferShift;
    size_t  blockOffset = (blockNum & m_BufferOffsetMask) * m_BlockSize;
    
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
                for (block = s_MRUList.m_First; block != nullptr && block->m_UseCount == 0 && block->IsFlushing(); block = block->m_Next) /*EMPTY*/;
                if (block == nullptr) {
                    s_FlushingConditionVar.Wait(s_Mutex);
                    continue;
                }
                block = s_MRUList.m_Last;
                s_MRUList.Remove(block);
                FlushBuffer(block->m_Device, block->m_bufferNumber, true);
            }
            else
            {
                printf("ERROR: KBlockCache::GetBlock() all cache blocks locked!\n");
                return KCacheBlockDesc();
            }
            if (block != nullptr)
            {
                if (doLoad) {
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
//        KCacheBlockDesc block = GetBlock(blockNum + i, false);
        KCacheBlockDesc block = GetBlock(blockNum + i, true); // FIXME: Don't load the buffer if it's all going to be overwritten.

        if (block.m_Buffer != nullptr) {
            kprintf("Block %" PRIu64 " written\n", blockNum + i);
            memcpy(block.m_Buffer, reinterpret_cast<const uint8_t*>(buffer) + i * m_BlockSize, m_BlockSize);
            CRITICAL_SCOPE(s_Mutex);
            block.m_Block->SetDirty(true);
        } else {
            printf("KBlockCache::CachedWrite() failed to find block %d / %" PRIu64 "\n", m_Device, blockNum + i);
            return -1;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::FlushBuffer(int device, off64_t bufferNum, bool removeAfter)
{
    kassert(s_Mutex.IsLocked());
    KBlockCache* cache = GetDeviceCache(device);
    if (cache != nullptr)
    {
        return cache->FlushBuffer(bufferNum, removeAfter);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
bool KBlockCache::FlushBuffer(off64_t bufferNum, bool removeAfter)
{
    kassert(s_Mutex.IsLocked());
    auto i = m_BlockMap.find(bufferNum);
    if (i != m_BlockMap.end())
    {
        KCacheBlockHeader* block = i->second;
        if (block->IsDirty())
        {
            block->Flush(s_Mutex);
        }
        if (removeAfter) {
//            kprintf("Block %" PRIu64 " removed\n", bufferNum);
            m_BlockMap.erase(i);
        }
        return true;
    }
    return false;
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
        KBlockCache::s_FlushingConditionVar.Wakeup(0);
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
        if ((m_Flags & BCF_DIRTY) == 0) {
            m_Flags |= BCF_DIRTY;
            KBlockCache::s_DirtyBlockCount++;
        }
        kernel_log(KLogCategory::BlockCache, KLogSeverity::INFO_HIGH_VOL, "Block %" PRIu64 " from device %d dirty\n", m_bufferNumber, m_Device);
    }
    else
    {
        if (m_Flags & BCF_DIRTY) {
            m_Flags &= ~BCF_DIRTY;
            KBlockCache::s_DirtyBlockCount--;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KCacheBlockHeader::Flush(KMutex& mutex)
{
    kassert(mutex.IsLocked());
    
    if (IsDirty())
    {
        SetIsFlushing(true);
        mutex.Unlock();   
        
        kprintf("Block %" PRIu64 " flushed\n", m_bufferNumber);

        bool result = FileIO::Write(m_Device, m_bufferNumber * KBlockCache::BUFFER_BLOCK_SIZE, m_Buffer, KBlockCache::BUFFER_BLOCK_SIZE) >= 0;
        mutex.Lock();
        SetIsFlushing(false);
        SetDirty(false);
        KBlockCache::s_FlushingConditionVar.Wakeup(0);
        if (!result) {
            printf("ERROR: KCacheBlockHeader::Flush() failed to write block to disk: %s\n", strerror(get_last_error()));
        }
        return result;
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

void KBlockCache::FlushBlockList(KCacheBlockHeader** blockList, size_t blockCount)
{
    kernel_log(KLogCategory::BlockCache, KLogSeverity::INFO_HIGH_VOL, "KBlockCache::FlushBlockList() flushing %d blocks.\n", blockCount);
    for (size_t i = 0; i < blockCount; ++i)
    {
        KCacheBlockHeader* block = blockList[i];
        
        kernel_log(KLogCategory::BlockCache, KLogSeverity::INFO_HIGH_VOL, "KBlockCache::FlushBlockList() writing block %" PRId64 " from device %d\n", block->m_bufferNumber, block->m_Device);
        
        if (FileIO::Write(block->m_Device, block->m_bufferNumber * KBlockCache::BUFFER_BLOCK_SIZE, block->m_Buffer, KBlockCache::BUFFER_BLOCK_SIZE) < 0) {
            kernel_log(KLogCategory::BlockCache, KLogSeverity::CRITICAL, "Failed to flush block %" PRId64 " from device %d\n", block->m_bufferNumber, block->m_Device);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::DiskCacheFlusher(void* arg)
{
    for (;;)
    {
        if (s_DirtyBlockCount > 0) {
			snooze_ms(250);
        } else {
            snooze_s(5);
        }
        KVFSManager::FlushInodes();
        CRITICAL_BEGIN(s_Mutex)
        {
            KCacheBlockHeader* blockList[BC_FLUSH_COUNT];
            if (s_DirtyBlockCount > 0)
            {
                int blocksFlushed = 0;
                int deviceID = -1;
                for (KCacheBlockHeader* block = s_MRUList.m_First; block != nullptr && blocksFlushed < BC_FLUSH_COUNT && s_DirtyBlockCount > 0; block = block->m_Next)
                {
                    if (block->IsDirty() && !block->IsFlushing())
                    {
                        if (deviceID == -1) deviceID = block->m_Device;
                        if (block->m_Device == deviceID) {
                            block->SetIsFlushing(true);
                            blockList[blocksFlushed++] = block;
                        }
                    }
                }
                s_Mutex.Unlock();
                FlushBlockList(blockList, blocksFlushed);
                s_Mutex.Lock();
                for (size_t i = 0; i < blocksFlushed; ++i)
                {
                    KCacheBlockHeader* block = blockList[i];
                    block->SetDirty(false);
                    block->SetIsFlushing(false);
                }
                s_FlushingConditionVar.Wakeup(0);
            }
        } CRITICAL_END;
    }
}
