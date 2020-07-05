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
// Created: 17.06.2020 23:45:32


#include "Utils/XMLObjectParser.h"
#include "Math/Rect.h"
#include "GUI/LayoutNode.h"
#include "GUI/GUIDefines.h"

using namespace os;

namespace xml_object_parser
{

bool parse(const char* text, int& value)	{ return sscanf(text, " %d ", &value) == 1; }
bool parse(const char* text, int32_t& value)	{ return sscanf(text, " %ld ", &value) == 1; }
bool parse(const char* text, uint32_t& value)	{ return sscanf(text, " %lu ", &value) == 1; }
bool parse(const char* text, int64_t& value)	{ return sscanf(text, " %lld ", &value) == 1; }
bool parse(const char* text, uint64_t& value)	{ return sscanf(text, " %llu ", &value) == 1; }
bool parse(const char* text, float& value)	{ return sscanf(text, " %f ", &value) == 1; }
bool parse(const char* text, double& value)	{ return sscanf(text, " %lf ", &value) == 1; }

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, bool& value)
{
	if (strcmp(text, "false") == 0 || strcmp(text, "0") == 0) {
		value = false;
		return true;
	} else if (strcmp(text, "true") == 0 || strcmp(text, "1") == 0) {
		value = true;
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, String& value)
{
	value = text; return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, Point& value)
{
    return sscanf(text, "Point( %f , %f )", &value.x, &value.y) == 2;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, Rect& value)
{
	return sscanf(text, "Rect( %f , %f , %f , %f )", &value.left, &value.top, &value.right, &value.bottom) == 4;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, Ptr<LayoutNode>& value)
{
    if (*text != '\0')
    {
	if (strcmp(text, "stacked") == 0) {
	    value = ptr_new<LayoutNode>();
	    return true;
	} else if (strcmp(text, "horizontal") == 0) {
	    value = ptr_new<HLayoutNode>();
	    return true;
	} else if (strcmp(text, "vertical") == 0) {
	    value = ptr_new<VLayoutNode>();
	    return true;
	} else {
	    printf("ERROR: View - invalid layout mode %s\n", text);
	    value = nullptr;
	    return false;
	}
    }
    else
    {
	value = nullptr;
	return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, Alignment& value)
{
	if (*text != '\0')
	{
		if (strcmp(text, "left") == 0) {
			value = Alignment::Left;
			return true;
		} else if (strcmp(text, "right") == 0) {
			value = Alignment::Right;
			return true;
		} else if (strcmp(text, "top") == 0) {
			value = Alignment::Top;
			return true;
		} else if (strcmp(text, "bottom") == 0) {
			value = Alignment::Bottom;
			return true;
		} else if (strcmp(text, "center") == 0) {
			value = Alignment::Center;
			return true;
		} else {
			printf("ERROR: View - invalid layout mode '%s'\n", text);
			value = Alignment::Left;
			return false;
		}
	} else
	{
		value = Alignment::Left;
		return true;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, Orientation& value)
{
	if (*text != '\0')
	{
		if (strcmp(text, "horizontal") == 0) {
			value = Orientation::Horizontal;
			return true;
		} else if (strcmp(text, "vertical") == 0) {
			value = Orientation::Vertical;
			return true;
		} else {
			printf("ERROR: View - invalid orientation '%s'\n", text);
			value = Orientation::Horizontal;
			return false;
		}
	}
	else
	{
		value = Orientation::Horizontal;
		return true;
	}
}

}
