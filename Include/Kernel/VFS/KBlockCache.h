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

#pragma once
#include <stdint.h>
#include <stddef.h>

#include <atomic>

#include "System/Types.h"
#include "Utils/IntrusiveList.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"

namespace kernel
{

enum
{
    BCF_DIRTY       = 0x01,
    BCF_IS_FLUSHING = 0x02
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

struct KCacheBlockHeader
{
    void AddRef();
    void RemoveRef();
    
    bool IsDirty() const { return (m_Flags & BCF_DIRTY) != 0; }
    void SetDirty(bool isDirty);
    
    bool IsFlushing() const { return (m_Flags & BCF_IS_FLUSHING) != 0; }
    void SetIsFlushing(bool isFlushing) { m_Flags = (isFlushing) ? (m_Flags | BCF_IS_FLUSHING) : (m_Flags & ~BCF_IS_FLUSHING); }

    bool Flush(KMutex& mutex);

    KCacheBlockHeader*                m_Next         = nullptr;
    KCacheBlockHeader*                m_Prev         = nullptr;
    IntrusiveList<KCacheBlockHeader>* m_List         = nullptr;
    int                               m_Device       = 0;
    off64_t                           m_bufferNumber = 0;
    uint32_t                          m_UseCount     = 0;
    void*                             m_Buffer       = nullptr;
    uint32_t                          m_Flags        = 0;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

struct KCacheBlockDesc
{
    KCacheBlockDesc() : m_Block(nullptr), m_Buffer(nullptr) {}
    KCacheBlockDesc(KCacheBlockHeader* block, size_t blockOffset) : m_Block(block), m_Buffer(static_cast<uint8_t*>(block->m_Buffer) + blockOffset) {}
    ~KCacheBlockDesc();

    void MarkDirty();

    void Reset();
        
    KCacheBlockHeader* m_Block;
    void*              m_Buffer;

    KCacheBlockDesc(KCacheBlockDesc&& src) : m_Block(src.m_Block), m_Buffer(src.m_Buffer) { src.m_Block = nullptr; src.m_Buffer = nullptr; }
    KCacheBlockDesc& operator=(KCacheBlockDesc&& src);
    
    KCacheBlockDesc(const KCacheBlockDesc&) = delete;
    KCacheBlockDesc& operator=(const KCacheBlockDesc&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class KBlockCache
{
public:
    static const size_t BUFFER_BLOCK_SIZE = 512; //4096;
    static const size_t MIN_BLOCK_SIZE    = 512;
    static const size_t MAX_BLOCK_SIZE    = BUFFER_BLOCK_SIZE;
    
    KBlockCache();
    ~KBlockCache();

    static KBlockCache* GetDeviceCache(int device);
    bool SetDevice(int device, off64_t blockCount, size_t blockSize);
    
    static void Initialize();
        
    KCacheBlockDesc GetBlock(off64_t blockNum, bool doLoad = true);
    bool            MarkBlockDirty(off64_t blockNum);
    
    int  CachedRead(off64_t blockNum, void* buffer, size_t blockCount);
    int  CachedWrite(off64_t blockNum, const void* buffer, size_t blockCount);

    bool Flush() {return true;}
    bool Shutdown(bool flush) { if (flush) return Flush(); return true; }
        
private:
    friend struct KCacheBlockHeader;
    friend struct KCacheBlockDesc;

    static bool     FlushBuffer(int device, off64_t bufferNum, bool removeAfter);
    bool            FlushBuffer(off64_t bufferNum, bool removeAfter);
    
    static void  FlushBlockList(KCacheBlockHeader** blockList, size_t blockCount);
    static void  DiskCacheFlusher(void* arg);
    
    static std::map<int, KBlockCache*>                  s_DeviceMap;
    static IntrusiveList<KCacheBlockHeader>             s_FreeList;
    static IntrusiveList<KCacheBlockHeader>             s_MRUList;
    static KMutex                                       s_Mutex;
    static KConditionVariable                           s_FlushingConditionVar;
    static std::atomic_int s_DirtyBlockCount;
    
    int     m_Device;
    size_t  m_BlockSize;
    off64_t m_BlockCount;
    int     m_BlocksPerBuffer;
    int     m_BlockToBufferShift;
    uint32_t m_BufferOffsetMask;
    std::map<off64_t, KCacheBlockHeader*> m_BlockMap;
    
    KBlockCache(const KBlockCache&) = delete;
    KBlockCache& operator=(const KBlockCache&) = delete;
};

} // namespace
