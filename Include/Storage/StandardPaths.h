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

namespace os
{
class Mutex;

using StandardPathID = uint32_t;


#define DEFINE_STANDARD_PATH_ID(ID)   static constexpr StandardPathID ID = os::String::hash_string_literal(#ID, sizeof(#ID) - 1);

namespace StandardPath
{
DEFINE_STANDARD_PATH_ID(System);
DEFINE_STANDARD_PATH_ID(Keyboards);
DEFINE_STANDARD_PATH_ID(GUI);
}

class StandardPaths
{
public:
    static bool    RegisterPath(StandardPathID pathID, const String& path);
    static bool    UpdatePath(StandardPathID pathID, const String& path);
    static String  GetPath(StandardPathID pathID);
    static String  GetPath(StandardPathID pathID, const String& file);

private:
    static Mutex& GetMutex();

    static std::map<StandardPathID, String> s_PathMap;
};

} // namespace os
