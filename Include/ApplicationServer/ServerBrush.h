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
    uint64_t GetRevision() const noexcept { return m_Revision; }
    void SetProperties(const PBrushDescription& description, Ptr<PSrvBitmap> bitmap);
    void SetColor(PColor color);

    Signal<void> SignalChanged;

    uint64_t m_RenderSequence = 0;
    size_t m_ResourceIndex = 0;
    PDisplayDriver* m_Driver = nullptr;
    PDriverResourceID m_DriverResourceID = DRIVER_INVALID_RESOURCE_ID;

private:
    Ptr<PSrvBitmap> m_Bitmap;
    PBrushDescription m_Description;
    uint64_t m_Revision = 1;
};

struct PServerBrushBinding
{
    Ptr<PServerBrush> Brush;
    PRect Bounds;
    PBrushSource Source = PBrushSource::Inherit;
    bool AutomaticBounds = true;
};

struct PServerBrushLayer
{
    Ptr<PServerBrush> Brush;
    PBrushDescription Description;
    PIRect Bounds;
    uint64_t Revision = 0;

    bool HasSameBrush(const PServerBrushLayer& rhs) const noexcept
    {
        return Brush == rhs.Brush && Revision == rhs.Revision && (Brush != nullptr || Description == rhs.Description);
    }
    bool operator==(const PServerBrushLayer& rhs) const noexcept { return Bounds == rhs.Bounds && HasSameBrush(rhs); }
};

// A constant composition needs no layer storage and uses the native solid paths.
struct PServerBrushState
{
    std::vector<PServerBrushLayer> Layers;
    PColor Color = PColor(255, 255, 255);
    bool IsSolid = true;
};
