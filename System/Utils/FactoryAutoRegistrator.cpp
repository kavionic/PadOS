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
// Created: 11.05.2025 16:00

#include <Utils/FactoryAutoRegistrator.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PFactoryAutoRegistratorBase* PFactoryAutoRegistratorBase::s_FirstRegistrator = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PFactoryAutoRegistratorBase::PFactoryAutoRegistratorBase()
{
    m_NextRegistrator = s_FirstRegistrator; s_FirstRegistrator = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PFactoryAutoRegistratorBase::InvokeAll()
{
    while (s_FirstRegistrator != nullptr)
    {
        s_FirstRegistrator->Invoke();
        s_FirstRegistrator = s_FirstRegistrator->m_NextRegistrator;
    }
}
