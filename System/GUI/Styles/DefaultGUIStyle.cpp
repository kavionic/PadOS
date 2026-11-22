// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/GUIStyleFactory.h>
#include <GUI/Style/GUIStyleInfo.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <System/ExceptionHandling.h>
#include <Utils/FactoryAutoRegistrator.h>

static PFactoryAutoRegistrator<PDefaultGUIStyle> g_DefaultGUIStyleRegistration([]
{
    PGUIStyleFactory::Get().RegisterStyle(DEFAULT_GUI_STYLE_NAME, &PDefaultGUIStyle::GetVariants, &PDefaultGUIStyle::CreateStyle);
});

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDefaultGUIStyle::PDefaultGUIStyle(PApplication* application) : PGUIStyle(application)
{
    SetColor(PGUIColorID::Foreground, PColor(0, 0, 0));
    SetColor(PGUIColorID::Background, PColor(169, 169, 169));
    SetColor(PGUIColorID::ForegroundDisabled, PColor(128, 128, 128));
    SetColor(PGUIColorID::GroupLabel, PColor(0, 0, 0));
    SetColor(PGUIColorID::ButtonLabelNormal, PColor(0, 0, 0));
    SetColor(PGUIColorID::ButtonLabelHovered, PColor(0, 0, 0));
    SetColor(PGUIColorID::ButtonLabelPressed, PColor(0, 0, 0));
    SetColor(PGUIColorID::ButtonLabelDisabled, PColor(128, 128, 128));
    SetColor(PGUIColorID::ButtonLabelCheckedNormal, PColor(0, 0, 0));
    SetColor(PGUIColorID::ButtonLabelCheckedHovered, PColor(0, 0, 0));
    SetColor(PGUIColorID::ButtonLabelCheckedPressed, PColor(0, 0, 0));
    SetColor(PGUIColorID::ButtonLabelCheckedDisabled, PColor(128, 128, 128));
    SetColor(PGUIColorID::CheckBoxLabelNormal, PColor(0, 0, 0));
    SetColor(PGUIColorID::CheckBoxLabelHovered, PColor(0, 0, 0));
    SetColor(PGUIColorID::CheckBoxLabelPressed, PColor(0, 0, 0));
    SetColor(PGUIColorID::CheckBoxLabelDisabled, PColor(128, 128, 128));
    SetColor(PGUIColorID::CheckBoxLabelCheckedNormal, PColor(0, 0, 0));
    SetColor(PGUIColorID::CheckBoxLabelCheckedHovered, PColor(0, 0, 0));
    SetColor(PGUIColorID::CheckBoxLabelCheckedPressed, PColor(0, 0, 0));
    SetColor(PGUIColorID::CheckBoxLabelCheckedDisabled, PColor(128, 128, 128));
    SetColor(PGUIColorID::RadioButtonLabelNormal, PColor(0, 0, 0));
    SetColor(PGUIColorID::RadioButtonLabelHovered, PColor(0, 0, 0));
    SetColor(PGUIColorID::RadioButtonLabelPressed, PColor(0, 0, 0));
    SetColor(PGUIColorID::RadioButtonLabelDisabled, PColor(128, 128, 128));
    SetColor(PGUIColorID::RadioButtonLabelCheckedNormal, PColor(0, 0, 0));
    SetColor(PGUIColorID::RadioButtonLabelCheckedHovered, PColor(0, 0, 0));
    SetColor(PGUIColorID::RadioButtonLabelCheckedPressed, PColor(0, 0, 0));
    SetColor(PGUIColorID::RadioButtonLabelCheckedDisabled, PColor(128, 128, 128));
    SetColor(PGUIColorID::TabLabelNormal, PColor(0, 0, 0));
    SetColor(PGUIColorID::TabLabelHovered, PColor(0, 0, 0));
    SetColor(PGUIColorID::TabLabelPressed, PColor(0, 0, 0));
    SetColor(PGUIColorID::TabLabelSelectedNormal, PColor(0, 0, 0));
    SetColor(PGUIColorID::TabLabelSelectedHovered, PColor(0, 0, 0));
    SetColor(PGUIColorID::TabLabelSelectedPressed, PColor(0, 0, 0));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PDefaultGUIStyle::GetName()
{
    return DEFAULT_GUI_STYLE_NAME;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<PString> PDefaultGUIStyle::GetVariants()
{
    return {DEFAULT_GUI_STYLE_VARIANT};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PGUIStyle> PDefaultGUIStyle::CreateStyle(PApplication* application, const PString& variant)
{
    if (variant != DEFAULT_GUI_STYLE_VARIANT) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return ptr_new<PDefaultGUIStyle>(application);
}
