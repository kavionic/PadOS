// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <GUI/Brush.h>
#include <GUI/Style/ScrollBarStyleData.h>
#include <GUI/Style/TabViewStyleData.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Widgets/TextBox.h>

class PPath;

namespace pugi
{
class xml_node;
}

enum class PButtonSkinState : uint8_t
{
    Normal,
    Hovered,
    Pressed,
    Disabled,
    Count
};

struct PButtonSkinAppearance
{
    Ptr<const PBrush> Background;
    PColor LabelColor = PColor(0, 0, 0);
};

struct PButtonSkin
{
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> States;
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> CheckedStates;
    // Text insets are independent of the image's nine-slice borders.
    PRect LabelPadding = PRect(8.0f, 4.0f, 8.0f, 4.0f);
    PPoint PressedLabelOffset = PPoint(1.0f, 1.0f);

    PButtonSkin();
};

struct PButtonIndicatorSkin
{
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> States;
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> CheckedStates;
    // Indicator images use absolute layout with dimensions (Size, Size).
    float Size = 32.0f;
    float LabelSpacing = 8.0f;
    float LabelVerticalOffset = -1.0f;
};

using PCheckBoxSkin = PButtonIndicatorSkin;

struct PRadioButtonSkin : PButtonIndicatorSkin
{
    float LabelVerticalSpacing = 4.0f;
};

struct PSliderSkinColors
{
    PColor Before;
    PColor After;
    PColor Label;
    PColor Tick;
    PColor TickHighlight;

    bool operator==(const PSliderSkinColors& rhs) const = default;
};

struct PSliderKnobAppearance
{
    Ptr<const PBitmap> Bitmap;
    // Bitmap pixel aligned with the track; rounded to the nearest native pixel.
    PPoint Anchor;

    PRect GetBounds() const;
};

struct PSliderKnobSkin
{
    std::array<PSliderKnobAppearance, std::to_underlying(PButtonSkinState::Count)> States;
    PSliderKnobAppearance Shadow;

    PRect GetBounds() const;
};

struct PSliderOrientationSkin
{
    Ptr<const PImage> Track;
    // Plain, leading point, trailing point, both points.
    std::array<PSliderKnobSkin, 4> Knobs;
};

struct PSliderSkin
{
    std::array<PSliderSkinColors, std::to_underlying(PButtonSkinState::Count)> Colors;
    PSliderOrientationSkin Horizontal;
    PSliderOrientationSkin Vertical;
};

struct PScrollBarSkinAppearance
{
    Ptr<const PImage> Image;
    Ptr<const PBitmap> Glyph;
    PPoint GlyphOffset;
};

struct PScrollBarSkinPart
{
    std::array<PScrollBarSkinAppearance, std::to_underlying(PButtonSkinState::Count)> States;
};

struct PScrollBarOrientationSkin
{
    PScrollBarSkinPart Track;
    PScrollBarSkinPart Knob;
    PScrollBarSkinPart Decrement;
    PScrollBarSkinPart Increment;
    PRect KnobInsets = PRect(2.0f, 2.0f, 2.0f, 2.0f);
    float Thickness = 20.0f;
    float ArrowLength = 18.0f;
    float MinimumKnobLength = 24.0f;
    bool DoubleArrows = true;
};

struct PScrollBarSkin
{
    PScrollBarOrientationSkin Horizontal;
    PScrollBarOrientationSkin Vertical;
};

enum class PTabSkinState : uint8_t
{
    Normal,
    Hovered,
    Pressed,
    Count
};

struct PTabSkinAppearance
{
    Ptr<const PImage> Image;
    PColor LabelColor = PColor(0, 0, 0);
    PPoint LabelOffset;
};

struct PTabViewSkin
{
    std::array<PTabSkinAppearance, std::to_underlying(PTabSkinState::Count)> States;
    std::array<PTabSkinAppearance, std::to_underlying(PTabSkinState::Count)> SelectedStates;
    Ptr<const PImage> Frame;
    Ptr<const PBitmap> OverflowLeft;
    Ptr<const PBitmap> OverflowRight;
    PRect FrameInsets = PRect(3.0f, 3.0f, 3.0f, 3.0f);
    PRect LabelPadding = PRect(12.0f, 4.0f, 12.0f, 6.0f);
    PPoint MinimumTabSize = PPoint(40.0f, 28.0f);
    float TabSpacing = 2.0f;
    float InactiveTopInset = 3.0f;
    float OverflowWidth = 12.0f;
};

enum class PDropdownMenuRowState : uint8_t
{
    Normal,
    Selected,
    Pressed,
    Count
};

struct PDropdownMenuAppearance
{
    Ptr<const PBrush> Background;
    Ptr<const PBitmap> Arrow;
    Ptr<const PTextBoxStyle> Editor;
    PPoint ArrowOffset;
};

struct PDropdownMenuRowAppearance
{
    Ptr<const PImage> Image;
    PColor LabelColor = PColor(0, 0, 0);
    PPoint LabelOffset;
};

struct PDropdownMenuSkin
{
    std::array<PDropdownMenuAppearance, std::to_underlying(PButtonSkinState::Count)> States;
    std::array<PDropdownMenuRowAppearance, std::to_underlying(PDropdownMenuRowState::Count)> Rows;
    Ptr<const PBrush> PopupFrame;
    Ptr<const PBrush> PopupBackground;
    PRect EditorInsets = PRect(10.0f, 4.0f, 6.0f, 5.0f);
    PRect PopupInsets = PRect(4.0f, 4.0f, 4.0f, 4.0f);
    PRect RowPadding = PRect(8.0f, 4.0f, 8.0f, 5.0f);
    PPoint MinimumSize = PPoint(72.0f, 32.0f);
    float ArrowWidth = 30.0f;
    float MinimumRowHeight = 32.0f;
};

struct PGroupViewSkin
{
    Ptr<const PImage> Frame;
    PRect FrameInsets;
    PRect LabelPadding = PRect(14.0f, 0.0f, 14.0f, 0.0f);
    PPoint MinimumSize = PPoint(28.0f, 28.0f);
    PColor LabelColor = PColor(0, 0, 0);
    float LabelGap = 4.0f;
};

struct PProgressBarOrientationSkin
{
    Ptr<const PImage> Track;
    Ptr<const PImage> Fill;
    PRect FillInsets = PRect(3.0f, 3.0f, 3.0f, 3.0f);
    float MinimumLength = 50.0f;
    float Thickness = 24.0f;
};

struct PProgressBarSkin
{
    PProgressBarOrientationSkin Horizontal;
    PProgressBarOrientationSkin Vertical;
};

class PSkinnableGUIStyle : public PDefaultGUIStyle
{
public:
    PSkinnableGUIStyle();
    explicit PSkinnableGUIStyle(const PButtonSkin& buttonSkin);
    PSkinnableGUIStyle(const PButtonSkin& buttonSkin, const PCheckBoxSkin& checkBoxSkin);
    PSkinnableGUIStyle(const PButtonSkin& buttonSkin, const PCheckBoxSkin& checkBoxSkin, const PRadioButtonSkin& radioButtonSkin);
    PSkinnableGUIStyle(
        const PButtonSkin& buttonSkin,
        const PCheckBoxSkin& checkBoxSkin,
        const PRadioButtonSkin& radioButtonSkin,
        const PSliderSkin& sliderSkin,
        PApplication* application = nullptr);
    PSkinnableGUIStyle(
        const PButtonSkin& buttonSkin,
        const PCheckBoxSkin& checkBoxSkin,
        const PRadioButtonSkin& radioButtonSkin,
        const PSliderSkin& sliderSkin,
        const PScrollBarSkin& scrollBarSkin,
        PApplication* application = nullptr);
    PSkinnableGUIStyle(
        const PButtonSkin& buttonSkin,
        const PCheckBoxSkin& checkBoxSkin,
        const PRadioButtonSkin& radioButtonSkin,
        const PSliderSkin& sliderSkin,
        const PScrollBarSkin& scrollBarSkin,
        const PTabViewSkin& tabViewSkin,
        PApplication* application = nullptr);
    PSkinnableGUIStyle(
        const PButtonSkin& buttonSkin,
        const PCheckBoxSkin& checkBoxSkin,
        const PRadioButtonSkin& radioButtonSkin,
        const PSliderSkin& sliderSkin,
        const PScrollBarSkin& scrollBarSkin,
        const PTabViewSkin& tabViewSkin,
        const PDropdownMenuSkin& dropdownMenuSkin,
        PApplication* application = nullptr);
    PSkinnableGUIStyle(
        const PButtonSkin& buttonSkin,
        const PCheckBoxSkin& checkBoxSkin,
        const PRadioButtonSkin& radioButtonSkin,
        const PSliderSkin& sliderSkin,
        const PScrollBarSkin& scrollBarSkin,
        const PTabViewSkin& tabViewSkin,
        const PDropdownMenuSkin& dropdownMenuSkin,
        const PGroupViewSkin& groupViewSkin,
        PApplication* application = nullptr);
    PSkinnableGUIStyle(
        const PButtonSkin& buttonSkin,
        const PCheckBoxSkin& checkBoxSkin,
        const PRadioButtonSkin& radioButtonSkin,
        const PSliderSkin& sliderSkin,
        const PScrollBarSkin& scrollBarSkin,
        const PTabViewSkin& tabViewSkin,
        const PDropdownMenuSkin& dropdownMenuSkin,
        const PGroupViewSkin& groupViewSkin,
        const PProgressBarSkin& progressBarSkin,
        PApplication* application = nullptr);
    // Load Style.xml, widget XML files and their assets for this instance.
    explicit PSkinnableGUIStyle(const PPath& directory, PApplication* application = nullptr);

    static PString GetName();
    static std::vector<PString> GetVariants();
    static Ptr<const PGUIStyle> CreateStyle(PApplication* application, const PString& variant);

    Ptr<const PBrush> GetBackgroundBrush() const override;

    void ButtonPaint(PView& target, const PRect& bounds, bool isEnabled, PButtonStyleData& data) const override;
    void ButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

    void CheckBoxPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const override;
    void CheckBoxGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

    void RadioButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const override;
    void RadioButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

    void SliderPaint(PView& target, const PRect& frame, const PSliderStyleData& data) const override;
    void SliderGetPreferredSize(const PView& target, const PSliderStyleData& data, PPoint* minSize, PPoint* maxSize) const override;
    void SliderPropertyChanged(PView& target, PSliderStyleData& data, PSliderProperties properties) const override;
    PPoint SliderGetValuePosition(const PView& target, const PSliderStyleData& data, float relativeValue) const override;
    float SliderGetValueFromPosition(const PView& target, const PSliderStyleData& data, const PPoint& position) const override;

    void ScrollBarPaint(PView& target, const PRect& frame, const PScrollBarStyleData& data) const override;
    void ScrollBarGetPreferredSize(const PView& target, const PScrollBarStyleData& data, PPoint* minSize, PPoint* maxSize) const override;
    void ScrollBarPropertyChanged(PView& target, PScrollBarStyleData& data, PScrollBarProperties properties) const override;
    PScrollBarPart ScrollBarHitTest(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const override;
    PPoint ScrollBarGetValuePosition(const PView& target, const PScrollBarStyleData& data, float relativeValue) const override;
    float ScrollBarGetValueFromPosition(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const override;

    void DropdownMenuPaint(PView& target, const PRect& frame, const PDropdownMenuStyleData& data) const override;
    void DropdownMenuGetPreferredSize(const PView& target, const PDropdownMenuStyleData& data, PPoint* minSize, PPoint* maxSize) const override;
    void DropdownMenuPropertyChanged(PView& target, PDropdownMenuStyleData& data, PDropdownMenuProperties properties) const override;
    PRect DropdownMenuGetEditorFrame(const PView& target, const PDropdownMenuStyleData& data) const override;
    Ptr<const PTextBoxStyle> DropdownMenuGetEditorStyle(const PDropdownMenuStyleData& data) const override;
    bool DropdownMenuHitTest(const PView& target, const PDropdownMenuStyleData& data, const PPoint& position) const override;
    void DropdownMenuPopupFramePaint(PView& target, const PRect& frame) const override;
    void DropdownMenuPopupFrameGetPreferredSize(const PView& target, const PPoint& contentSize, PPoint* minSize, PPoint* maxSize) const override;
    PRect DropdownMenuPopupFrameGetContentFrame(const PView& target) const override;
    void DropdownMenuPopupPaint(PView& target, const PRect& updateRect, const PDropdownMenuPopupStyleData& data) const override;
    void DropdownMenuPopupGetPreferredSize(const PView& target, const PDropdownMenuPopupStyleData& data, PPoint* minSize, PPoint* maxSize) const override;
    void DropdownMenuPopupPropertyChanged(PView& target, PDropdownMenuPopupStyleData& data, PDropdownMenuPopupProperties properties) const override;
    PRect DropdownMenuPopupGetItemFrame(const PView& target, const PDropdownMenuPopupStyleData& data, size_t index) const override;
    size_t DropdownMenuPopupGetItemFromPosition(const PView& target, const PDropdownMenuPopupStyleData& data, const PPoint& position) const override;

    virtual void GroupViewPaint(PView& target, const PRect& updateRect, const PGroupViewStyleData& data) const override;
    virtual void GroupViewGetPreferredSize(const PView& target, const PGroupViewStyleData& data, PPoint* minSize, PPoint* maxSize) const override;
    virtual void GroupViewPropertyChanged(PView& target, PGroupViewStyleData& data, PGroupViewProperties properties) const override;

    virtual void ProgressBarPaint(PView& target, const PRect& frame, const PProgressBarStyleData& data) const override;
    virtual void ProgressBarGetPreferredSize(const PView& target, const PProgressBarStyleData& data, PPoint* minSize, PPoint* maxSize) const override;
    virtual void ProgressBarPropertyChanged(PView& target, PProgressBarStyleData& data, PProgressBarProperties properties) const override;

    void TabViewPaint(PView& target, const PView& tabsTarget, const PRect& updateRect, const PTabViewStyleData& data) const override;
    void TabViewTabsPaint(PView& target, const PView& tabView, const PRect& updateRect, const PTabViewStyleData& data) const override;
    void TabViewGetPreferredSize(
        const PView& target,
        const PTabViewStyleData& data,
        const PPoint& contentMinSize,
        const PPoint& contentMaxSize,
        PPoint* minSize,
        PPoint* maxSize) const override;
    void TabViewPropertyChanged(PView& target, PView& tabsTarget, PTabViewStyleData& data, PTabViewProperties properties, size_t tabIndex) const override;
    PTabViewFrames TabViewGetFrames(const PView& target, const PTabViewStyleData& data) const override;
    PPoint TabViewGetTabsContentSize(const PView& target, const PTabViewStyleData& data) const override;
    size_t TabViewGetTabFromPosition(const PView& target, const PView& tabsTarget, const PTabViewStyleData& data, const PPoint& position) const override;

private:
    struct SkinnedSliderStyleData;
    struct SkinnedScrollBarPart;
    struct SkinnedScrollBarStyleData;
    struct SkinnedScrollBarGeometry;
    struct SkinnedImage;
    struct SkinnedTabViewTab;
    struct SkinnedTabViewStyleData;
    struct SkinnedTabViewGeometry;
    struct SkinnedDropdownMenuStyleData;
    struct SkinnedDropdownMenuPopupStyleData;
    struct SkinnedGroupViewStyleData;
    struct SkinnedProgressBarStyleData;

    static constexpr std::array<PScrollBarPart, 4> SCROLL_BAR_ARROW_PARTS =
    {
        PScrollBarPart::DecrementStart, PScrollBarPart::IncrementStart, PScrollBarPart::DecrementEnd, PScrollBarPart::IncrementEnd
    };

    static constexpr std::array<const char*, std::to_underlying(PButtonSkinState::Count)> STATE_NAMES = {"Normal", "Hovered", "Pressed", "Disabled"};

    static constexpr std::array<const char*, std::to_underlying(PTabSkinState::Count)> TAB_STATE_NAMES = {"Normal", "Hovered", "Pressed"};

    static constexpr std::array<const char*, std::to_underlying(PDropdownMenuRowState::Count)> DROPDOWN_ROW_STATE_NAMES = {"Normal", "Selected", "Pressed"};

    static PButtonSkinState GetButtonState(bool isEnabled, const PButtonStyleData& data);
    static void ValidateIndicatorSkin(const PButtonIndicatorSkin& skin);
    static void ValidateSliderSkin(const PSliderSkin& skin);
    static void ValidateScrollBarSkin(const PScrollBarSkin& skin);
    static void ValidateTabViewSkin(const PTabViewSkin& skin);
    static void ValidateDropdownMenuSkin(const PDropdownMenuSkin& skin);
    static void ValidateGroupViewSkin(const PGroupViewSkin& skin);
    static void ValidateProgressBarSkin(const PProgressBarSkin& skin);
    static Ptr<const PBrush> LoadBackgroundBrush(const PPath& directory, PApplication* application);
    static PButtonSkin LoadButtonSkin(const PPath& directory, PApplication* application);
    static PCheckBoxSkin LoadCheckBoxSkin(const PPath& directory, PApplication* application);
    static PRadioButtonSkin LoadRadioButtonSkin(const PPath& directory, PApplication* application);
    static PSliderSkin LoadSliderSkin(const PPath& directory, PApplication* application);
    static PSliderOrientationSkin LoadSliderOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static PSliderKnobAppearance LoadSliderKnobAppearance(
        const pugi::xml_node& node,
        const PPath& directory,
        PApplication* application,
        const PPoint& anchor);
    static PScrollBarSkin LoadScrollBarSkin(const PPath& directory, PApplication* application);
    static PScrollBarOrientationSkin LoadScrollBarOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static PScrollBarSkinPart LoadScrollBarPart(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static PTabViewSkin LoadTabViewSkin(const PPath& directory, PApplication* application);
    static PDropdownMenuSkin LoadDropdownMenuSkin(const PPath& directory, PApplication* application);
    static PGroupViewSkin LoadGroupViewSkin(const PPath& directory, PApplication* application);
    static PProgressBarSkin LoadProgressBarSkin(const PPath& directory, PApplication* application);
    static PProgressBarOrientationSkin LoadProgressBarOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static Ptr<const PImage> LoadSkinImage(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static void LoadIndicatorSkin(const pugi::xml_node& root, const PPath& directory, PApplication* application, PButtonIndicatorSkin& skin);
    static Ptr<PBitmap> LoadSkinBitmap(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static PButtonSkinAppearance LoadAppearance(
        const pugi::xml_node& node,
        const PPath& directory,
        PApplication* application,
        const PPoint& imageSize = PPoint(1.0f),
        PImageLayoutMode imageLayout = PImageLayoutMode::ViewRelative);
    static void PaintIndicatorButton(
        PView& target,
        const PRect& frame,
        bool isEnabled,
        PButtonStyleData& data,
        const PButtonIndicatorSkin& skin,
        const PPoint& indicatorPosition,
        const PPoint& labelPosition,
        bool labelOverlapsIndicator);
    static void PaintOverlappingIndicator(
        PView& target,
        const PRect& frame,
        const PString& label,
        const PPoint& labelPosition,
        const PRect& indicatorFrame,
        Ptr<const PBrush> brush);
    static float GetSpacedExtent(float firstSize, float secondSize, float spacing);
    PPoint GetButtonImageMinimumSize() const;
    bool HasSliderSkin() const;
    SkinnedSliderStyleData& GetSliderStyleData(const PSliderStyleData& data) const;
    static PButtonSkinState GetSliderState(const PSliderStyleData& data);
    static size_t GetSliderKnobIndex(const PSliderStyleData& data);
    static uint8_t ShadeSliderComponent(uint8_t component, uint8_t shade);
    static Ptr<PImage> CreateSliderTrackImage(const PImage& source, PColor color);
    static void UpdateSliderTrackImages(
        SkinnedSliderStyleData& state,
        const PSliderOrientationSkin& skin,
        const PSliderSkinColors& colors,
        const PSliderStyleData& data,
        const PPoint& size);
    static void DrawSkinSliderKnob(
        PView& target,
        const PRect& knobFrame,
        const PRect& beforeFrame,
        const PRect& afterFrame,
        const SkinnedSliderStyleData& state,
        const Ptr<const PBitmap>& bitmap);

    bool HasScrollBarSkin() const;
    const PScrollBarOrientationSkin& GetScrollBarOrientation(POrientation orientation) const;
    SkinnedScrollBarGeometry GetScrollBarGeometry(const PRect& frame, const PScrollBarStyleData& data, float relativeValue) const;
    static PRect GetScrollBarPartFrame(const SkinnedScrollBarGeometry& geometry, PScrollBarPart part);
    static PButtonSkinState GetScrollBarState(const PScrollBarStyleData& data, PScrollBarPart part);
    static Ptr<const PBrush> GetScrollBarBrush(SkinnedScrollBarPart& cache, const PScrollBarSkinPart& part, PButtonSkinState state, const PPoint& size);
    static void PaintScrollBarPart(PView& target, const PRect& frame, const PScrollBarSkinPart& part, SkinnedScrollBarPart& cache, PButtonSkinState state);
    static void ClearScrollBarTrack(PView& target, const SkinnedScrollBarGeometry& geometry);

    SkinnedTabViewStyleData& GetTabViewStyleData(const PView& target, const PTabViewStyleData& data) const;
    SkinnedTabViewGeometry GetSkinnedTabViewGeometry(const PView& target, const PTabViewStyleData& data) const;
    PRect GetSkinnedTabFrame(const SkinnedTabViewStyleData& state, const SkinnedTabViewGeometry& geometry, size_t index, bool selected) const;
    static PTabSkinState GetTabViewState(const PTabViewStyleData& data, size_t index);
    static Ptr<const PBrush> GetSkinBrush(SkinnedImage& cache, const PImage& source, const PPoint& size);
    void PaintSkinnedTab(
        PView& target,
        const PTabViewStyleData& data,
        SkinnedTabViewStyleData& state,
        const SkinnedTabViewGeometry& geometry,
        size_t index) const;
    static void PaintTabViewOverflow(PView& target, const PRect& frame, const Ptr<const PBitmap>& bitmap);

    static PButtonSkinState GetDropdownMenuState(const PDropdownMenuStyleData& data);
    PRect GetDropdownMenuArrowFrame(const PRect& frame) const;
    static PRect InsetSkinFrame(const PRect& frame, const PRect& insets);
    static SkinnedDropdownMenuStyleData& GetDropdownMenuStyleData(const PView& target, const PDropdownMenuStyleData& data);
    SkinnedDropdownMenuPopupStyleData& GetDropdownMenuPopupStyleData(const PView& target, const PDropdownMenuPopupStyleData& data) const;
    void PaintDropdownMenuRow(PView& target, const PDropdownMenuPopupStyleData& data, SkinnedDropdownMenuPopupStyleData& state, size_t index) const;

    SkinnedGroupViewStyleData& GetGroupViewStyleData(const PView& target, const PGroupViewStyleData& data) const;
    void PaintGroupViewFrame(PView& target, const PRect& updateRect, const PRect& frame, SkinnedGroupViewStyleData& state) const;

    const PProgressBarOrientationSkin& GetProgressBarOrientation(POrientation orientation) const;
    static PRect GetProgressBarFillFrame(const PRect& bounds, const PProgressBarOrientationSkin& skin, const PProgressBarStyleData& data);

    Ptr<const PBrush> m_BackgroundBrush;
    PButtonSkin m_ButtonSkin;
    PCheckBoxSkin m_CheckBoxSkin;
    PRadioButtonSkin m_RadioButtonSkin;
    PSliderSkin m_SliderSkin;
    PScrollBarSkin m_ScrollBarSkin;
    PTabViewSkin m_TabViewSkin;
    PDropdownMenuSkin m_DropdownMenuSkin;
    PGroupViewSkin m_GroupViewSkin;
    PProgressBarSkin m_ProgressBarSkin;
};
