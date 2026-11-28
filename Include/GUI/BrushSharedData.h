// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/BrushTypes.h>
#include <GUI/Image.h>
#include <Ptr/WeakPtr.h>

class PBrushSharedData : public PtrTarget
{
public:
    PBrushSharedData(PApplication* application, const PBrushData& data, const PImage& image);

    PApplication* GetApplication() const;
    const PBrushData& GetData() const noexcept { return m_Data; }
    const PImage& GetImage() const noexcept { return m_Image; }
    void SetProperties(const PBrushData& data, const PImage& image);

private:
    WeakPtr<PApplication> m_Application;
    PImage m_Image;
    PBrushData m_Data;
};
