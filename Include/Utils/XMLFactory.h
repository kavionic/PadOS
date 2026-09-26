// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
