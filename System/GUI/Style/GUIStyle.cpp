// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/ViewRenderer.h>
#include <GUI/Styles/Default/ViewRendererButtonDefault.h>
#include <GUI/Styles/Default/ViewRendererCheckBoxDefault.h>
#include <GUI/Styles/Default/ViewRendererDropdownMenuDefault.h>
#include <GUI/Styles/Default/ViewRendererDropdownMenuPopupDefault.h>
#include <GUI/Styles/Default/ViewRendererDropdownMenuPopupFrameDefault.h>
#include <GUI/Styles/Default/ViewRendererGroupViewDefault.h>
#include <GUI/Styles/Default/ViewRendererProgressBarDefault.h>
#include <GUI/Styles/Default/ViewRendererRadioButtonDefault.h>
#include <GUI/Styles/Default/ViewRendererScrollBarDefault.h>
#include <GUI/Styles/Default/ViewRendererSliderDefault.h>
#include <GUI/Styles/Default/ViewRendererTabViewDefault.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PViewRenderer> PGUIStyle::CreateDefaultRenderer(PGUIComponentID componentID)
{
    switch (componentID)
    {
        case PGUIComponentID::Button:
            return ptr_new<PViewRendererButtonDefault>();
        case PGUIComponentID::CheckBox:
            return ptr_new<PViewRendererCheckBoxDefault>();
        case PGUIComponentID::RadioButton:
            return ptr_new<PViewRendererRadioButtonDefault>();
        case PGUIComponentID::Slider:
            return ptr_new<PViewRendererSliderDefault>();
        case PGUIComponentID::ScrollBar:
            return ptr_new<PViewRendererScrollBarDefault>();
        case PGUIComponentID::DropdownMenu:
            return ptr_new<PViewRendererDropdownMenuDefault>();
        case PGUIComponentID::DropdownMenuPopupFrame:
            return ptr_new<PViewRendererDropdownMenuPopupFrameDefault>();
        case PGUIComponentID::DropdownMenuPopup:
            return ptr_new<PViewRendererDropdownMenuPopupDefault>();
        case PGUIComponentID::GroupView:
            return ptr_new<PViewRendererGroupViewDefault>();
        case PGUIComponentID::ProgressBar:
            return ptr_new<PViewRendererProgressBarDefault>();
        case PGUIComponentID::TabView:
            return ptr_new<PViewRendererTabViewDefault>();
        default:
            return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGUIStyle::SetRenderer(PGUIComponentID componentID, Ptr<const PViewRenderer> renderer)
{
    const size_t index = std::to_underlying(componentID);
    if (componentID == PGUIComponentID::None || index >= m_Renderers.size()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    m_Renderers[index] = std::move(renderer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PViewRenderer> PGUIStyle::GetRenderer(PGUIComponentID componentID) const
{
    const size_t index = std::to_underlying(componentID);
    if (componentID == PGUIComponentID::None || index >= m_Renderers.size()) {
        return nullptr;
    }
    Ptr<const PViewRenderer>& renderer = m_Renderers[index];
    if (renderer == nullptr) {
        renderer = CreateDefaultRenderer(componentID);
    }
    return renderer;
}
