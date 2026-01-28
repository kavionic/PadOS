// This file is part of PadOS.
//
// Copyright (C) 2001-2020 Kurt Skauen <http://kavionic.com/>
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

#include <Utils/String.h>

namespace os
{
class FileReference;

///////////////////////////////////////////////////////////////////////////////
// \ingroup storage
// \par Description:
//
// \sa
// \author Kurt Skauen (kurt@atheos.cx)
///////////////////////////////////////////////////////////////////////////////

class DirIterator
{
public:
    virtual ~DirIterator() {}

    virtual bool GetNextEntry(PString& outName) = 0;
    virtual bool GetNextEntry(FileReference& outReference) = 0;
    virtual bool Rewind() = 0;
};

} // namespace os
