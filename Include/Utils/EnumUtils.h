// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
