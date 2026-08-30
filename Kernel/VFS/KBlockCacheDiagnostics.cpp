// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <utility>

#include <Kernel/KLogging.h>
#include <Kernel/KThread.h>
#include <Kernel/KTime.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KBlockCache.h>
#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>


namespace kernel
{

static constexpr size_t DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT = 2;
static constexpr size_t DIAGNOSTIC_READBACK_ATTEMPT_COUNT = 4;

static uint32_t* gk_DiagnosticSubmittedChecksums;
static uint32_t* gk_DiagnosticPreviousChecksums;
static uint8_t* gk_DiagnosticPreviousData;
static uint8_t* gk_DiagnosticReadbackBuffer;

alignas(DCACHE_LINE_SIZE)
static uint8_t gk_BlockCacheInternalRetryBuffer[
    KBlockCache::CACHE_BUFFER_SIZE * DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT]
    __attribute__((section(".bss.block_cache_diagnostic")));

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KBlockCache::GetFlushDiagnosticBufferSize()
{
    return sizeof(uint32_t) * MAX_FLUSH_BLOCK_COUNT * 2
        + CACHE_BUFFER_SIZE * MAX_FLUSH_BLOCK_COUNT
        + CACHE_BUFFER_SIZE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::InitializeFlushDiagnostics(void* buffer)
{
    uint8_t* currentBuffer = static_cast<uint8_t*>(buffer);

    gk_DiagnosticSubmittedChecksums = reinterpret_cast<uint32_t*>(currentBuffer);
    currentBuffer += sizeof(uint32_t) * MAX_FLUSH_BLOCK_COUNT;

    gk_DiagnosticPreviousChecksums = reinterpret_cast<uint32_t*>(currentBuffer);
    currentBuffer += sizeof(uint32_t) * MAX_FLUSH_BLOCK_COUNT;

    gk_DiagnosticPreviousData = currentBuffer;
    currentBuffer += CACHE_BUFFER_SIZE * MAX_FLUSH_BLOCK_COUNT;

    gk_DiagnosticReadbackBuffer = currentBuffer;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KBlockCache::ValidateFlushBlockList(KCacheBlockHeader** blockList, size_t blockCount)
{
    kassert(s_Mutex.IsLocked());

    for (size_t blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
        KCacheBlockHeader* block = blockList[blockIndex];
        if (block == nullptr)
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_BlockCache, "Block cache flush list contains a null block at index {}.", blockIndex);
            kassert(block != nullptr);
            continue;
        }

        KBlockCache* cache = block->m_BlockCache;
        if (cache == nullptr)
        {
            kernel_log<PLogSeverity::CRITICAL>(
                LogCatKernel_BlockCache,
                "Dirty block {} has no owning block cache.",
                block->m_bufferNumber);
            kassert(cache != nullptr);
            continue;
        }

        const int device = cache->m_Device;
        const auto mapping = cache->m_BlockMap.find(block->m_bufferNumber);
        if (mapping == cache->m_BlockMap.end() || mapping->second != block)
        {
            kernel_log<PLogSeverity::CRITICAL>(
                LogCatKernel_BlockCache,
                "Dirty block {} for device {} is not mapped to its flush-list header.",
                block->m_bufferNumber,
                device);
            kassert(mapping != cache->m_BlockMap.end() && mapping->second == block);
        }

        if (blockIndex != 0)
        {
            KCacheBlockHeader* previousBlock = blockList[blockIndex - 1];
            if (previousBlock != nullptr &&
                previousBlock->m_BlockCache == cache &&
                previousBlock->m_bufferNumber == block->m_bufferNumber)
            {
                kernel_log<PLogSeverity::CRITICAL>(
                    LogCatKernel_BlockCache,
                    "Block cache flush list contains device {} block {} more than once.",
                    device,
                    block->m_bufferNumber);
                kassert(previousBlock->m_bufferNumber != block->m_bufferNumber || previousBlock->m_BlockCache != cache);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static uint32_t CalculateBlockCacheDiagnosticChecksum(const void* buffer, size_t blockSize)
{
    const uint8_t* bytes = static_cast<const uint8_t*>(buffer);
    uint32_t checksum = 2166136261u;

    for (size_t byteIndex = 0; byteIndex < blockSize; ++byteIndex) {
        checksum = (checksum ^ bytes[byteIndex]) * 16777619u;
    }
    return checksum;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool VerifyBlockCacheDiagnosticWrite(
    int device,
    off64_t firstBlockNumber,
    const uint32_t* expectedChecksums,
    size_t blockCount,
    uint8_t* readbackBuffer,
    size_t blockSize,
    const char* retryDescription)
{
    for (size_t blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
        const off64_t blockNumber = firstBlockNumber + off64_t(blockIndex);
        const size_t bytesRead = kpread_trw(
            device,
            readbackBuffer,
            blockSize,
            blockNumber * off64_t(blockSize));
        const uint32_t readbackChecksum =
            (bytesRead == blockSize)
                ? CalculateBlockCacheDiagnosticChecksum(readbackBuffer, blockSize)
                : 0;

        if (bytesRead != blockSize
            || readbackChecksum != expectedChecksums[blockIndex])
        {
            printf(
                "Block cache diagnostic: %s readback for device %d block %lu returned %lu bytes, expected %08lx, raw %08lx.\n",
                retryDescription,
                device,
                static_cast<unsigned long>(blockNumber),
                static_cast<unsigned long>(bytesRead),
                static_cast<unsigned long>(expectedChecksums[blockIndex]),
                static_cast<unsigned long>(readbackChecksum));

            ksnooze(TimeValNanos::FromMilliseconds(1));
            SCB_InvalidateDCache_by_Addr(
                reinterpret_cast<uint32_t*>(readbackBuffer),
                blockSize);
            const uint32_t delayedReadbackChecksum =
                CalculateBlockCacheDiagnosticChecksum(readbackBuffer, blockSize);
            printf(
                "Block cache diagnostic: %s delayed memory reread for device %d block %lu issued no new device command, raw %08lx.\n",
                retryDescription,
                device,
                static_cast<unsigned long>(blockNumber),
                static_cast<unsigned long>(delayedReadbackChecksum));

            for (size_t readbackAttempt = 2;
                readbackAttempt <= DIAGNOSTIC_READBACK_ATTEMPT_COUNT;
                ++readbackAttempt)
            {
                const size_t repeatedBytesRead = kpread_trw(
                    device,
                    readbackBuffer,
                    blockSize,
                    blockNumber * off64_t(blockSize));
                const uint32_t repeatedReadbackChecksum =
                    (repeatedBytesRead == blockSize)
                        ? CalculateBlockCacheDiagnosticChecksum(readbackBuffer, blockSize)
                        : 0;
                printf(
                    "Block cache diagnostic: %s reread %lu for device %d block %lu returned %lu bytes, raw %08lx.\n",
                    retryDescription,
                    static_cast<unsigned long>(readbackAttempt),
                    device,
                    static_cast<unsigned long>(blockNumber),
                    static_cast<unsigned long>(repeatedBytesRead),
                    static_cast<unsigned long>(repeatedReadbackChecksum));
            }
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KBlockCache::FlushBlockListWithDiagnostics(KCacheBlockHeader** blockList, size_t blockCount)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_BlockCache, "KBlockCache::FlushBlockList() flushing {} blocks.", blockCount);

    if (blockCount == 0) {
        return false;
    }

    std::sort(blockList, blockList + blockCount, CompareCacheBlockOrder);

    ValidateFlushBlockList(blockList, blockCount);

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

        if (
            i == blockCount ||
            (i > start &&
                (blockList[i - 1]->m_BlockCache != blockList[i]->m_BlockCache ||
                    blockList[i - 1]->m_bufferNumber + 1 != blockList[i]->m_bufferNumber)))
        {
            size_t segmentCount = i - start;
            KBlockCache* blockCache = blockList[start]->m_BlockCache;
            kassert(blockCache != nullptr);
            const size_t blockSize = blockCache->m_BlockSize;
            if (requiredSegment || hasTimedOutBlocks || segmentCount * blockSize >= MIN_FLUSH_SIZE)
            {
                const int device = blockCache->m_Device;

                for (size_t j = start; j < i; ++j) {
                    blockList[j]->ClearDirtyPending();
                }

//                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_BlockCache, "  {}:{}", blockList[start]->m_bufferNumber, segmentCount);

                for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
                {
                    KCacheBlockHeader* block = blockList[start + segmentIndex];
                    kassert(block->m_BlockCache == blockCache);
                    kassert(block->m_bufferNumber == blockList[start]->m_bufferNumber + off64_t(segmentIndex));
                    kassert(segments[segmentIndex].iov_base == block->m_Buffer);
                    kassert(segments[segmentIndex].iov_len == blockSize);
                    gk_DiagnosticSubmittedChecksums[segmentIndex] =
                        CalculateBlockCacheDiagnosticChecksum(block->m_Buffer, blockSize);
                }

                PErrorCode writeError = PErrorCode::Success;
                s_Mutex.Unlock();
                try
                {
                    const off64_t firstBlockNumber = blockList[start]->m_bufferNumber;
                    for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
                    {
                        const off64_t blockNumber = firstBlockNumber + off64_t(segmentIndex);
                        uint8_t* previousData =
                            gk_DiagnosticPreviousData + segmentIndex * blockSize;
                        const size_t bytesRead = kpread_trw(
                            device,
                            previousData,
                            blockSize,
                            blockNumber * off64_t(blockSize));
                        kassert(bytesRead == blockSize);
                        gk_DiagnosticPreviousChecksums[segmentIndex] =
                            CalculateBlockCacheDiagnosticChecksum(previousData, blockSize);
                    }

                    const size_t bytesWritten = kpwritev_trw(device, segments, segmentCount, firstBlockNumber * off64_t(blockSize));
                    if (bytesWritten != segmentCount * blockSize)
                    {
                        printf(
                            "Block cache diagnostic: write of device %d blocks %lu:%lu returned %lu bytes.\n",
                            device,
                            static_cast<unsigned long>(firstBlockNumber),
                            static_cast<unsigned long>(segmentCount),
                            static_cast<unsigned long>(bytesWritten));
                        fflush(stdout);
                        kassert(bytesWritten == segmentCount * blockSize);
                    }

                    bool writeVerified = true;
                    size_t retrySegmentIndex = segmentCount;
                    for (size_t segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex)
                    {
                        KCacheBlockHeader* block = blockList[start + segmentIndex];
                        const off64_t blockNumber = firstBlockNumber + off64_t(segmentIndex);
                        const size_t bytesRead = kpread_trw(
                            device,
                            gk_DiagnosticReadbackBuffer,
                            blockSize,
                            blockNumber * off64_t(blockSize));
                        const uint32_t submittedChecksum = gk_DiagnosticSubmittedChecksums[segmentIndex];
                        const uint32_t readbackChecksum =
                            CalculateBlockCacheDiagnosticChecksum(gk_DiagnosticReadbackBuffer, blockSize);
                        const uint32_t currentSourceChecksum =
                            CalculateBlockCacheDiagnosticChecksum(block->m_Buffer, blockSize);
                        const bool sourceChanged = currentSourceChecksum != submittedChecksum;

                        if (sourceChanged)
                        {
                            printf(
                                "Block cache diagnostic: source buffer for device %d block %lu at segment %lu/%lu changed during the unlocked flush interval (%08lx -> %08lx).\n",
                                device,
                                static_cast<unsigned long>(blockNumber),
                                static_cast<unsigned long>(segmentIndex),
                                static_cast<unsigned long>(segmentCount),
                                static_cast<unsigned long>(submittedChecksum),
                                static_cast<unsigned long>(currentSourceChecksum));
                            fflush(stdout);
                        }

                        if (bytesRead != blockSize || readbackChecksum != submittedChecksum)
                        {
                            bool dirtyPending;
                            {
                                CRITICAL_SCOPE(s_Mutex);
                                dirtyPending = block->IsDirtyPending();
                            }

                            if (bytesRead != blockSize || (!sourceChanged && !dirtyPending)) {
                                writeVerified = false;
                            }

                            size_t matchingSegmentIndex = segmentCount;
                            size_t differingByteCount = 0;
                            size_t firstDifferingByte = blockSize;
                            size_t lastDifferingByte = 0;
                            size_t changedBeforeWriteByteCount = 0;
                            size_t submittedValueByteCount = 0;
                            size_t previousValueByteCount = 0;
                            size_t neitherValueByteCount = 0;
                            const uint8_t* submittedBytes = static_cast<const uint8_t*>(block->m_Buffer);
                            const uint8_t* previousBytes =
                                gk_DiagnosticPreviousData + segmentIndex * blockSize;
                            for (size_t candidateIndex = 0; candidateIndex < segmentCount; ++candidateIndex)
                            {
                                if (gk_DiagnosticSubmittedChecksums[candidateIndex] == readbackChecksum)
                                {
                                    matchingSegmentIndex = candidateIndex;
                                    break;
                                }
                            }
                            if (bytesRead == blockSize)
                            {
                                for (size_t byteIndex = 0; byteIndex < blockSize; ++byteIndex)
                                {
                                    if (submittedBytes[byteIndex] != previousBytes[byteIndex])
                                    {
                                        ++changedBeforeWriteByteCount;
                                        if (gk_DiagnosticReadbackBuffer[byteIndex] == submittedBytes[byteIndex]) {
                                            ++submittedValueByteCount;
                                        } else if (gk_DiagnosticReadbackBuffer[byteIndex] == previousBytes[byteIndex]) {
                                            ++previousValueByteCount;
                                        }
                                    }

                                    if (submittedBytes[byteIndex] != gk_DiagnosticReadbackBuffer[byteIndex])
                                    {
                                        if (differingByteCount == 0) {
                                            firstDifferingByte = byteIndex;
                                        }
                                        lastDifferingByte = byteIndex;
                                        ++differingByteCount;

                                        if (gk_DiagnosticReadbackBuffer[byteIndex] != previousBytes[byteIndex]) {
                                            ++neitherValueByteCount;
                                        }
                                    }
                                }
                            }

                            printf(
                                "Block cache diagnostic: raw device %d block %lu from write %lu:%lu does not match submitted segment %lu (bytes %lu, submitted %08lx, raw %08lx, current source %08lx).\n",
                                device,
                                static_cast<unsigned long>(blockNumber),
                                static_cast<unsigned long>(firstBlockNumber),
                                static_cast<unsigned long>(segmentCount),
                                static_cast<unsigned long>(segmentIndex),
                                static_cast<unsigned long>(bytesRead),
                                static_cast<unsigned long>(submittedChecksum),
                                static_cast<unsigned long>(readbackChecksum),
                                static_cast<unsigned long>(currentSourceChecksum));
                            if (readbackChecksum == gk_DiagnosticPreviousChecksums[segmentIndex])
                            {
                                printf(
                                    "Block cache diagnostic: raw block %lu still contains its pre-write data (%08lx).\n",
                                    static_cast<unsigned long>(blockNumber),
                                    static_cast<unsigned long>(gk_DiagnosticPreviousChecksums[segmentIndex]));
                            }
                            else
                            {
                                printf(
                                    "Block cache diagnostic: raw block %lu also differs from its pre-write data (%08lx).\n",
                                    static_cast<unsigned long>(blockNumber),
                                    static_cast<unsigned long>(gk_DiagnosticPreviousChecksums[segmentIndex]));
                            }
                            if (bytesRead == blockSize && differingByteCount != 0)
                            {
                                printf(
                                    "Block cache diagnostic: %lu differing bytes, first %lu (%02x -> %02x), last %lu (%02x -> %02x).\n",
                                    static_cast<unsigned long>(differingByteCount),
                                    static_cast<unsigned long>(firstDifferingByte),
                                    static_cast<unsigned int>(submittedBytes[firstDifferingByte]),
                                    static_cast<unsigned int>(gk_DiagnosticReadbackBuffer[firstDifferingByte]),
                                    static_cast<unsigned long>(lastDifferingByte),
                                    static_cast<unsigned int>(submittedBytes[lastDifferingByte]),
                                    static_cast<unsigned int>(gk_DiagnosticReadbackBuffer[lastDifferingByte]));
                                printf(
                                    "Block cache diagnostic: of %lu bytes changed by this write, the readback contains %lu submitted values and %lu previous values; %lu differing bytes match neither value.\n",
                                    static_cast<unsigned long>(changedBeforeWriteByteCount),
                                    static_cast<unsigned long>(submittedValueByteCount),
                                    static_cast<unsigned long>(previousValueByteCount),
                                    static_cast<unsigned long>(neitherValueByteCount));
                            }
                            if (dirtyPending)
                            {
                                printf(
                                    "Block cache diagnostic: block %lu was dirtied again during the unlocked flush interval; this readback mismatch is inconclusive and the block remains queued for another write.\n",
                                    static_cast<unsigned long>(blockNumber));
                            }
                            if (matchingSegmentIndex < segmentCount)
                            {
                                printf(
                                    "Block cache diagnostic: raw block %lu instead matches submitted segment %lu for block %lu.\n",
                                    static_cast<unsigned long>(blockNumber),
                                    static_cast<unsigned long>(matchingSegmentIndex),
                                    static_cast<unsigned long>(firstBlockNumber + off64_t(matchingSegmentIndex)));
                            }
                            else
                            {
                                printf(
                                    "Block cache diagnostic: raw block %lu does not match any segment submitted in this write.\n",
                                    static_cast<unsigned long>(blockNumber));
                            }

                            if (retrySegmentIndex == segmentCount
                                && bytesRead == blockSize
                                && !sourceChanged
                                && !dirtyPending) {
                                retrySegmentIndex = segmentIndex;
                            }
                            fflush(stdout);
                        }
                    }

                    if (retrySegmentIndex < segmentCount)
                    {
                        if (segmentCount >= DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT)
                        {
                            const size_t internalRetryStartSegment =
                                (retrySegmentIndex + DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT <= segmentCount)
                                    ? retrySegmentIndex
                                    : segmentCount - DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT;
                            const off64_t internalRetryFirstBlock =
                                firstBlockNumber + off64_t(internalRetryStartSegment);
                            const size_t internalRetryByteLength =
                                blockSize * DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT;
                            size_t changedInternalRetrySegmentIndex = segmentCount;

                            for (size_t internalRetrySegmentOffset = 0;
                                internalRetrySegmentOffset < DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT;
                                ++internalRetrySegmentOffset)
                            {
                                const size_t sourceSegmentIndex =
                                    internalRetryStartSegment + internalRetrySegmentOffset;
                                KCacheBlockHeader* sourceBlock =
                                    blockList[start + sourceSegmentIndex];
                                uint8_t* retryTarget =
                                    gk_BlockCacheInternalRetryBuffer
                                    + internalRetrySegmentOffset * blockSize;
                                memcpy(
                                    retryTarget,
                                    sourceBlock->m_Buffer,
                                    blockSize);

                                if (CalculateBlockCacheDiagnosticChecksum(retryTarget, blockSize)
                                    != gk_DiagnosticSubmittedChecksums[sourceSegmentIndex])
                                {
                                    changedInternalRetrySegmentIndex = sourceSegmentIndex;
                                    break;
                                }
                            }

                            if (changedInternalRetrySegmentIndex == segmentCount)
                            {
                                memcpy(
                                    gk_DiagnosticPreviousData,
                                    gk_BlockCacheInternalRetryBuffer,
                                    internalRetryByteLength);

                                for (size_t internalRetrySegmentOffset = 0;
                                    internalRetrySegmentOffset < DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT;
                                    ++internalRetrySegmentOffset)
                                {
                                    segments[internalRetrySegmentOffset].iov_base =
                                        gk_BlockCacheInternalRetryBuffer
                                        + internalRetrySegmentOffset * blockSize;
                                    segments[internalRetrySegmentOffset].iov_len =
                                        blockSize;
                                }

                                try
                                {
                                    const size_t internalDoubleBufferRetryBytesWritten = kpwritev_trw(
                                        device,
                                        segments,
                                        DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT,
                                        internalRetryFirstBlock * off64_t(blockSize));
                                    const bool internalDoubleBufferRetryVerified =
                                        internalDoubleBufferRetryBytesWritten == internalRetryByteLength
                                        && VerifyBlockCacheDiagnosticWrite(
                                            device,
                                            internalRetryFirstBlock,
                                            gk_DiagnosticSubmittedChecksums + internalRetryStartSegment,
                                            DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT,
                                            gk_BlockCacheInternalRetryBuffer,
                                            blockSize,
                                            "on-chip AXI SRAM double-buffer CMD25 retry");
                                    printf(
                                        "Block cache diagnostic: immutable on-chip AXI SRAM double-buffer CMD25 retry from %08lx wrote %lu bytes: %s.\n",
                                        static_cast<unsigned long>(intptr_t(gk_BlockCacheInternalRetryBuffer)),
                                        static_cast<unsigned long>(internalDoubleBufferRetryBytesWritten),
                                        internalDoubleBufferRetryVerified ? "matched" : "failed");
                                }
                                PERROR_CATCH(([&](PErrorCode error)
                                    {
                                        printf(
                                            "Block cache diagnostic: immutable on-chip AXI SRAM double-buffer CMD25 retry failed with error %d.\n",
                                            std::to_underlying(error));
                                    }
                                ));

                                memcpy(
                                    gk_BlockCacheInternalRetryBuffer,
                                    gk_DiagnosticPreviousData,
                                    internalRetryByteLength);

                                size_t changedRestoredSegmentIndex = segmentCount;
                                for (size_t internalRetrySegmentOffset = 0;
                                    internalRetrySegmentOffset < DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT;
                                    ++internalRetrySegmentOffset)
                                {
                                    const size_t sourceSegmentIndex =
                                        internalRetryStartSegment + internalRetrySegmentOffset;
                                    const uint8_t* restoredSegment =
                                        gk_BlockCacheInternalRetryBuffer
                                        + internalRetrySegmentOffset * blockSize;

                                    if (CalculateBlockCacheDiagnosticChecksum(restoredSegment, blockSize)
                                        != gk_DiagnosticSubmittedChecksums[sourceSegmentIndex])
                                    {
                                        changedRestoredSegmentIndex = sourceSegmentIndex;
                                        break;
                                    }
                                }

                                if (changedRestoredSegmentIndex == segmentCount)
                                {
                                    try
                                    {
                                        const size_t internalSingleBufferRetryBytesWritten = kpwrite_trw(
                                            device,
                                            gk_BlockCacheInternalRetryBuffer,
                                            internalRetryByteLength,
                                            internalRetryFirstBlock * off64_t(blockSize));
                                        const bool internalSingleBufferRetryVerified =
                                            internalSingleBufferRetryBytesWritten == internalRetryByteLength
                                            && VerifyBlockCacheDiagnosticWrite(
                                                device,
                                                internalRetryFirstBlock,
                                                gk_DiagnosticSubmittedChecksums + internalRetryStartSegment,
                                                DIAGNOSTIC_INTERNAL_RETRY_BLOCK_COUNT,
                                                gk_BlockCacheInternalRetryBuffer,
                                                blockSize,
                                                "on-chip AXI SRAM single-buffer CMD25 retry");
                                        printf(
                                            "Block cache diagnostic: immutable on-chip AXI SRAM single-buffer CMD25 retry from %08lx wrote %lu bytes: %s.\n",
                                            static_cast<unsigned long>(intptr_t(gk_BlockCacheInternalRetryBuffer)),
                                            static_cast<unsigned long>(internalSingleBufferRetryBytesWritten),
                                            internalSingleBufferRetryVerified ? "matched" : "failed");
                                    }
                                    PERROR_CATCH(([&](PErrorCode error)
                                        {
                                            printf(
                                                "Block cache diagnostic: immutable on-chip AXI SRAM single-buffer CMD25 retry failed with error %d.\n",
                                                std::to_underlying(error));
                                        }
                                    ));
                                }
                                else
                                {
                                    printf(
                                        "Block cache diagnostic: AXI SRAM single-buffer CMD25 retry skipped because restored segment %lu did not match the submitted data.\n",
                                        static_cast<unsigned long>(changedRestoredSegmentIndex));
                                }
                            }
                            else
                            {
                                printf(
                                    "Block cache diagnostic: AXI SRAM CMD25 retry skipped because cache segment %lu no longer matched the submitted data.\n",
                                    static_cast<unsigned long>(changedInternalRetrySegmentIndex));
                            }
                        }

                        size_t changedSnapshotSegmentIndex = segmentCount;
                        for (size_t snapshotSegmentIndex = 0; snapshotSegmentIndex < segmentCount; ++snapshotSegmentIndex)
                        {
                            KCacheBlockHeader* snapshotBlock = blockList[start + snapshotSegmentIndex];
                            uint8_t* retrySnapshot =
                                gk_DiagnosticPreviousData + snapshotSegmentIndex * blockSize;
                            memcpy(
                                retrySnapshot,
                                snapshotBlock->m_Buffer,
                                blockSize);

                            if (CalculateBlockCacheDiagnosticChecksum(retrySnapshot, blockSize)
                                != gk_DiagnosticSubmittedChecksums[snapshotSegmentIndex])
                            {
                                changedSnapshotSegmentIndex = snapshotSegmentIndex;
                                break;
                            }
                        }

                        if (changedSnapshotSegmentIndex == segmentCount)
                        {
                            const size_t retryByteLength =
                                segmentCount * blockSize;
                            SCB_CleanDCache_by_Addr(
                                reinterpret_cast<uint32_t*>(gk_DiagnosticPreviousData),
                                static_cast<int32_t>(retryByteLength));

                            for (size_t snapshotSegmentIndex = 0; snapshotSegmentIndex < segmentCount; ++snapshotSegmentIndex)
                            {
                                segments[snapshotSegmentIndex].iov_base =
                                    gk_DiagnosticPreviousData
                                    + snapshotSegmentIndex * blockSize;
                                segments[snapshotSegmentIndex].iov_len =
                                    blockSize;
                            }

                            try
                            {
                                const size_t doubleBufferRetryBytesWritten = kpwritev_trw(
                                    device,
                                    segments,
                                    segmentCount,
                                    firstBlockNumber * off64_t(blockSize));
                                const bool doubleBufferRetryVerified =
                                    doubleBufferRetryBytesWritten == retryByteLength
                                    && VerifyBlockCacheDiagnosticWrite(
                                        device,
                                        firstBlockNumber,
                                        gk_DiagnosticSubmittedChecksums,
                                        segmentCount,
                                        gk_BlockCacheInternalRetryBuffer,
                                        blockSize,
                                        "double-buffer CMD25 retry");
                                printf(
                                    "Block cache diagnostic: immutable double-buffer CMD25 retry wrote %lu bytes: %s.\n",
                                    static_cast<unsigned long>(doubleBufferRetryBytesWritten),
                                    doubleBufferRetryVerified ? "matched" : "failed");
                            }
                            PERROR_CATCH(([&](PErrorCode error)
                                {
                                    printf(
                                        "Block cache diagnostic: immutable double-buffer CMD25 retry failed with error %d.\n",
                                        std::to_underlying(error));
                                }
                            ));

                            try
                            {
                                const size_t singleBufferRetryBytesWritten = kpwrite_trw(
                                    device,
                                    gk_DiagnosticPreviousData,
                                    retryByteLength,
                                    firstBlockNumber * off64_t(blockSize));
                                const bool singleBufferRetryVerified =
                                    singleBufferRetryBytesWritten == retryByteLength
                                    && VerifyBlockCacheDiagnosticWrite(
                                        device,
                                        firstBlockNumber,
                                        gk_DiagnosticSubmittedChecksums,
                                        segmentCount,
                                        gk_BlockCacheInternalRetryBuffer,
                                        blockSize,
                                        "single-buffer CMD25 retry");
                                printf(
                                    "Block cache diagnostic: immutable single-buffer CMD25 retry wrote %lu bytes: %s.\n",
                                    static_cast<unsigned long>(singleBufferRetryBytesWritten),
                                    singleBufferRetryVerified ? "matched" : "failed");
                            }
                            PERROR_CATCH(([&](PErrorCode error)
                                {
                                    printf(
                                        "Block cache diagnostic: immutable single-buffer CMD25 retry failed with error %d.\n",
                                        std::to_underlying(error));
                                }
                            ));

                        }
                        else
                        {
                            printf(
                                "Block cache diagnostic: CMD25 A/B retries skipped because cache segment %lu no longer matched the submitted data.\n",
                                static_cast<unsigned long>(changedSnapshotSegmentIndex));
                        }
                        fflush(stdout);
                    }
                    kassert(writeVerified);
                    anythingProcessed = true;
                }
                PERROR_CATCH(([&writeError](PErrorCode error)
                    {
                        writeError = error;
                    }
                ));
                s_Mutex.Lock();

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
                    anythingProcessed = true;
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
            requiredSegment = requiredSegment || blockList[i]->IsFlushRequested();
            hasTimedOutBlocks =
                hasTimedOutBlocks || (curTime - blockList[i]->m_DirtyTime) >= FLUSH_PERIOD;
            segments[i - start].iov_base = blockList[i]->m_Buffer;
            segments[i - start].iov_len = blockList[i]->m_BlockCache->m_BlockSize;
        }
    }
    return anythingProcessed;
}


} // namespace kernel
