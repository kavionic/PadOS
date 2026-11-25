// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <string_view>

#include <App/Application.h>
#include <GUI/Bitmap.h>
#include <GUI/Style/GUIStyleFactory.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Styles/Skinned/ViewRendererButtonSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererCheckBoxSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererDropdownMenuPopupFrameSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererDropdownMenuPopupSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererDropdownMenuSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererGroupViewSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererProgressBarSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererRadioButtonSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererScrollBarSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererSliderSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererTabViewSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererTextBoxSkinned.h>
#include <GUI/View.h>
#include <GUI/ViewFactory.h>
#include <Math/Misc.h>
#include <Storage/Directory.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>
#include <Utils/FactoryAutoRegistrator.h>
#include <Utils/XMLObjectParser.h>

static PFactoryAutoRegistrator<PSkinnableGUIStyle> g_SkinnableGUIStyleRegistration([]
{
    PGUIStyleFactory::Get().RegisterStyle(PSkinnableGUIStyle::GetName(), &PSkinnableGUIStyle::GetVariants, &PSkinnableGUIStyle::CreateStyle);
});

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename ValueType> static ValueType ParseSkinAttribute(const pugi::xml_node& node, const char* name)
{
    const pugi::xml_attribute attribute = node.attribute(name);
    ValueType value;
    if (!attribute || !p_xml_object_parser::parse(attribute.value(), value)) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Invalid skin attribute '{}' on '{}'", name, node.name()));
    }
    return value;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSliderKnobAppearance::GetBounds() const
{
    if (Bitmap != nullptr) {
        return PRect::FromSize(PPoint(Bitmap->GetBounds().Size())) - Anchor.GetRounded();
    } else {
        return PRect();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSliderKnobSkin::GetBounds() const
{
    PRect bounds;
    for (const PSliderKnobAppearance& appearance : States)
    {
        const PRect frame = appearance.GetBounds();
        if (frame.IsValid()) {
            bounds = bounds.IsValid() ? bounds | frame : frame;
        }
    }
    const PRect shadowBounds = Shadow.GetBounds();
    if (shadowBounds.IsValid()) {
        bounds = bounds.IsValid() ? bounds | shadowBounds : shadowBounds;
    }
    return bounds;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(const PPath& directory, PApplication* application) : PGUIStyle(application)
{
    if (application == nullptr) {
        application = PApplication::GetCurrentApplication();
    }
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    LoadStyle(directory, application);
    const Ptr<const PBrush> background = GetBrush(PGUIColorID::Background);
    const PButtonSkin buttonSkin = LoadButtonSkin(directory, application);
    const PCheckBoxSkin checkBoxSkin = LoadCheckBoxSkin(directory, application);
    const PRadioButtonSkin radioButtonSkin = LoadRadioButtonSkin(directory, application);
    const PSliderSkin sliderSkin = LoadSliderSkin(directory, application);
    const PScrollBarSkin scrollBarSkin = LoadScrollBarSkin(directory, application);
    const PTabViewSkin tabViewSkin = LoadTabViewSkin(directory, application);
    const PDropdownMenuSkin dropdownMenuSkin = LoadDropdownMenuSkin(directory, application);
    const PTextBoxSkin textBoxSkin = LoadTextBoxSkin(directory, application);
    const PGroupViewSkin groupViewSkin = LoadGroupViewSkin(directory, application);
    const PProgressBarSkin progressBarSkin = LoadProgressBarSkin(directory, application);

    if (buttonSkin.LabelPadding.left < 0.0f || buttonSkin.LabelPadding.top < 0.0f
        || buttonSkin.LabelPadding.right < 0.0f || buttonSkin.LabelPadding.bottom < 0.0f) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    ValidateIndicatorSkin(checkBoxSkin);
    ValidateIndicatorSkin(radioButtonSkin);
    ValidateSliderSkin(sliderSkin);
    ValidateScrollBarSkin(scrollBarSkin);
    ValidateTabViewSkin(tabViewSkin);
    ValidateDropdownMenuSkin(dropdownMenuSkin);
    ValidateTextBoxSkin(textBoxSkin);
    ValidateGroupViewSkin(groupViewSkin);
    ValidateProgressBarSkin(progressBarSkin);
    if (!PMath::is_finite(radioButtonSkin.LabelVerticalSpacing)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (buttonSkin.States[0].Background != nullptr) {
        SetRenderer(PGUIComponentID::Button, ptr_new<PViewRendererButtonSkinned>(buttonSkin));
    }
    if (checkBoxSkin.States[0].Background != nullptr) {
        SetRenderer(PGUIComponentID::CheckBox, ptr_new<PViewRendererCheckBoxSkinned>(checkBoxSkin));
    }
    if (radioButtonSkin.States[0].Background != nullptr) {
        SetRenderer(PGUIComponentID::RadioButton, ptr_new<PViewRendererRadioButtonSkinned>(radioButtonSkin));
    }
    if (sliderSkin.Horizontal.Track != nullptr && sliderSkin.Vertical.Track != nullptr) {
        SetRenderer(PGUIComponentID::Slider, ptr_new<PViewRendererSliderSkinned>(sliderSkin));
    }
    if (scrollBarSkin.Horizontal.Track.States[0].Image != nullptr && scrollBarSkin.Vertical.Track.States[0].Image != nullptr) {
        SetRenderer(PGUIComponentID::ScrollBar, ptr_new<PViewRendererScrollBarSkinned>(scrollBarSkin));
    }
    if (dropdownMenuSkin.States[0].Background != nullptr) {
        SetRenderer(PGUIComponentID::DropdownMenu, ptr_new<PViewRendererDropdownMenuSkinned>(dropdownMenuSkin));
    }
    if (dropdownMenuSkin.PopupFrame != nullptr) {
        SetRenderer(PGUIComponentID::DropdownMenuPopupFrame, ptr_new<PViewRendererDropdownMenuPopupFrameSkinned>(dropdownMenuSkin, background));
    }
    if (dropdownMenuSkin.PopupFrame != nullptr) {
        SetRenderer(PGUIComponentID::DropdownMenuPopup, ptr_new<PViewRendererDropdownMenuPopupSkinned>(dropdownMenuSkin));
    }
    if (textBoxSkin.States[0] != nullptr) {
        SetRenderer(PGUIComponentID::TextBox, ptr_new<PViewRendererTextBoxSkinned>(textBoxSkin));
    }
    if (groupViewSkin.Frame != nullptr) {
        SetRenderer(PGUIComponentID::GroupView, ptr_new<PViewRendererGroupViewSkinned>(groupViewSkin));
    }
    if (progressBarSkin.Horizontal.Track != nullptr) {
        SetRenderer(PGUIComponentID::ProgressBar, ptr_new<PViewRendererProgressBarSkinned>(progressBarSkin));
    }
    if (tabViewSkin.Frame != nullptr) {
        SetRenderer(PGUIComponentID::TabView, ptr_new<PViewRendererTabViewSkinned>(tabViewSkin, background));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PSkinnableGUIStyle::GetName()
{
    return "Skinned";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<PGUIStyleVariantInfo> PSkinnableGUIStyle::GetVariants()
{
    const PPath stylesPath(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/{}", GetName())));
    PDirectory directory(stylesPath.GetPath());
    if (!directory.IsValid())
    {
        if (errno != ENOENT) {
            PERROR_THROW_CODE(PErrorCode(errno));
        }
        return {};
    }
    std::vector<PGUIStyleVariantInfo> variants;
    PString name;
    for (;;)
    {
        errno = 0;
        if (!directory.GetNextEntry(name))
        {
            if (errno != 0) {
                PERROR_THROW_CODE(PErrorCode(errno));
            }
            break;
        }
        if (name != "." && name != "..")
        {
            const PString stylePath = name + "/Style.xml";
            struct stat fileStat;
            if (fstatat(directory.GetFileDescriptor(), stylePath.c_str(), &fileStat, 0) == 0 && S_ISREG(fileStat.st_mode))
            {
                PPath variantPath(stylesPath);
                variantPath.Append(name);
                variants.push_back(LoadVariantInfo(name, variantPath));
            }
        }
    }
    std::ranges::sort(variants, {}, &PGUIStyleVariantInfo::Name);
    return variants;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PGUIStyle> PSkinnableGUIStyle::CreateStyle(PApplication* application, const PString& variant)
{
    const std::vector<PGUIStyleVariantInfo> variants = GetVariants();
    if (std::ranges::find(variants, variant, &PGUIStyleVariantInfo::Name) == variants.end()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/{}/{}", GetName(), variant)));
    return ptr_new<PSkinnableGUIStyle>(directory, application);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGUIStyleVariantInfo PSkinnableGUIStyle::LoadVariantInfo(const PString& variant, const PPath& directory)
{
    PPath path(directory);
    path.Append("Style.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load style '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    PGUIStyleVariantInfo info{variant, root.attribute("display_name").as_string(), PString::zero};
    if (std::string_view(root.name()) != "Style" || info.DisplayName.empty()) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Style.xml must have a Style root element with a display_name");
    }
    const PString preview = root.attribute("preview").as_string();
    if (!preview.empty())
    {
        PPath previewPath(directory);
        previewPath.Append(preview);
        info.PreviewPath = previewPath.GetPath();
    }
    return info;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateIndicatorSkin(const PButtonIndicatorSkin& skin)
{
    if (!PMath::is_finite(skin.Size) || skin.Size <= 0.0f || !PMath::is_finite(skin.LabelSpacing) || !PMath::is_finite(skin.LabelVerticalOffset)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateSliderSkin(const PSliderSkin& skin)
{
    for (const PSliderOrientationSkin* orientation : {&skin.Horizontal, &skin.Vertical})
    {
        for (const PSliderKnobSkin& knob : orientation->Knobs)
        {
            for (const PSliderKnobAppearance& appearance : knob.States)
            {
                if (!PMath::is_finite(appearance.Anchor.x) || !PMath::is_finite(appearance.Anchor.y)) {
                    PERROR_THROW_CODE(PErrorCode::INVAL);
                }
            }
            if (!PMath::is_finite(knob.Shadow.Anchor.x) || !PMath::is_finite(knob.Shadow.Anchor.y)) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateScrollBarSkin(const PScrollBarSkin& skin)
{
    for (const PScrollBarOrientationSkin* orientation : {&skin.Horizontal, &skin.Vertical})
    {
        if (!PMath::is_finite(orientation->Thickness) || orientation->Thickness <= 0.0f
            || !PMath::is_finite(orientation->ArrowLength) || orientation->ArrowLength < 0.0f
            || !PMath::is_finite(orientation->MinimumKnobLength) || orientation->MinimumKnobLength <= 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Invalid scrollbar dimensions");
        }
        const PRect& insets = orientation->KnobInsets;
        for (float inset : {insets.left, insets.top, insets.right, insets.bottom})
        {
            if (!PMath::is_finite(inset) || inset < 0.0f) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Scrollbar knob insets must be finite and nonnegative");
            }
        }
        for (const PScrollBarSkinPart* part : {&orientation->Track, &orientation->Knob, &orientation->Decrement, &orientation->Increment})
        {
            if (orientation->Track.States[0].Image != nullptr && part->States[0].Image == nullptr) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Scrollbar parts require a normal image");
            }
            for (const PScrollBarSkinAppearance& appearance : part->States)
            {
                if (!PMath::is_finite(appearance.GlyphOffset.x) || !PMath::is_finite(appearance.GlyphOffset.y)) {
                    PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Scrollbar glyph offsets must be finite");
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateTabViewSkin(const PTabViewSkin& skin)
{
    const PRect& insets = skin.FrameInsets;
    for (float inset : {insets.left, insets.top, insets.right, insets.bottom})
    {
        if (!PMath::is_finite(inset) || inset < 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Invalid tab-view frame inset");
        }
    }
    const PRect& padding = skin.LabelPadding;
    for (float value : {padding.left, padding.top, padding.right, padding.bottom, skin.TabSpacing, skin.InactiveTopInset})
    {
        if (!PMath::is_finite(value)) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Tab-view spacing must be finite");
        }
    }
    if (!PMath::is_finite(skin.MinimumTabSize.x) || !PMath::is_finite(skin.MinimumTabSize.y)
        || skin.MinimumTabSize.x <= 0.0f || skin.MinimumTabSize.y <= 0.0f
        || !PMath::is_finite(skin.OverflowWidth) || skin.OverflowWidth < 0.0f) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Invalid tab-view dimensions");
    }
    for (const auto* appearances : {&skin.States, &skin.SelectedStates})
    {
        if (skin.Frame != nullptr && (*appearances)[0].Image == nullptr) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Tab-view skins require normal and selected tab images");
        }
        for (const PTabSkinAppearance& appearance : *appearances)
        {
            if (!PMath::is_finite(appearance.LabelOffset.x) || !PMath::is_finite(appearance.LabelOffset.y)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Tab label offsets must be finite");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateDropdownMenuSkin(const PDropdownMenuSkin& skin)
{
    for (const PRect& insets : {skin.EditorInsets, skin.PopupInsets, skin.RowPadding})
    {
        for (float value : {insets.left, insets.top, insets.right, insets.bottom})
        {
            if (!PMath::is_finite(value)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown spacing must be finite");
            }
        }
    }
    for (float value : {skin.MinimumSize.x, skin.MinimumSize.y, skin.ArrowWidth, skin.MinimumRowHeight})
    {
        if (!PMath::is_finite(value) || value <= 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown dimensions must be finite and positive");
        }
    }
    if (skin.States[0].Background != nullptr)
    {
        for (const auto* appearances : {&skin.States, &skin.EditableStates})
        {
            for (const PDropdownMenuAppearance& appearance : *appearances)
            {
                if (appearance.Background == nullptr || appearance.Arrow == nullptr) {
                    PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown states require field and arrow appearances");
                }
                if (!PMath::is_finite(appearance.ArrowOffset.x) || !PMath::is_finite(appearance.ArrowOffset.y)) {
                    PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown arrow offsets must be finite");
                }
            }
        }
        if (skin.PopupFrame == nullptr) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown skins require a popup frame");
        }
    }
    if (skin.PopupFrame != nullptr)
    {
        if (skin.PopupBackground == nullptr) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown popups require a background");
        }
        for (const PDropdownMenuRowAppearance& appearance : skin.Rows)
        {
            if (appearance.Image == nullptr || !PMath::is_finite(appearance.LabelOffset.x) || !PMath::is_finite(appearance.LabelOffset.y)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Invalid dropdown popup row appearance");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateTextBoxSkin(const PTextBoxSkin& skin)
{
    const PRect& insets = skin.EditorInsets;
    for (float value : {insets.left, insets.top, insets.right, insets.bottom})
    {
        if (!PMath::is_finite(value) || value < 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Text box editor insets must be finite and nonnegative");
        }
    }
    for (float value : {skin.MinimumSize.x, skin.MinimumSize.y})
    {
        if (!PMath::is_finite(value) || value <= 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Text box minimum dimensions must be finite and positive");
        }
    }
    for (size_t index = 0; index < skin.States.size(); ++index)
    {
        if (skin.States[index] == nullptr || skin.RaisedStates[index] == nullptr) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Text box skins require all recessed and raised states");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateGroupViewSkin(const PGroupViewSkin& skin)
{
    for (const PRect& spacing : {skin.FrameInsets, skin.LabelPadding})
    {
        for (float value : {spacing.left, spacing.top, spacing.right, spacing.bottom})
        {
            if (!PMath::is_finite(value)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Group view spacing must be finite");
            }
        }
    }
    if (!PMath::is_finite(skin.LabelGap)) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Group view label gap must be finite");
    }
    for (float value : {skin.MinimumSize.x, skin.MinimumSize.y})
    {
        if (!PMath::is_finite(value) || value <= 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Group view minimum dimensions must be finite and positive");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateProgressBarSkin(const PProgressBarSkin& skin)
{
    const bool hasImages = skin.Horizontal.Track != nullptr || skin.Horizontal.Fill != nullptr
        || skin.Vertical.Track != nullptr || skin.Vertical.Fill != nullptr;
    for (const PProgressBarOrientationSkin* orientation : {&skin.Horizontal, &skin.Vertical})
    {
        if (hasImages && (orientation->Track == nullptr || orientation->Fill == nullptr)) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Progress bar skins require a track and fill for both orientations");
        }
        const PRect& insets = orientation->FillInsets;
        for (float value : {insets.left, insets.top, insets.right, insets.bottom})
        {
            if (!PMath::is_finite(value)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Progress bar fill insets must be finite");
            }
        }
        for (float value : {orientation->MinimumLength, orientation->Thickness})
        {
            if (!PMath::is_finite(value) || value <= 0.0f) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Progress bar dimensions must be finite and positive");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::LoadStyle(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("Style.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load style '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "Style") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Style.xml must have a Style root element");
    }
    LoadPalette(root);
    const PString texture = root.attribute("background_texture").as_string();
    if (!texture.empty())
    {
        PPath texturePath(directory);
        texturePath.Append(texture);
        Ptr<const PBitmap> bitmap;
        try
        {
            bitmap = application->LoadBitmap(texturePath);
        }
        // Keep the palette background when the optional texture cannot be loaded.
        PERROR_CATCH([](PErrorCode) {});
        if (bitmap != nullptr) {
            SetBrush(PGUIColorID::Background, ptr_new<PBrush>(bitmap));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::LoadPalette(const pugi::xml_node& root)
{
    SetColor(PGUIColorID::Foreground, ParseSkinAttribute<PColor>(root, "color_foreground"));
    SetColor(PGUIColorID::Background, ParseSkinAttribute<PColor>(root, "color_background"));
    SetColor(PGUIColorID::ForegroundDisabled, ParseSkinAttribute<PColor>(root, "color_foreground_disabled"));
    SetColor(PGUIColorID::GroupLabel, ParseSkinAttribute<PColor>(root, "color_group_label"));
    SetColor(PGUIColorID::ButtonLabelNormal, ParseSkinAttribute<PColor>(root, "color_button_label_normal"));
    SetColor(PGUIColorID::ButtonLabelHovered, ParseSkinAttribute<PColor>(root, "color_button_label_hovered"));
    SetColor(PGUIColorID::ButtonLabelPressed, ParseSkinAttribute<PColor>(root, "color_button_label_pressed"));
    SetColor(PGUIColorID::ButtonLabelDisabled, ParseSkinAttribute<PColor>(root, "color_button_label_disabled"));
    SetColor(PGUIColorID::ButtonLabelCheckedNormal, ParseSkinAttribute<PColor>(root, "color_button_label_checked_normal"));
    SetColor(PGUIColorID::ButtonLabelCheckedHovered, ParseSkinAttribute<PColor>(root, "color_button_label_checked_hovered"));
    SetColor(PGUIColorID::ButtonLabelCheckedPressed, ParseSkinAttribute<PColor>(root, "color_button_label_checked_pressed"));
    SetColor(PGUIColorID::ButtonLabelCheckedDisabled, ParseSkinAttribute<PColor>(root, "color_button_label_checked_disabled"));
    SetColor(PGUIColorID::CheckBoxLabelNormal, ParseSkinAttribute<PColor>(root, "color_check_box_label_normal"));
    SetColor(PGUIColorID::CheckBoxLabelHovered, ParseSkinAttribute<PColor>(root, "color_check_box_label_hovered"));
    SetColor(PGUIColorID::CheckBoxLabelPressed, ParseSkinAttribute<PColor>(root, "color_check_box_label_pressed"));
    SetColor(PGUIColorID::CheckBoxLabelDisabled, ParseSkinAttribute<PColor>(root, "color_check_box_label_disabled"));
    SetColor(PGUIColorID::CheckBoxLabelCheckedNormal, ParseSkinAttribute<PColor>(root, "color_check_box_label_checked_normal"));
    SetColor(PGUIColorID::CheckBoxLabelCheckedHovered, ParseSkinAttribute<PColor>(root, "color_check_box_label_checked_hovered"));
    SetColor(PGUIColorID::CheckBoxLabelCheckedPressed, ParseSkinAttribute<PColor>(root, "color_check_box_label_checked_pressed"));
    SetColor(PGUIColorID::CheckBoxLabelCheckedDisabled, ParseSkinAttribute<PColor>(root, "color_check_box_label_checked_disabled"));
    SetColor(PGUIColorID::RadioButtonLabelNormal, ParseSkinAttribute<PColor>(root, "color_radio_button_label_normal"));
    SetColor(PGUIColorID::RadioButtonLabelHovered, ParseSkinAttribute<PColor>(root, "color_radio_button_label_hovered"));
    SetColor(PGUIColorID::RadioButtonLabelPressed, ParseSkinAttribute<PColor>(root, "color_radio_button_label_pressed"));
    SetColor(PGUIColorID::RadioButtonLabelDisabled, ParseSkinAttribute<PColor>(root, "color_radio_button_label_disabled"));
    SetColor(PGUIColorID::RadioButtonLabelCheckedNormal, ParseSkinAttribute<PColor>(root, "color_radio_button_label_checked_normal"));
    SetColor(PGUIColorID::RadioButtonLabelCheckedHovered, ParseSkinAttribute<PColor>(root, "color_radio_button_label_checked_hovered"));
    SetColor(PGUIColorID::RadioButtonLabelCheckedPressed, ParseSkinAttribute<PColor>(root, "color_radio_button_label_checked_pressed"));
    SetColor(PGUIColorID::RadioButtonLabelCheckedDisabled, ParseSkinAttribute<PColor>(root, "color_radio_button_label_checked_disabled"));
    SetColor(PGUIColorID::TabLabelNormal, ParseSkinAttribute<PColor>(root, "color_tab_label_normal"));
    SetColor(PGUIColorID::TabLabelHovered, ParseSkinAttribute<PColor>(root, "color_tab_label_hovered"));
    SetColor(PGUIColorID::TabLabelPressed, ParseSkinAttribute<PColor>(root, "color_tab_label_pressed"));
    SetColor(PGUIColorID::TabLabelSelectedNormal, ParseSkinAttribute<PColor>(root, "color_tab_label_selected_normal"));
    SetColor(PGUIColorID::TabLabelSelectedHovered, ParseSkinAttribute<PColor>(root, "color_tab_label_selected_hovered"));
    SetColor(PGUIColorID::TabLabelSelectedPressed, ParseSkinAttribute<PColor>(root, "color_tab_label_selected_pressed"));
    SetColor(PGUIColorID::DropdownBackgroundNormal, ParseSkinAttribute<PColor>(root, "color_dropdown_background_normal"));
    SetColor(PGUIColorID::DropdownBackgroundHovered, ParseSkinAttribute<PColor>(root, "color_dropdown_background_hovered"));
    SetColor(PGUIColorID::DropdownBackgroundPressed, ParseSkinAttribute<PColor>(root, "color_dropdown_background_pressed"));
    SetColor(PGUIColorID::DropdownBackgroundDisabled, ParseSkinAttribute<PColor>(root, "color_dropdown_background_disabled"));
    SetColor(PGUIColorID::DropdownBackgroundEditableNormal, ParseSkinAttribute<PColor>(root, "color_dropdown_background_editable_normal"));
    SetColor(PGUIColorID::DropdownBackgroundEditableHovered, ParseSkinAttribute<PColor>(root, "color_dropdown_background_editable_hovered"));
    SetColor(PGUIColorID::DropdownBackgroundEditablePressed, ParseSkinAttribute<PColor>(root, "color_dropdown_background_editable_pressed"));
    SetColor(PGUIColorID::DropdownBackgroundEditableDisabled, ParseSkinAttribute<PColor>(root, "color_dropdown_background_editable_disabled"));
    SetColor(PGUIColorID::DropdownTextNormal, ParseSkinAttribute<PColor>(root, "color_dropdown_text_normal"));
    SetColor(PGUIColorID::DropdownTextHovered, ParseSkinAttribute<PColor>(root, "color_dropdown_text_hovered"));
    SetColor(PGUIColorID::DropdownTextPressed, ParseSkinAttribute<PColor>(root, "color_dropdown_text_pressed"));
    SetColor(PGUIColorID::DropdownTextDisabled, ParseSkinAttribute<PColor>(root, "color_dropdown_text_disabled"));
    SetColor(PGUIColorID::DropdownTextEditableNormal, ParseSkinAttribute<PColor>(root, "color_dropdown_text_editable_normal"));
    SetColor(PGUIColorID::DropdownTextEditableHovered, ParseSkinAttribute<PColor>(root, "color_dropdown_text_editable_hovered"));
    SetColor(PGUIColorID::DropdownTextEditablePressed, ParseSkinAttribute<PColor>(root, "color_dropdown_text_editable_pressed"));
    SetColor(PGUIColorID::DropdownTextEditableDisabled, ParseSkinAttribute<PColor>(root, "color_dropdown_text_editable_disabled"));
    SetColor(PGUIColorID::TextBoxBackgroundNormal, ParseSkinAttribute<PColor>(root, "color_text_box_background_normal"));
    SetColor(PGUIColorID::TextBoxBackgroundReadOnly, ParseSkinAttribute<PColor>(root, "color_text_box_background_read_only"));
    SetColor(PGUIColorID::TextBoxBackgroundDisabled, ParseSkinAttribute<PColor>(root, "color_text_box_background_disabled"));
    SetColor(PGUIColorID::TextBoxTextNormal, ParseSkinAttribute<PColor>(root, "color_text_box_text_normal"));
    SetColor(PGUIColorID::TextBoxTextReadOnly, ParseSkinAttribute<PColor>(root, "color_text_box_text_read_only"));
    SetColor(PGUIColorID::TextBoxTextDisabled, ParseSkinAttribute<PColor>(root, "color_text_box_text_disabled"));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkin PSkinnableGUIStyle::LoadButtonSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("Button.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load button skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "Button") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Button.xml must have a Button root element");
    }

    PButtonSkin skin;
    skin.LabelPadding = ParseSkinAttribute<PRect>(root, "label_padding");
    skin.PressedLabelOffset = ParseSkinAttribute<PPoint>(root, "pressed_label_offset");
    const PRect& padding = skin.LabelPadding;
    if (!PMath::is_finite(padding.left) || !PMath::is_finite(padding.top) || !PMath::is_finite(padding.right) || !PMath::is_finite(padding.bottom)) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Button label padding must be finite");
    }
    if (!PMath::is_finite(skin.PressedLabelOffset.x) || !PMath::is_finite(skin.PressedLabelOffset.y)) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Pressed button label offset must be finite");
    }

    const pugi::xml_node checked = root.child("Checked");
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        skin.States[index] = LoadAppearance(root.child(STATE_NAMES[index]), directory, application);
        skin.CheckedStates[index] = LoadAppearance(checked.child(STATE_NAMES[index]), directory, application);
    }
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBoxSkin PSkinnableGUIStyle::LoadCheckBoxSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("CheckBox.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load checkbox skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "CheckBox") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "CheckBox.xml must have a CheckBox root element");
    }

    PCheckBoxSkin skin;
    LoadIndicatorSkin(root, directory, application, skin);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRadioButtonSkin PSkinnableGUIStyle::LoadRadioButtonSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("RadioButton.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load radio button skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "RadioButton") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "RadioButton.xml must have a RadioButton root element");
    }

    PRadioButtonSkin skin;
    skin.LabelVerticalSpacing = ParseSkinAttribute<float>(root, "label_vertical_spacing");
    LoadIndicatorSkin(root, directory, application, skin);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSliderSkin PSkinnableGUIStyle::LoadSliderSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("Slider.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load slider skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "Slider") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Slider.xml must have a Slider root element");
    }
    PSliderSkin skin;
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        const pugi::xml_node node = root.child(STATE_NAMES[index]);
        PSliderSkinColors& colors = skin.Colors[index];
        colors.Before = ParseSkinAttribute<PColor>(node, "before_color");
        colors.After = ParseSkinAttribute<PColor>(node, "after_color");
        colors.Label = ParseSkinAttribute<PColor>(node, "text_color");
        colors.Tick = ParseSkinAttribute<PColor>(node, "tick_color");
        colors.TickHighlight = ParseSkinAttribute<PColor>(node, "tick_highlight");
    }
    skin.Horizontal = LoadSliderOrientation(root.child("Horizontal"), directory, application);
    skin.Vertical = LoadSliderOrientation(root.child("Vertical"), directory, application);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSliderOrientationSkin PSkinnableGUIStyle::LoadSliderOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    PSliderOrientationSkin skin;
    const pugi::xml_node trackNode = node.child("Track");
    const Ptr<const PBitmap> bitmap = LoadSkinBitmap(trackNode, directory, application);
    if (bitmap->GetColorSpace() != PEColorSpace::RGB32 && bitmap->GetColorSpace() != PEColorSpace::RGBA32) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Slider track masks must decode to RGB32 or RGBA32");
    }
    Ptr<PImage> track = ptr_new<PImage>(bitmap);
    const PRect borders = ParseSkinAttribute<PRect>(trackNode, "borders");
    const PIRect bounds = bitmap->GetBounds();
    track->SetSliceRect(PRect(borders.left, borders.top, float(bounds.right) - borders.right, float(bounds.bottom) - borders.bottom));
    track->SetLayoutSize(PPoint(bounds.Size()), PImageLayoutMode::Absolute);
    skin.Track = track;
    const bool horizontal = std::string_view(node.name()) == "Horizontal";
    const std::array<const char*, 4> shapes = {"Plain", horizontal ? "Up" : "Left", horizontal ? "Down" : "Right", "Both"};
    for (size_t shape = 0; shape < shapes.size(); ++shape)
    {
        const pugi::xml_node shapeNode = node.child(shapes[shape]);
        const PPoint anchor = ParseSkinAttribute<PPoint>(shapeNode, "anchor");
        for (size_t state = 0; state < STATE_NAMES.size(); ++state)
        {
            skin.Knobs[shape].States[state] = LoadSliderKnobAppearance(shapeNode.child(STATE_NAMES[state]), directory, application, anchor);
        }
        skin.Knobs[shape].Shadow = LoadSliderKnobAppearance(shapeNode.child("Shadow"), directory, application, anchor);
    }
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSliderKnobAppearance PSkinnableGUIStyle::LoadSliderKnobAppearance(
    const pugi::xml_node& node,
    const PPath& directory,
    PApplication* application,
    const PPoint& anchor)
{
    PSliderKnobAppearance appearance;
    appearance.Bitmap = LoadSkinBitmap(node, directory, application);
    appearance.Anchor = node.attribute("anchor") ? ParseSkinAttribute<PPoint>(node, "anchor") : anchor;
    return appearance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarSkin PSkinnableGUIStyle::LoadScrollBarSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("ScrollBar.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load scrollbar skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "ScrollBar") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "ScrollBar.xml must have a ScrollBar root element");
    }
    PScrollBarSkin skin;
    skin.Horizontal = LoadScrollBarOrientation(root.child("Horizontal"), directory, application);
    skin.Vertical = LoadScrollBarOrientation(root.child("Vertical"), directory, application);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarOrientationSkin PSkinnableGUIStyle::LoadScrollBarOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    PScrollBarOrientationSkin skin;
    skin.Thickness = ParseSkinAttribute<float>(node, "thickness");
    skin.ArrowLength = ParseSkinAttribute<float>(node, "arrow_length");
    skin.MinimumKnobLength = ParseSkinAttribute<float>(node, "minimum_knob_length");
    skin.KnobInsets = ParseSkinAttribute<PRect>(node, "knob_insets");
    skin.DoubleArrows = ParseSkinAttribute<bool>(node, "double_arrows");
    skin.Track = LoadScrollBarPart(node.child("Track"), directory, application);
    skin.Knob = LoadScrollBarPart(node.child("Knob"), directory, application);
    skin.Decrement = LoadScrollBarPart(node.child("Decrement"), directory, application);
    skin.Increment = LoadScrollBarPart(node.child("Increment"), directory, application);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarSkinPart PSkinnableGUIStyle::LoadScrollBarPart(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    PScrollBarSkinPart part;
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        const pugi::xml_node state = node.child(STATE_NAMES[index]);
        const Ptr<const PBitmap> bitmap = LoadSkinBitmap(state, directory, application);
        const PRect borders = ParseSkinAttribute<PRect>(state, "borders");
        const PIRect bounds = bitmap->GetBounds();
        Ptr<PImage> image = ptr_new<PImage>(bitmap);
        image->SetSliceRect(PRect(borders.left, borders.top, float(bounds.right) - borders.right, float(bounds.bottom) - borders.bottom));
        image->SetLayoutSize(PPoint(bounds.Size()), PImageLayoutMode::Absolute);
        part.States[index].Image = image;
        const pugi::xml_node glyph = state.child("Glyph");
        if (glyph)
        {
            part.States[index].Glyph = LoadSkinBitmap(glyph, directory, application);
            part.States[index].GlyphOffset = ParseSkinAttribute<PPoint>(glyph, "offset");
        }
    }
    return part;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabViewSkin PSkinnableGUIStyle::LoadTabViewSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("TabView.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load tab-view skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "TabView") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "TabView.xml must have a TabView root element");
    }
    PTabViewSkin skin;
    skin.FrameInsets = ParseSkinAttribute<PRect>(root, "frame_insets");
    skin.LabelPadding = ParseSkinAttribute<PRect>(root, "label_padding");
    skin.MinimumTabSize = ParseSkinAttribute<PPoint>(root, "minimum_tab_size");
    skin.TabSpacing = ParseSkinAttribute<float>(root, "tab_spacing");
    skin.InactiveTopInset = ParseSkinAttribute<float>(root, "inactive_top_inset");
    skin.OverflowWidth = ParseSkinAttribute<float>(root, "overflow_width");
    skin.Frame = LoadSkinImage(root.child("Frame"), directory, application);
    skin.OverflowLeft = LoadSkinBitmap(root.child("OverflowLeft"), directory, application);
    skin.OverflowRight = LoadSkinBitmap(root.child("OverflowRight"), directory, application);
    for (size_t selected = 0; selected < 2; ++selected)
    {
        const pugi::xml_node group = root.child((selected == 0) ? "Tab" : "Selected");
        for (size_t index = 0; index < TAB_STATE_NAMES.size(); ++index)
        {
            const pugi::xml_node node = group.child(TAB_STATE_NAMES[index]);
            PTabSkinAppearance& appearance = (selected == 0) ? skin.States[index] : skin.SelectedStates[index];
            appearance.Image = LoadSkinImage(node, directory, application);
            appearance.LabelOffset = ParseSkinAttribute<PPoint>(node, "text_offset");
        }
    }
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDropdownMenuSkin PSkinnableGUIStyle::LoadDropdownMenuSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("DropdownMenu.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load dropdown skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "DropdownMenu") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "DropdownMenu.xml must have a DropdownMenu root element");
    }
    PDropdownMenuSkin skin;
    skin.EditorInsets = ParseSkinAttribute<PRect>(root, "editor_insets");
    skin.MinimumSize = ParseSkinAttribute<PPoint>(root, "minimum_size");
    skin.ArrowWidth = ParseSkinAttribute<float>(root, "arrow_width");
    const pugi::xml_node editable = root.child("Editable");
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        skin.States[index] = LoadDropdownMenuAppearance(root.child(STATE_NAMES[index]), directory, application);
        skin.EditableStates[index] = LoadDropdownMenuAppearance(editable.child(STATE_NAMES[index]), directory, application);
    }
    const pugi::xml_node popup = root.child("Popup");
    skin.PopupInsets = ParseSkinAttribute<PRect>(popup, "frame_insets");
    skin.RowPadding = ParseSkinAttribute<PRect>(popup, "row_padding");
    skin.MinimumRowHeight = ParseSkinAttribute<float>(popup, "minimum_row_height");
    skin.PopupBackground = ptr_new<PBrush>(ParseSkinAttribute<PColor>(popup, "background_color"), application);
    Ptr<PImage> frame = ptr_new<PImage>(*LoadSkinImage(popup.child("Frame"), directory, application));
    frame->SetLayoutSize(PPoint(1.0f), PImageLayoutMode::ViewRelative);
    skin.PopupFrame = ptr_new<PBrush>(frame);
    for (size_t index = 0; index < DROPDOWN_ROW_STATE_NAMES.size(); ++index)
    {
        const pugi::xml_node node = popup.child(DROPDOWN_ROW_STATE_NAMES[index]);
        PDropdownMenuRowAppearance& appearance = skin.Rows[index];
        appearance.Image = LoadSkinImage(node, directory, application);
        appearance.LabelColor = ParseSkinAttribute<PColor>(node, "text_color");
        appearance.LabelOffset = ParseSkinAttribute<PPoint>(node, "text_offset");
    }
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDropdownMenuAppearance PSkinnableGUIStyle::LoadDropdownMenuAppearance(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    const PButtonSkinAppearance field = LoadAppearance(node, directory, application);
    PDropdownMenuAppearance appearance;
    appearance.Background = field.Background;
    const pugi::xml_node arrow = node.child("Arrow");
    appearance.Arrow = LoadSkinBitmap(arrow, directory, application);
    appearance.ArrowOffset = ParseSkinAttribute<PPoint>(arrow, "offset");
    return appearance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextBoxSkin PSkinnableGUIStyle::LoadTextBoxSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("TextBox.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load text box skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "TextBox") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "TextBox.xml must have a TextBox root element");
    }
    PTextBoxSkin skin;
    skin.EditorInsets = ParseSkinAttribute<PRect>(root, "editor_insets");
    skin.MinimumSize = ParseSkinAttribute<PPoint>(root, "minimum_size");
    const pugi::xml_node raised = root.child("Raised");
    for (size_t index = 0; index < TEXT_BOX_STATE_NAMES.size(); ++index)
    {
        skin.States[index] = LoadAppearance(root.child(TEXT_BOX_STATE_NAMES[index]), directory, application).Background;
        skin.RaisedStates[index] = LoadAppearance(raised.child(TEXT_BOX_STATE_NAMES[index]), directory, application).Background;
    }
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGroupViewSkin PSkinnableGUIStyle::LoadGroupViewSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("GroupView.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load group view skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "GroupView") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "GroupView.xml must have a GroupView root element");
    }
    PGroupViewSkin skin;
    skin.Frame = LoadSkinImage(root.child("Frame"), directory, application);
    skin.FrameInsets = ParseSkinAttribute<PRect>(root, "frame_insets");
    skin.LabelPadding = ParseSkinAttribute<PRect>(root, "label_padding");
    skin.LabelGap = ParseSkinAttribute<float>(root, "label_gap");
    skin.MinimumSize = ParseSkinAttribute<PPoint>(root, "minimum_size");
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PProgressBarSkin PSkinnableGUIStyle::LoadProgressBarSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("ProgressBar.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load progress bar skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "ProgressBar") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "ProgressBar.xml must have a ProgressBar root element");
    }
    PProgressBarSkin skin;
    skin.Horizontal = LoadProgressBarOrientation(root.child("Horizontal"), directory, application);
    skin.Vertical = LoadProgressBarOrientation(root.child("Vertical"), directory, application);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PProgressBarOrientationSkin PSkinnableGUIStyle::LoadProgressBarOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    PProgressBarOrientationSkin skin;
    skin.Track = LoadSkinImage(node.child("Track"), directory, application);
    skin.Fill = LoadSkinImage(node.child("Fill"), directory, application);
    skin.FillInsets = ParseSkinAttribute<PRect>(node, "fill_insets");
    skin.MinimumLength = ParseSkinAttribute<float>(node, "minimum_length");
    skin.Thickness = ParseSkinAttribute<float>(node, "thickness");
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PImage> PSkinnableGUIStyle::LoadSkinImage(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    const Ptr<const PBitmap> bitmap = LoadSkinBitmap(node, directory, application);
    const PRect borders = ParseSkinAttribute<PRect>(node, "borders");
    const PIRect bounds = bitmap->GetBounds();
    Ptr<PImage> image = ptr_new<PImage>(bitmap);
    image->SetSliceRect(PRect(borders.left, borders.top, float(bounds.right) - borders.right, float(bounds.bottom) - borders.bottom));
    image->SetLayoutSize(PPoint(bounds.Size()), PImageLayoutMode::Absolute);
    return image;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::LoadIndicatorSkin(const pugi::xml_node& root, const PPath& directory, PApplication* application, PButtonIndicatorSkin& skin)
{
    skin.Size = ParseSkinAttribute<float>(root, "size");
    skin.LabelSpacing = ParseSkinAttribute<float>(root, "label_spacing");
    skin.LabelVerticalOffset = ParseSkinAttribute<float>(root, "label_vertical_offset");

    const pugi::xml_node checked = root.child("Checked");
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        skin.States[index] = LoadAppearance(root.child(STATE_NAMES[index]), directory, application, PPoint(skin.Size), PImageLayoutMode::Absolute);
        skin.CheckedStates[index] = LoadAppearance(checked.child(STATE_NAMES[index]), directory, application, PPoint(skin.Size), PImageLayoutMode::Absolute);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBitmap> PSkinnableGUIStyle::LoadSkinBitmap(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    const PString filename = ParseSkinAttribute<PString>(node, "image");
    if (filename.empty()) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Skin image filename must not be empty");
    }
    PPath path(directory);
    path.Append(filename);
    try
    {
        return application->LoadBitmap(path);
    }
    PERROR_CATCH_RET([&path](const std::exception&, PErrorCode error) -> Ptr<const PBitmap>
    {
        PERROR_THROW_CODE_WHAT(error, PString::format_string("Unable to load skin image '{}'", path.GetPath()));
    });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkinAppearance PSkinnableGUIStyle::LoadAppearance(
    const pugi::xml_node& node,
    const PPath& directory,
    PApplication* application,
    const PPoint& imageSize,
    PImageLayoutMode imageLayout)
{
    const PRect borders = ParseSkinAttribute<PRect>(node, "borders");
    Ptr<const PBitmap> bitmap = LoadSkinBitmap(node, directory, application);
    Ptr<PImage> image = ptr_new<PImage>(bitmap);
    image->SetLayoutSize(imageSize, imageLayout);
    const PIRect bounds = bitmap->GetBounds();
    image->SetSliceRect(PRect(borders.left, borders.top, float(bounds.right) - borders.right, float(bounds.bottom) - borders.bottom));

    PButtonSkinAppearance appearance;
    appearance.Background = ptr_new<PBrush>(image);
    return appearance;
}
