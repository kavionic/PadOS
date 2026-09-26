// This file is part of PadOS.
//
// Copyright (c) 2001-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <System/Types.h>
#include <Storage/StreamableIO.h>


///////////////////////////////////////////////////////////////////////////////
/// \ingroup storage
/// \par Description:
///
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class PSeekableIO : public PStreamableIO
{
public:
    virtual ~PSeekableIO();
    
    virtual ssize_t ReadPos(off64_t position, void* buffer, ssize_t size) const = 0;
    virtual ssize_t WritePos(off64_t position, const void* buffer, ssize_t size) = 0;

    virtual off64_t Seek(off64_t position, int mode) = 0;

};
