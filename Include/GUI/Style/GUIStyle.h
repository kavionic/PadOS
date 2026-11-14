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

struct PDropdownMenuStyleData;
enum class PDropdownMenuProperty : uint32_t;
using PDropdownMenuProperties = PEnumBitmask<PDropdownMenuProperty>;

struct PDropdownMenuPopupStyleData;
enum class PDropdownMenuPopupProperty : uint32_t;
using PDropdownMenuPopupProperties = PEnumBitmask<PDropdownMenuPopupProperty>;

struct PGroupViewStyleData;
enum class PGroupViewProperty : uint32_t;
using PGroupViewProperties = PEnumBitmask<PGroupViewProperty>;

struct PProgressBarStyleData;
enum class PProgressBarProperty : uint32_t;
using PProgressBarProperties = PEnumBitmask<PProgressBarProperty>;

struct PTabViewStyleData;
struct PTabViewFrames;
enum class PTabViewProperty : uint32_t;
using PTabViewProperties = PEnumBitmask<PTabViewProperty>;

class PGUIStyle : public PtrTarget
{
public:
    ~PGUIStyle() override = default;

    virtual void ButtonPaint(PView& target, const PRect& bounds, bool isEnabled, PButtonStyleData& data) const = 0;
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

    virtual void    DropdownMenuPaint(PView& target, const PRect& frame, const PDropdownMenuStyleData& data) const = 0;
    virtual void    DropdownMenuGetPreferredSize(const PView& target, const PDropdownMenuStyleData& data, PPoint* minSize, PPoint* maxSize) const = 0;
    virtual void    DropdownMenuPropertyChanged(PView& target, PDropdownMenuStyleData& data, PDropdownMenuProperties properties) const = 0;
    virtual PRect   DropdownMenuGetEditorFrame(const PView& target, const PDropdownMenuStyleData& data) const = 0;
    virtual bool    DropdownMenuHitTest(const PView& target, const PDropdownMenuStyleData& data, const PPoint& position) const = 0;

    virtual void    DropdownMenuPopupFramePaint(PView& target, const PRect& frame) const = 0;
    virtual void    DropdownMenuPopupFrameGetPreferredSize(const PView& target, const PPoint& contentSize, PPoint* minSize, PPoint* maxSize) const = 0;
    virtual PRect   DropdownMenuPopupFrameGetContentFrame(const PView& target) const = 0;

    virtual void    DropdownMenuPopupPaint(PView& target, const PRect& updateRect, const PDropdownMenuPopupStyleData& data) const = 0;
    virtual void    DropdownMenuPopupGetPreferredSize(
        const PView& target,
        const PDropdownMenuPopupStyleData& data,
        PPoint* minSize,
        PPoint* maxSize) const = 0;

    virtual void DropdownMenuPopupPropertyChanged(
        PView& target,
        PDropdownMenuPopupStyleData& data,
        PDropdownMenuPopupProperties properties) const = 0;

    virtual PRect DropdownMenuPopupGetItemFrame(const PView& target, const PDropdownMenuPopupStyleData& data, size_t index) const = 0;

    virtual size_t DropdownMenuPopupGetItemFromPosition(
        const PView& target,
        const PDropdownMenuPopupStyleData& data,
        const PPoint& position) const = 0;

    virtual void GroupViewPaint(PView& target, const PRect& updateRect, const PGroupViewStyleData& data) const = 0;

    // Returns the frame and label size limits, combined with the child layout by PGroupView.
    virtual void GroupViewGetPreferredSize(const PView& target, const PGroupViewStyleData& data, PPoint* minSize, PPoint* maxSize) const = 0;
    virtual void GroupViewPropertyChanged(PView& target, PGroupViewStyleData& data, PGroupViewProperties properties) const = 0;

    virtual void ProgressBarPaint(PView& target, const PRect& frame, const PProgressBarStyleData& data) const = 0;
    virtual void ProgressBarGetPreferredSize(const PView& target, const PProgressBarStyleData& data, PPoint* minSize, PPoint* maxSize) const = 0;

    // Called after all listed properties have been updated. The style owns
    // invalidation and preferred-size notifications for these changes.
    virtual void ProgressBarPropertyChanged(PView& target, PProgressBarStyleData& data, PProgressBarProperties properties) const = 0;

    virtual void TabViewPaint(PView& target, const PView& tabsTarget, const PRect& updateRect, const PTabViewStyleData& data) const = 0;
    virtual void TabViewTabsPaint(PView& target, const PView& tabView, const PRect& updateRect, const PTabViewStyleData& data) const = 0;
    virtual void TabViewGetPreferredSize(
        const PView& target,
        const PTabViewStyleData& data,
        const PPoint& contentMinSize,
        const PPoint& contentMaxSize,
        PPoint* minSize,
        PPoint* maxSize) const = 0;

    // The style owns damage and preferred/content size notifications for both drawing surfaces.
    // tabIndex identifies the inserted, deleted, or renamed tab. Each notification may contain at most one such edit with other properties.
    virtual void TabViewPropertyChanged(
        PView& target,
        PView& tabsTarget,
        PTabViewStyleData& data,
        PTabViewProperties properties,
        size_t tabIndex) const = 0;

    virtual PTabViewFrames TabViewGetFrames(const PView& target, const PTabViewStyleData& data) const = 0;
    virtual PPoint TabViewGetTabsContentSize(const PView& target, const PTabViewStyleData& data) const = 0;

    // position is in tabsTarget's scrolled content coordinates.
    virtual size_t TabViewGetTabFromPosition(
        const PView& target,
        const PView& tabsTarget,
        const PTabViewStyleData& data,
        const PPoint& position) const = 0;
};
