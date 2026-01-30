// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 15.06.2020 22:45:22

#pragma once
#include <functional>
#include <map>
#include <string.h>

#include <pugixml.hpp>

#include "String.h"

#include "Ptr/Ptr.h"
#include "Math/Rect.h"


template<typename ...ARGS>
class PXMLFactory
{
public:
    using CreateObjectCallback = std::function<Ptr<PtrTarget>(ARGS...)>;

    virtual ~PXMLFactory() {}

    void RegisterClass(const PString& name, CreateObjectCallback factory) { m_ClassMap[name] = factory; }

    template<typename T>
    Ptr<T> CreateInstance(const PString& name, ARGS... args)
    {
        auto i = m_ClassMap.find(name);
        if (i != m_ClassMap.end())
        {
            return ptr_dynamic_cast<T>(i->second(args...));
        }
        printf("ERROR: XMLFactory no class called '%s' is registered\n", name.c_str());
        return nullptr;
    }
private:
    std::map<PString, CreateObjectCallback> m_ClassMap;
};
