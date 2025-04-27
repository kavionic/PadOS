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
// Created: 15.06.2020 22:55:22

#include <sys/fcntl.h>

#include <pugixml/src/pugixml.hpp>

#include <GUI/ViewFactory.h>
#include <GUI/View.h>
#include <GUI/Widgets/Button.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <GUI/Widgets/Checkbox.h>
#include <GUI/Widgets/DropdownMenu.h>
#include <GUI/Widgets/ListView.h>
#include <GUI/Widgets/ProgressBar.h>
#include <GUI/Widgets/RadioButton.h>
#include <GUI/Widgets/ScrollView.h>
#include <GUI/Widgets/Slider.h>
#include <GUI/Widgets/TabView.h>
#include <GUI/Widgets/TextBox.h>
#include <GUI/Widgets/TextView.h>
#include <GUI/Widgets/GroupView.h>
#include <Utils/XMLObjectParser.h>
#include <Storage/StandardPaths.h>
#include <Storage/File.h>


using namespace os;
using namespace pugi;

VIEW_FACTORY_REGISTER_CLASS(Button);
VIEW_FACTORY_REGISTER_CLASS(CheckBox);
VIEW_FACTORY_REGISTER_CLASS(DropdownMenu);
VIEW_FACTORY_REGISTER_CLASS(GroupView);
VIEW_FACTORY_REGISTER_CLASS(ListView);
VIEW_FACTORY_REGISTER_CLASS(ProgressBar);
VIEW_FACTORY_REGISTER_CLASS(RadioButton);
VIEW_FACTORY_REGISTER_CLASS(Slider);
VIEW_FACTORY_REGISTER_CLASS(TabView);
VIEW_FACTORY_REGISTER_CLASS(TextBox);
VIEW_FACTORY_REGISTER_CLASS(TextView);
VIEW_FACTORY_REGISTER_CLASS(View);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ViewFactory::ViewFactory()
{

    RegisterClass("ScrollView", [](ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData)
        {
            Ptr<ScrollView> view = ptr_new<ScrollView>(context, parent, xmlData);
            Ptr<View> client = view->GetScrolledView();
            return (client != nullptr) ? client : ptr_static_cast<View>(view);
        }
    );

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ViewFactory& ViewFactory::Get()
{
    static ViewFactory factory;
    return factory;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> ViewFactory::CreateView(Ptr<View> parentView, String&& XML)
{
    XMLDocument doc;

    if (!doc.Parse(std::move(XML))) {
        return nullptr;
    }

    xml_node rootNode = doc.GetDocumentElement();

    if (!rootNode) {
        return nullptr;
    }
    ViewFactoryContext context;
    return CreateView(context, parentView, rootNode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> ViewFactory::CreateView(ViewFactoryContext& context, Ptr<View> parentView, const pugi::xml_node& xmlNode)
{
    if (!xmlNode) {
        return nullptr;
    }

    if (parentView == nullptr) {
        parentView = ptr_new<View>(String::zero);
    }

    parentView->MergeFlags(xml_object_parser::parse_flags_attribute<uint32_t>(xmlNode, ViewFlags::FlagMap, "flags", 0));
    parentView->SetLayoutNode(xml_object_parser::parse_attribute(xmlNode, "layout", parentView->GetLayoutNode()));
    parentView->SetBorders(xml_object_parser::parse_attribute(xmlNode, "layout_borders", parentView->GetBorders()));

    Parse(context, parentView, xmlNode);

    return parentView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> ViewFactory::LoadView(Ptr<View> parentView, const String& path)
{
    File file(StandardPaths::GetPath(StandardPath::GUI, path));

    if (!file.IsValid()) {
        return nullptr;
    }
    String buffer;
    if (!file.Read(buffer)) {
        return nullptr;
    }
    file.Close();
    return CreateView(parentView, std::move(buffer));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ViewFactory::Parse(ViewFactoryContext& context, Ptr<View> parentView, const pugi::xml_node& xmlNode)
{
    std::vector< std::map<String, pugi::xml_node>::iterator> localTemplates;
    for (xml_node childNode = xmlNode.first_child(); childNode; childNode = childNode.next_sibling())
    {
        if (strcmp(childNode.name(), "Template") != 0)
        {
            if (*childNode.name() != '_')
            {
                Ptr<View> childView = CreateInstance<View>(childNode.name(), context, parentView, childNode);
                if (childView != nullptr && childNode.first_child()) {
                    Parse(context, childView, childNode);
                }
            }
        }
        else
        {
            xml_attribute name = childNode.attribute("name");
            if (name)
            {
                localTemplates.push_back(context.m_Templates.emplace(name.value(), childNode));
            }
        }
    }
    for (auto i : localTemplates) {
        context.m_Templates.erase(i);
    }
    localTemplates.clear();

    Ptr<LayoutNode> layoutNode = parentView->GetLayoutNode();
    if (layoutNode != nullptr)
    {
        Rect innerBorders = context.GetAttribute(xmlNode, "inner_borders", Rect());
        float spacing = context.GetAttribute(xmlNode, "spacing", 0.0f);

        if (spacing != 0.0f || innerBorders.left != 0.0f || innerBorders.top != 0.0f || innerBorders.right != 0.0f || innerBorders.bottom != 0.0f) {
            layoutNode->ApplyInnerBorders(innerBorders, spacing);
        }
    }
    return true;
}
