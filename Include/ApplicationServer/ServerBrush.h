// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ApplicationServer/ServerBitmap.h>
#include <DeviceControl/DisplayBrush.h>
#include <Ptr/Ptr.h>
#include <vector>
#include <Signals/Signal.h>

class PServerBrush : public PtrTarget
{
public:
    PServerBrush(PDisplayDriver& driver, const PBrushDescription& description, Ptr<PSrvBitmap> bitmap);
    ~PServerBrush();

    const PBrushDescription& GetDescription() const noexcept { return m_Description; }
    const Ptr<PSrvBitmap>& GetBitmap() const noexcept { return m_Bitmap; }
    uint32_t GetRevision() const noexcept { return m_Revision; }
    void SetProperties(const PBrushDescription& description, Ptr<PSrvBitmap> bitmap);
    void SetColor(PColor color);

    Signal<void> SignalChanged;

    uint32_t m_RenderSequence = 0;
    size_t m_ResourceIndex = 0;
    PDisplayDriver* m_Driver = nullptr;
    PDriverResourceID m_DriverResourceID = DRIVER_INVALID_RESOURCE_ID;

private:
    void AdvanceRevision();

    Ptr<PSrvBitmap> m_Bitmap;
    PBrushDescription m_Description;
    uint32_t m_Revision = 1;
};

struct PServerBrushBinding
{
    Ptr<PServerBrush> Brush;
    PPoint Origin;
};

struct PServerBrushLayer
{
    Ptr<PServerBrush> Brush;
    PIRect Bounds;
    PIPoint ReferenceSize;
    uint32_t Revision = 0;
    PColor Color = PColor(255, 255, 255); // Used only by constant layers without a brush.

    bool HasSameBrush(const PServerBrushLayer& rhs) const noexcept
    {
        return Brush == rhs.Brush && Revision == rhs.Revision && (Brush != nullptr || Color == rhs.Color);
    }
    bool CanTranslateFrom(const PServerBrushLayer& previous, const PIPoint& movement) const noexcept
    {
        return HasSameBrush(previous)
            && (Brush == nullptr || PBrushUtils::CanTranslate(Brush->GetDescription(), previous.Bounds, Bounds, movement));
    }
    bool operator==(const PServerBrushLayer& rhs) const noexcept
    {
        return Bounds == rhs.Bounds && ReferenceSize == rhs.ReferenceSize && HasSameBrush(rhs);
    }
};

// A constant composition needs no layer storage and uses the native solid paths.
struct PServerBrushState
{
    std::vector<PServerBrushLayer> Layers;
    PColor Color = PColor(255, 255, 255);
    bool IsSolid() const noexcept { return Layers.empty(); }
};
