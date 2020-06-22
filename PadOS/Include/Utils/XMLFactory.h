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
// Created: 15.06.2020 22:45:22

#pragma once
#include <functional>
#include <map>
#include <string.h>

#include <pugixml/src/pugixml.hpp>

#include "String.h"

#include "Ptr/Ptr.h"
#include "Math/Rect.h"
/*
namespace xml_object_parser
{

inline bool parse(const char* text, os::String& value) { value = text; return true; }
inline bool parse(const char* text, Rect& value) { return sscanf(text, "Rect( %f , %f , %f , %f )", &value.left, &value.top, &value.right, &value.bottom) == 4; }
inline bool parse(const char* text, int& value) { return sscanf(text, " %d ", &value) == 1; }
inline bool parse(const char* text, int32_t& value) { return sscanf(text, " %ld ", &value) == 1; }
inline bool parse(const char* text, uint32_t& value) { return sscanf(text, " %lu ", &value) == 1; }
inline bool parse(const char* text, int64_t& value) { return sscanf(text, " %lld ", &value) == 1; }
inline bool parse(const char* text, uint64_t& value) { return sscanf(text, " %llu ", &value) == 1; }
inline bool parse(const char* text, float& value) { return sscanf(text, " %f ", &value) == 1; }
inline bool parse(const char* text, double& value) { return sscanf(text, " %lf ", &value) == 1; }
inline bool parse(const char* text, bool& value)
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

inline bool parse(const char* text, Ptr<os::LayoutNode>& value)
{
    if (strcmp(text.value(), "stacked") == 0) {
	SetLayoutNode(ptr_new<LayoutNode>());
    } else if (strcmp(text.value(), "horizontal") == 0) {
	SetLayoutNode(ptr_new<HLayoutNode>());
    } else if (strcmp(text.value(), "vertical") == 0) {
	SetLayoutNode(ptr_new<VLayoutNode>());
    } else {
	printf("ERROR: View - invalid layout mode %s\n", layout.value());
    }
}

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

}
*/
namespace os
{



template<typename ...ARGS>
class XMLFactory
{
public:
    using CreateObjectCallback = std::function<Ptr<PtrTarget>(ARGS...)>;

    virtual ~XMLFactory() {}

    void RegisterClass(const String& name, CreateObjectCallback factory) { m_ClassMap[name] = factory; }

    template<typename T>
    Ptr<T> CreateInstance(const String& name, ARGS... args)
    {
	auto i = m_ClassMap.find(name);
	if (i != m_ClassMap.end())
	{
	    return ptr_dynamic_cast<T>(i->second(args...));
	}
	return nullptr;
    }
private:
    std::map<String, CreateObjectCallback> m_ClassMap;
};


}
