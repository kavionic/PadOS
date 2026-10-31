// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Ptr/PtrTarget.h>
#include <Utils/EnumBitmask.h>

class PPoint;
class PRect;
class PView;

struct PButtonStyleData;

struct PSliderStyleData;
enum class PSliderProperty : uint32_t;
using PSliderProperties = PEnumBitmask<PSliderProperty>;

struct PScrollBarStyleData;
enum class PScrollBarPart : uint8_t;
enum class PScrollBarProperty : uint32_t;
using PScrollBarProperties = PEnumBitmask<PScrollBarProperty>;

class PGUIStyle : public PtrTarget
{
public:
    ~PGUIStyle() override = default;

    virtual void ButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const = 0;
    virtual void ButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const = 0;

    virtual void CheckBoxPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const = 0;
    virtual void CheckBoxGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const = 0;

    virtual void RadioButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const = 0;
    virtual void RadioButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const = 0;

    virtual void SliderPaint(PView& target, const PRect& frame, const PSliderStyleData& data) const = 0;
    virtual void SliderGetPreferredSize(const PView& target, const PSliderStyleData& data, PPoint* minSize, PPoint* maxSize) const = 0;

    // Called after all listed properties have been updated. The style owns
    // invalidation and preferred-size notifications for these changes.
    virtual void SliderPropertyChanged(PView& target, PSliderStyleData& data, PSliderProperties properties) const = 0;

    virtual PPoint  SliderGetValuePosition(const PView& target, const PSliderStyleData& data, float relativeValue) const = 0;
    virtual float   SliderGetValueFromPosition(const PView& target, const PSliderStyleData& data, const PPoint& position) const = 0;

    virtual void ScrollBarPaint(PView& target, const PRect& frame, const PScrollBarStyleData& data) const = 0;
    virtual void ScrollBarGetPreferredSize(const PView& target, const PScrollBarStyleData& data, PPoint* minSize, PPoint* maxSize) const = 0;

    // Called after all listed properties have been updated. The style owns
    // invalidation and preferred-size notifications for these changes.
    virtual void ScrollBarPropertyChanged(PView& target, PScrollBarStyleData& data, PScrollBarProperties properties) const = 0;
    virtual PScrollBarPart ScrollBarHitTest(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const = 0;

    // Positions refer to the knob center. The widget preserves the pointer's
    // offset from this center while dragging.
    virtual PPoint ScrollBarGetValuePosition(const PView& target, const PScrollBarStyleData& data, float relativeValue) const = 0;
    virtual float ScrollBarGetValueFromPosition(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const = 0;
};
