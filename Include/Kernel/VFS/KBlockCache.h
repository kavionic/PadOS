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

#pragma once
#include <stdint.h>
#include <stddef.h>

#include <atomic>

#include "System/Types.h"
#include "Signals/Signal.h"
#include "Utils/IntrusiveList.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"

namespace kernel
{

enum
{
    BCF_DIRTY           = 0x01,
    BCF_DIRTY_PENDING   = 0x02,
    BCF_FLUSH_REQUESTED = 0x04,
    BCF_IS_FLUSHING     = 0x08,
    BCF_DISCARD         = 0x10
};

class KBlockCache;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

struct KCacheBlockHeader : PIntrusiveListNode<KCacheBlockHeader>
{
    void AddRef();
    void RemoveRef();
    
    inline bool IsDirty() const { return (m_Flags & BCF_DIRTY) != 0; }
    bool SetDirty(bool isDirty);

    inline bool IsDirtyPending() const { return (m_Flags & BCF_DIRTY_PENDING) != 0; }
    inline void ClearDirtyPending() { m_Flags &= ~BCF_DIRTY_PENDING; }

    inline bool IsFlushRequested() const { return (m_Flags & BCF_FLUSH_REQUESTED) != 0; }
    inline void SetFlushRequested(bool isRequesting) { m_Flags = (isRequesting) ? (m_Flags | BCF_FLUSH_REQUESTED) : (m_Flags & ~BCF_FLUSH_REQUESTED); }

    inline bool IsFlushing() const { return (m_Flags & BCF_IS_FLUSHING) != 0; }
    inline void SetIsFlushing(bool isFlushing) { m_Flags = (isFlushing) ? (m_Flags | BCF_IS_FLUSHING) : (m_Flags & ~BCF_IS_FLUSHING); }

    inline bool IsDiscardRequested() const { return (m_Flags & BCF_DISCARD) != 0; }
    inline void SetDiscardRequested(bool isDiscarding) { m_Flags = (isDiscarding) ? (m_Flags | BCF_DISCARD) : (m_Flags & ~BCF_DISCARD); }

    KBlockCache* m_BlockCache   = nullptr;
    off64_t      m_bufferNumber = 0;
    uint32_t     m_UseCount     = 0;
    void*        m_Buffer       = nullptr;
    TimeValNanos m_DirtyTime;
    uint32_t     m_Flags        = 0;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

struct KCacheBlockDesc
{
    inline KCacheBlockDesc() : m_Block(nullptr), m_Buffer(nullptr) {}
    inline KCacheBlockDesc(KCacheBlockHeader* block, size_t blockOffset) : m_Block(block), m_Buffer(static_cast<uint8_t*>(block->m_Buffer) + blockOffset) {}
    ~KCacheBlockDesc();

    void MarkDirty();

    void Reset();
        
    KCacheBlockHeader* m_Block;
    void*              m_Buffer;

    inline KCacheBlockDesc(KCacheBlockDesc&& src) : m_Block(src.m_Block), m_Buffer(src.m_Buffer) { src.m_Block = nullptr; src.m_Buffer = nullptr; }
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
    
    KBlockCache() = default;
    ~KBlockCache();

    static inline size_t GetDirtyBlockCount() { return s_DirtyBlockCount; }

    static KBlockCache* GetDeviceCache(int device);
    bool SetDevice(int device, off64_t blockCount, size_t blockSize, bool readOnly = false);
    bool IsReadOnly() const { return m_IsReadOnly.load(std::memory_order_relaxed); }
    size_t GetDeviceDirtyBlockCount() const { return m_DirtyBlockCount.load(std::memory_order_relaxed); }
    
    static void Initialize();
        
    KCacheBlockDesc GetBlock_trw(off64_t blockNum, bool doLoad = true);
    PErrorCode      MarkBlockDirty(off64_t blockNum);
    
    void CachedRead_trw(off64_t blockNum, void* buffer, size_t blockCount);
    void CachedWrite_trw(off64_t blockNum, const void* buffer, size_t blockCount);

    bool Flush();
    bool Sync();
    inline bool Shutdown(bool flush) { if (flush) return Sync(); return true; }

    Signal<void, PErrorCode> SignalBecameReadOnly;
        
private:
    friend struct KCacheBlockHeader;
    friend struct KCacheBlockDesc;

    static constexpr TimeValNanos FLUSH_PERIOD = TimeValNanos::FromMilliseconds(1000);
    static constexpr size_t MAX_FLUSH_BLOCK_COUNT = 128;
    static constexpr size_t MIN_FLUSH_WAKEUP_BLOCK_COUNT = 64;
    static constexpr size_t MIN_FLUSH_BLOCK_COUNT = 96;

    bool DetachBlocks(bool waitForBusyBlocks, bool discardDirtyBlocks);
    void AbandonBlockWriteback(KCacheBlockHeader* block);
    void DiscardBlock(KCacheBlockHeader* block);
    void ReleaseBlock(KCacheBlockHeader* block);
    void SetReadOnlyAfterWriteError(PErrorCode error);
    bool FlushInternal();

    static bool  CompareCacheBlockOrder(const KCacheBlockHeader* lhs, const KCacheBlockHeader* rhs);
    static bool  FlushBlockList(KCacheBlockHeader** blockList, size_t blockCount);
    static void* DiskCacheFlusher(void* arg);

#ifdef PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
    static size_t GetFlushDiagnosticBufferSize();
    static void   InitializeFlushDiagnostics(void* buffer);
    static void   ValidateFlushBlockList(KCacheBlockHeader** blockList, size_t blockCount);
    static bool   FlushBlockListWithDiagnostics(KCacheBlockHeader** blockList, size_t blockCount);
#endif // PADOS_OPT_DEBUG_BLOCK_CACHE_DIAGNOSTICS
    
    static std::map<int, KBlockCache*>      s_DeviceMap;
    static PIntrusiveList<KCacheBlockHeader> s_FreeList;
    static PIntrusiveList<KCacheBlockHeader> s_MRUList;
    static KMutex                           s_Mutex;
    static KConditionVariable               s_FlushingRequestConditionVar;
    static KConditionVariable               s_FlushingDoneConditionVar;
    static std::atomic_int                  s_DirtyBlockCount;
    static size_t                           s_PendingReadOnlySignalCount;
    
    int                                     m_Device = -1;
    std::atomic_size_t                      m_DirtyBlockCount = 0;
    std::atomic_bool                        m_IsReadOnly = true;
    PErrorCode                              m_WriteError = PErrorCode::Success;
    bool                                    m_ReadOnlySignalPending = false;
    bool                                    m_ReadOnlySignalInProgress = false;
    size_t                                  m_BlockSize = 0;
    off64_t                                 m_BlockCount = 0;
    int                                     m_BlocksPerBuffer = 1;
    int                                     m_BlockToBufferShift = 0;
    uint32_t                                m_BufferOffsetMask = 0x00;
    std::map<off64_t, KCacheBlockHeader*>   m_BlockMap;
    
    KBlockCache(const KBlockCache&) = delete;
    KBlockCache& operator=(const KBlockCache&) = delete;
};

size_t kget_dirty_disk_cache_blocks();

} // namespace
