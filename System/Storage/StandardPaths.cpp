// This file is part of PadOS.
//
// Copyright (C) 2020-2024 Kurt Skauen <http://kavionic.com/>
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

#include <Storage/StandardPaths.h>
#include <Storage/Path.h>
#include <Threads/Mutex.h>
#include <Utils/Logging.h>


namespace os
{

std::map<StandardPathID, PString> StandardPaths::s_PathMap;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool StandardPaths::RegisterPath(StandardPathID pathID, const PString& path)
{
    CRITICAL_SCOPE(GetMutex());

    if (s_PathMap.find(pathID) != s_PathMap.end())
    {
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "StandardPaths::RegisterPath() path already registered (hash collision?): {} : '{}'", pathID, path);
        return false;
    }
    s_PathMap[pathID] = path;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool StandardPaths::UpdatePath(StandardPathID pathID, const PString& path)
{
    CRITICAL_SCOPE(GetMutex());

    auto i = s_PathMap.find(pathID);
    if (i == s_PathMap.end())
    {
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "StandardPaths::UpdatePath() path not registered: {} : '{}'", pathID, path);
        return false;
    }
    i->second = path;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString StandardPaths::GetPath(StandardPathID pathID)
{
    CRITICAL_SCOPE(GetMutex());

    auto i = s_PathMap.find(pathID);
    if (i != s_PathMap.end()) {
        return i->second;
    }
    else {
        return PString::zero;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString StandardPaths::GetPath(StandardPathID pathID, const PString& file)
{
    if (!file.empty() && file[0] == '/') {
        return file; // Already an absolute path.
    }
    Path path(GetPath(pathID));
    path.Append(file);
    return path.GetPath();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Mutex& StandardPaths::GetMutex()
{
    static Mutex mutex("std_path_mutex", PEMutexRecursionMode_RaiseError);
    return mutex;
}

} // namespace os
