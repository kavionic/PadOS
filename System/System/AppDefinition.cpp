// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 21.12.2025 17:00

#include <System/AppDefinition.h>

PAppDefinition* PAppDefinition::s_FirstApp = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PAppDefinition::PAppDefinition(const char* name, int (*mainEntry)(int argc, char* argv[]), size_t stackSize)
    : Name(name)
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
