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
// Created: 15.06.2020 22:55:22

#include <sys/fcntl.h>

#include <pugixml/src/pugixml.hpp>

#include "GUI/ViewFactory.h"
#include "GUI/View.h"
#include "GUI/Checkbox.h"
#include "GUI/Slider.h"
#include "GUI/GroupView.h"
#include "Utils/XMLObjectParser.h"
#include "Kernel/VFS/FileIO.h"


using namespace os;
using namespace pugi;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#define VIEW_FACTORY_REGISTER_CLASS(CLASS) RegisterClass(#CLASS, [](Ptr<View> parent, const pugi::xml_node& xmlData) { return ptr_new<CLASS>(parent, xmlData); });
ViewFactory::ViewFactory()
{
    VIEW_FACTORY_REGISTER_CLASS(View);
//    VIEW_FACTORY_REGISTER_CLASS(Button);
    VIEW_FACTORY_REGISTER_CLASS(CheckBox);
    VIEW_FACTORY_REGISTER_CLASS(Slider);
    VIEW_FACTORY_REGISTER_CLASS(GroupView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ViewFactory& ViewFactory::GetInstance()
{
    static ViewFactory factory;
    return factory;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> ViewFactory::CreateView(Ptr<View> parentView, std::vector<char>&& XML)
{
    XMLDocument doc;

    if (!doc.Parse(std::move(XML))) {
	return nullptr;
    }

    xml_node rootNode = doc.GetDocumentElement();

    if (!rootNode) {
	return nullptr;
    }

    if (parentView == nullptr) {
	parentView = ptr_new<View>(String::zero);
    }

    Parse(parentView, rootNode);

    parentView->MergeFlags(xml_object_parser::parse_flags_attribute<uint32_t>(rootNode, ViewFlags::FlagMap, "flags", 0));
    parentView->SetLayoutNode(xml_object_parser::parse_attribute(rootNode, "layout", parentView->GetLayoutNode()));
    parentView->SetBorders(xml_object_parser::parse_attribute(rootNode, "layout_borders", parentView->GetBorders()));

    return parentView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> ViewFactory::LoadView(Ptr<View> parentView, const char* path)
{
    int file = FileIO::Open(path, O_RDONLY);
    if (file == -1) {
	return nullptr;
    }
    struct stat stats;

    if (FileIO::ReadStats(file, &stats) != -1)
    {
	std::vector<char> buffer;
	buffer.resize(stats.st_size + 1);
	if (FileIO::Read(file, buffer.data(), stats.st_size) == stats.st_size)
	{
	    buffer[stats.st_size] = '\0';
	    return CreateView(parentView, std::move(buffer));
	}
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ViewFactory::Parse(Ptr<View> parentView, const pugi::xml_node& xmlNode)
{
    for (xml_node childNode = xmlNode.first_child(); childNode; childNode = childNode.next_sibling())
    {
	Ptr<View> childView = CreateInstance<View>(childNode.name(), parentView, childNode);
	if (childNode.first_child()) {
	    Parse(childView, childNode);
	}
    }
    return true;
}
