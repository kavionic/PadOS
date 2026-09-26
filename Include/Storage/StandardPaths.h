// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>

#include <Utils/String.h>

class PMutex;


using PStandardPathID = uint32_t;


#define PDEFINE_STANDARD_PATH_ID(ID)   static constexpr PStandardPathID ID = PString::hash_string_literal(#ID, sizeof(#ID) - 1);

class PStandardPaths
{
public:
    static bool    RegisterPath(PStandardPathID pathID, const PString& path);
    static bool    UpdatePath(PStandardPathID pathID, const PString& path);
    static PString GetPath(PStandardPathID pathID);
    static PString GetPath(PStandardPathID pathID, const PString& file);

private:
    static PMutex& GetMutex();

    static std::map<PStandardPathID, PString> s_PathMap;
};


namespace PStandardPath
{
PDEFINE_STANDARD_PATH_ID(System);
PDEFINE_STANDARD_PATH_ID(Settings);
PDEFINE_STANDARD_PATH_ID(Keyboards);
PDEFINE_STANDARD_PATH_ID(GUI);
}
