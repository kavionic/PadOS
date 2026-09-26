// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 21.12.2025 17:00

#include <System/AppDefinition.h>

PAppDefinition* PAppDefinition::s_FirstApp = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PAppDefinition::PAppDefinition(const char* name, const char* description, int (*mainEntry)(int argc, char* argv[]), size_t stackSize)
    : Name(name)
    , Description(description)
    , MainEntry(mainEntry)
    , StackSize(stackSize)
{
    m_NextApp = s_FirstApp;
    s_FirstApp = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PAppDefinition::~PAppDefinition()
{
    for (PAppDefinition** app = &s_FirstApp; *app != nullptr; app = &(*app)->m_NextApp)
    {
        if (*app == this)
        {
            *app = m_NextApp;
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PAppDefinition* PAppDefinition::FindApplication(const char* name)
{
    for (const PAppDefinition* app = __app_definition.FirstAppPointer; app != nullptr; app = app->m_NextApp)
    {
        if (strcmp(app->Name, name) == 0) {
            return app;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<const PAppDefinition*> PAppDefinition::GetApplicationList()
{
    std::vector<const PAppDefinition*> apps;
    for (const PAppDefinition* app = __app_definition.FirstAppPointer; app != nullptr; app = app->m_NextApp)
    {
        apps.push_back(app);
    }
    return apps;
}
