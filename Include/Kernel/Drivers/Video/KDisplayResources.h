// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <limits>
#include <vector>
#include <DeviceControl/Display.h>
#include <Ptr/Ptr.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

// Owned by one display session and accessed with its existing driver mutex.
// Lookups borrow a reference; rendering does not acquire a reference per pixel.
template<typename TResource>
class KDisplayResourceTable
{
public:
    PDriverResourceID Add(Ptr<TResource> resource)
    {
        uint32_t index = m_FirstFree;
        if (index == NO_FREE_SLOT)
        {
            if (m_Slots.size() > INDEX_MASK) {
                PERROR_THROW_CODE(PErrorCode::NOMEM);
            }
            index = uint32_t(m_Slots.size());
            m_Slots.emplace_back();
        }
        else
        {
            m_FirstFree = m_Slots[index].NextFree;
        }
        Slot& slot = m_Slots[index];
        slot.Resource = std::move(resource);
        return (slot.Generation << INDEX_BITS) | index;
    }

    const Ptr<TResource>& Get(PDriverResourceID id) const
    {
        const uint32_t index = id & INDEX_MASK;
        if (index >= m_Slots.size() || m_Slots[index].Generation != (id >> INDEX_BITS) || m_Slots[index].Resource == nullptr) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        return m_Slots[index].Resource;
    }

    void Remove(PDriverResourceID id)
    {
        Get(id);
        const uint32_t index = id & INDEX_MASK;
        Slot& slot = m_Slots[index];
        slot.Resource = nullptr;
        // Retire exhausted slots rather than allowing an old ID to become valid.
        if (slot.Generation != GENERATION_MAXIMUM)
        {
            ++slot.Generation;
            slot.NextFree = m_FirstFree;
            m_FirstFree = index;
        }
    }

    void Clear() noexcept
    {
        m_Slots.clear();
        m_FirstFree = NO_FREE_SLOT;
    }

private:
    static constexpr uint32_t INDEX_BITS = 16;
    static constexpr uint32_t INDEX_MASK = 0xffff;
    static constexpr uint32_t GENERATION_MAXIMUM = std::numeric_limits<PDriverResourceID>::max() >> INDEX_BITS;
    static constexpr uint32_t NO_FREE_SLOT = std::numeric_limits<uint32_t>::max();

    struct Slot
    {
        Ptr<TResource> Resource;
        uint32_t Generation = 1;
        uint32_t NextFree = NO_FREE_SLOT;
    };

    std::vector<Slot> m_Slots;
    uint32_t m_FirstFree = NO_FREE_SLOT;
};

class KDisplayBitmapResource : public PtrTarget
{
public:
    KDisplayBitmapResource(PDisplayBitmap bitmap, bool writable) : m_Bitmap(bitmap), m_Writable(writable) {}

    PDisplayBitmap* GetBitmap(bool write)
    {
        if (write && !m_Writable) {
            PERROR_THROW_CODE(PErrorCode::ACCES);
        }
        return &m_Bitmap;
    }

private:
    PDisplayBitmap m_Bitmap;
    bool m_Writable;
};

class KDisplayBrushResource : public PtrTarget
{
public:
    KDisplayBrushResource(Ptr<KDisplayBitmapResource> bitmap, const PBrushDescription& description)
        : m_Bitmap(std::move(bitmap)), m_Description(description) {}

    const Ptr<KDisplayBitmapResource>& GetBitmap() const noexcept { return m_Bitmap; }
    const PBrushDescription& GetDescription() const noexcept { return m_Description; }
    void SetProperties(Ptr<KDisplayBitmapResource> bitmap, const PBrushDescription& description) noexcept
    {
        m_Bitmap = std::move(bitmap);
        m_Description = description;
    }
    void SetColor(PColor color) noexcept
    {
        m_Description.SetColor(color);
        m_Bitmap = nullptr;
    }

private:
    Ptr<KDisplayBitmapResource> m_Bitmap;
    PBrushDescription m_Description;
};

} // namespace kernel
