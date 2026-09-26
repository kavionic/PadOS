// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <sys/types.h>
#include <fcntl.h>
#include <string>

#include <Utils/String.h>


class PDirectory;


///////////////////////////////////////////////////////////////////////////////
/// \ingroup storage
/// \par Description:
///
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class PPath
{
public:
    PPath();
    PPath(const PPath& path);
    PPath(const PString& path);
    PPath(const char* path, size_t length);
    ~PPath();

    void operator =(const PPath& path);
    void operator =(const PString& path);
    bool operator==(const PPath& path) const;

    void SetTo(const PString& path);
    void SetTo(const char* path, size_t length);
    void Append(const PPath& path);
    void Append(const PString& name);
  
    PString         GetLeaf() const;
    const PString&  GetPath() const;
    PString         GetDir() const;

    size_t          GetExtensionLength() const;
    PString         GetExtension() const;
    bool            SetExtension(const PString& extension);

    const char* c_str() const { return m_Path.c_str(); }

    bool    CreateFolders(bool includeLeaf = true, PDirectory* outLeafDirectory = nullptr, int accessMode = S_IRWXU);

    operator PString() const;

    static PString GetCWD();
private:
    void Normalize();

    PString m_Path;
    size_t  m_NameStart = 0;
};

PFORMATTER_NAMESPACE
{

template<>
struct formatter<PPath> : formatter<::std::string>
{
  auto format(const PPath& a, format_context& ctx) const {
    return formatter<std::string>::format(static_cast<const std::string&>(a), ctx);
  }
};

}
