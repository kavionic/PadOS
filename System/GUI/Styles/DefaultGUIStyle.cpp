// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/DefaultGUIStyle.h>

#include <App/Application.h>
#include <GUI/Style/GUIStyleFactory.h>
#include <GUI/Style/GUIStyleInfo.h>
#include <Utils/FactoryAutoRegistrator.h>
#include <System/ExceptionHandling.h>

static PFactoryAutoRegistrator<PDefaultGUIStyle> g_DefaultGUIStyleRegistration([]
{
    PGUIStyleFactory::Get().RegisterStyle(DEFAULT_GUI_STYLE_NAME, &PDefaultGUIStyle::GetVariants, &PDefaultGUIStyle::CreateStyle);
});

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDefaultGUIStyle::PDefaultGUIStyle(PApplication* application)
    : m_Application(ptr_weak_cast((application != nullptr) ? application : PApplication::GetCurrentApplication()))
{
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBrush> PDefaultGUIStyle::GetBackgroundBrush() const
{
    PApplication* application = ptr_raw_pointer_cast(m_Application);
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return application->GetStandardBrush(PStandardColorID::DefaultBackground);
}
