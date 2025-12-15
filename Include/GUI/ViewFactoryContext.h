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
// Created: 30.06.2020 17:49

#pragma once

#include <map>

#include <pugixml.hpp>
#include "Utils/String.h"
#include "Ptr/Ptr.h"
#include "Utils/XMLObjectParser.h"


namespace os
{

class View;
class ButtonGroup;

struct ViewFactoryContext
{

	pugi::xml_attribute GetAttribute(const pugi::xml_node& xmlNode, const char* name)
	{
		pugi::xml_attribute attribute = xmlNode.attribute(name);
		if (!attribute.empty())
		{
			return attribute;
		}
		else
		{
			// Attribute not found in xmlNode. Check if it specify a template, and search it if so.
			pugi::xml_attribute templateName = xmlNode.attribute("template");
			if (!templateName.empty())
			{
				auto i = m_Templates.upper_bound(templateName.value());
				if (i != m_Templates.begin())
				{
					--i;
					if (i->first == templateName.value()) {
						return GetAttribute(i->second, name);
					}
				}
			}
		}
		return pugi::xml_attribute();
	}

	template<typename T>
	T GetAttribute(const pugi::xml_node& xmlNode, const char* name, const T& defaultValue)
	{
		pugi::xml_attribute attribute = GetAttribute(xmlNode, name);
		if (!attribute.empty())
		{
			T value;
			if (xml_object_parser::parse(attribute.value(), value)) {
				return value;
			}
		}
		return defaultValue;
	}

	template<typename T>
	T GetFlagsAttribute(const pugi::xml_node& xmlNode, const std::map<os::String, T>& flagDefinitions, const char* name, const T& defaultValue)
	{
		String noFlagString("-none-");
		String flagString = GetAttribute(xmlNode, name, noFlagString);
		if (flagString != noFlagString)
		{
			T flags = 0;
			if (xml_object_parser::parse_flags(flagString.c_str(), flagDefinitions, flags)) {
				return flags;
			}
		}
		return defaultValue;
	}

    Ptr<ButtonGroup> GetButtonGroup(const String& name);

	std::multimap<String, pugi::xml_node>	m_Templates;
	std::map<String, Ptr<View>>				m_WidthRings;
	std::map<String, Ptr<View>>				m_HeightRings;
	std::map<String, Ptr<ButtonGroup>>		m_ButtonGroups;
};

} // namespace
