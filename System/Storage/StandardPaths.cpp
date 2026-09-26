// This file is part of PadOS.
//
// Copyright (c) 2020-2024 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <Storage/StandardPaths.h>
#include <Storage/Path.h>
#include <Threads/Mutex.h>
#include <Utils/Logging.h>


std::map<PStandardPathID, PString> PStandardPaths::s_PathMap;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PStandardPaths::RegisterPath(PStandardPathID pathID, const PString& path)
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

bool PStandardPaths::UpdatePath(PStandardPathID pathID, const PString& path)
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

PString PStandardPaths::GetPath(PStandardPathID pathID)
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

PString PStandardPaths::GetPath(PStandardPathID pathID, const PString& file)
{
    if (!file.empty() && file[0] == '/') {
        return file; // Already an absolute path.
    }
    PPath path(GetPath(pathID));
    path.Append(file);
    return path.GetPath();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMutex& PStandardPaths::GetMutex()
{
    static PMutex mutex("std_path_mutex", PEMutexRecursionMode_RaiseError);
    return mutex;
}
