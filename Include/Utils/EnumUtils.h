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
// Created: 13.11.2025 21:30

#pragma once

#include <map>


template<typename TEnum>
struct PEnumNames
{
    PEnumNames(const std::map<TEnum, const char*>& entries) : Names(entries) {}

    const char* operator[](TEnum value) const
    {
        const auto i = Names.find(value);
        if (i != Names.end()) {
            return i->second;
        } else {
            return "*unknown*";
        }
    }

    const std::map<TEnum, const char*> Names;
};

#define PENUM_ENTRY_NAME(ENUM, ENTRY) {ENUM::ENTRY, #ENTRY}
