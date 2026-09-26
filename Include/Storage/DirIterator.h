// This file is part of PadOS.
//
// Copyright (c) 2001-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Utils/String.h>

class PFileReference;


///////////////////////////////////////////////////////////////////////////////
// \ingroup storage
// \par Description:
//
// \sa
// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class PDirIterator
{
public:
    virtual ~PDirIterator() {}

    virtual bool GetNextEntry(PString& outName) = 0;
    virtual bool GetNextEntry(PFileReference& outReference) = 0;
    virtual bool Rewind() = 0;
};
