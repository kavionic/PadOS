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

#pragma once

//#include <functional>
#include <map>
#include <string.h>

#include <pugixml/src/pugixml.hpp>

#include "String.h"

#include "Ptr/Ptr.h"

namespace os
{
class String;
class Point;
class Rect;
class LayoutNode;
enum class Alignment : uint8_t;
enum class Orientation : uint8_t;
}

namespace xml_object_parser
{

bool parse(const char* text, int& value);
bool parse(const char* text, int32_t& value);
bool parse(const char* text, uint32_t& value);
bool parse(const char* text, int64_t& value);
bool parse(const char* text, uint64_t& value);
bool parse(const char* text, float& value);
bool parse(const char* text, double& value);
bool parse(const char* text, bool& value);

bool parse(const char* text, os::String& value);
bool parse(const char* text, os::Point& value);
bool parse(const char* text, os::Rect& value);
bool parse(const char* text, Ptr<os::LayoutNode>& value);
bool parse(const char* text, os::Alignment& value);
bool parse(const char* text, os::Orientation& value);


template<typename T>
bool parse_flags(const char* text, const std::map<os::String, T>& flagDefinitions, T& value)
{
	T flags = 0;
	const char* start = text;

	for (;;)
	{
		while (*start != '\0' && (*start == '|' || isspace(*start))) ++start;
		if (*start == '\0') break;
		const char* end = strchr(start, '|');
		if (end == nullptr) end = start + strlen(start);
		while (isspace(end[-1])) --end;

		auto i = flagDefinitions.find(os::String(start, end));
		if (i != flagDefinitions.end()) {
			flags |= i->second;
		}
		start = end;
	}
	value = flags;
	return true;
}

template<typename T>
T parse_attribute(const pugi::xml_node& xmlNode, const char* name, const T& defaultValue)
{
    pugi::xml_attribute attribute = xmlNode.attribute(name);
    if (!attribute.empty())
    {
	T value;
	if (parse(attribute.value(), value)) {
	    return value;
	}
    }
    return defaultValue;
}

template<typename T>
T parse_flags_attribute(const pugi::xml_node& xmlNode, const std::map<os::String, T>& flagDefinitions, const char* name, const T& defaultValue)
{
    pugi::xml_attribute attribute = xmlNode.attribute(name);
    if (!attribute.empty())
    {
	T value;
	if (parse_flags(attribute.value(), flagDefinitions, value)) {
	    return value;
	}
    }
    return defaultValue;
}

} // namespace xml_object_parser

