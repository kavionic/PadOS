// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <utility>

#include <sys/types.h>

#include <ApplicationServer/ServerBitmap.h>
#include <Utils/IntrusiveList.h>

class ApplicationServer;

// The cache retains unused objects without a Ptr, so all Ptr references are
// external to the cache and LastReferenceGone marks the start of idle time.
class CachedSrvBitmap : public PSrvBitmap, public PIntrusiveListNode<CachedSrvBitmap>
{
public:
    using CacheKey = std::pair<dev_t, ino_t>;

    CachedSrvBitmap(const PIPoint& size, PEColorSpace colorSpace, const CacheKey& key);
    virtual ~CachedSrvBitmap() override = default;
    virtual bool LastReferenceGone() override;

    const CacheKey& GetCacheKey() const noexcept { return m_CacheKey; }
    size_t GetRasterSize() const noexcept { return m_BytesPerLine * size_t(m_Size.y); }
    void SetCacheOwner(ApplicationServer* server) noexcept { m_CacheOwner = server; }

private:
    const CacheKey m_CacheKey;
    ApplicationServer* m_CacheOwner = nullptr;
};
