// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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


#include <Utils/XMLObjectParser.h>

#include <Math/Rect.h>
#include <GUI/LayoutNode.h>
#include <GUI/GUIDefines.h>
#include <Utils/UTF8Utils.h>


namespace p_xml_object_parser
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

bool parse(const char* text, PString& value)
{
	value = text; return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, PPoint& value)
{
    if (std::optional<PPoint> point = PPoint::FromString(text))
    {
        value = *point;
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, PRect& value)
{
	return sscanf(text, "Rect( %f , %f , %f , %f )", &value.left, &value.top, &value.right, &value.bottom) == 4;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, Ptr<PLayoutNode>& value)
{
    if (*text != '\0')
    {
	if (strcmp(text, "stacked") == 0) {
	    value = ptr_new<PLayoutNode>();
	    return true;
	} else if (strcmp(text, "horizontal") == 0) {
	    value = ptr_new<PHLayoutNode>();
	    return true;
	} else if (strcmp(text, "vertical") == 0) {
	    value = ptr_new<PVLayoutNode>();
	    return true;
	} else {
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "View - invalid layout mode {}", text);
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

bool parse(const char* text, PAlignment& value)
{
	if (*text != '\0')
	{
		if (strcmp(text, "left") == 0) {
			value = PAlignment::Left;
			return true;
		} else if (strcmp(text, "right") == 0) {
			value = PAlignment::Right;
			return true;
		} else if (strcmp(text, "top") == 0) {
			value = PAlignment::Top;
			return true;
		} else if (strcmp(text, "bottom") == 0) {
			value = PAlignment::Bottom;
			return true;
		} else if (strcmp(text, "center") == 0) {
			value = PAlignment::Center;
			return true;
        } else if (strcmp(text, "stretch") == 0) {
            value = PAlignment::Stretch;
            return true;
        } else {
            p_system_log<PLogSeverity::ERROR>(LogCat_General, "View - invalid layout mode '{}'", text);
			value = PAlignment::Left;
			return false;
		}
	}
    else
	{
		value = PAlignment::Left;
		return true;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, POrientation& value)
{
	if (*text != '\0')
	{
		if (strcmp(text, "horizontal") == 0) {
			value = POrientation::Horizontal;
			return true;
		} else if (strcmp(text, "vertical") == 0) {
			value = POrientation::Vertical;
			return true;
		} else {
            p_system_log<PLogSeverity::ERROR>(LogCat_General, "View - invalid orientation '{}'", text);
			value = POrientation::Horizontal;
			return false;
		}
	}
	else
	{
		value = POrientation::Horizontal;
		return true;
	}
}

bool parse(const char* text, PKeyCodes& value)
{
    if (text[0] == '\0')
    {
        return false;
    }
    else if (text[1] == '\0')
    {
        if ((text[0] >= 'A' && text[0] <= 'Z') || (text[0] >= '0' && text[0] <= '9') || text[0] == ' ') {
            value = PKeyCodes(text[0]);
            return true;
        }
    }
    static const std::map<PString, PKeyCodes> nameMap =
    {
        {"NONE",            PKeyCodes::NONE},
        {"CURSOR_LEFT",     PKeyCodes::CURSOR_LEFT},
        {"CURSOR_RIGHT",    PKeyCodes::CURSOR_RIGHT},
        {"CURSOR_UP",       PKeyCodes::CURSOR_UP},
        {"CURSOR_DOWN",     PKeyCodes::CURSOR_DOWN},
        {"HOME",            PKeyCodes::HOME},
        {"END",             PKeyCodes::END},
        {"DELETE",          PKeyCodes::DELETE},
        {"BACKSPACE",       PKeyCodes::BACKSPACE},
        {"TAB",             PKeyCodes::TAB},
        {"ENTER",           PKeyCodes::ENTER},
        {"SHIFT",           PKeyCodes::SHIFT},
        {"CTRL",            PKeyCodes::CTRL},
        {"ALT",             PKeyCodes::ALT},
        {"SYMBOLS",         PKeyCodes::SYMBOLS},
        {"SPACE",           PKeyCodes::SPACE},

        {"AE",              PKeyCodes::AE},
        {"OE",              PKeyCodes::OE},
        {"AA",              PKeyCodes::AA},
    };
    auto i = nameMap.find(text);
    if (i != nameMap.end()) {
        value = i->second;
    } else {
        value = PKeyCodes(utf8_to_unicode(text));
    }
    return true;
}

}
