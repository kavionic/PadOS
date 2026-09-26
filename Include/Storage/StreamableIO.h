// This file is part of PadOS.
//
// Copyright (c) 2001-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <sys/types.h>


///////////////////////////////////////////////////////////////////////////////
/// \ingroup storage
/// \par Description:
///
/// \sa
/// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class PStreamableIO
{
public:
    virtual ~PStreamableIO();

    virtual ssize_t Read(void* buffer, ssize_t size) = 0;
    virtual ssize_t Write(const void* buffer, ssize_t size) = 0;
    
};
