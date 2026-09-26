// This file is part of PadOS.
//
// Copyright (c) 2001-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Storage/File.h>


///////////////////////////////////////////////////////////////////////////////
/// \ingroup storage
/// \par Description:
///
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class PTempFile : public PFile
{
public:
    PTempFile(const PString& prefix = PString::zero, const PString& path = PString::zero, int access = S_IRUSR | S_IWUSR );
    ~PTempFile();

    void    Detatch();
    bool    Unlink();

    PString GetPath() const;
    
private:
    PString m_Path;
    bool    m_DeleteFile;
};
