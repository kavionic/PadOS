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
