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

#include <pugixml.hpp>

#include <GUI/ViewFactory.h>
#include <GUI/View.h>
#include <GUI/Widgets/BitmapView.h>
#include <GUI/Widgets/Button.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <GUI/Widgets/Checkbox.h>
#include <GUI/Widgets/DropdownMenu.h>
#include <GUI/Widgets/ListView.h>
#include <GUI/Widgets/MVCGridView.h>
#include <GUI/Widgets/MVCListView.h>
#include <GUI/Widgets/ProgressBar.h>
#include <GUI/Widgets/RadioButton.h>
#include <GUI/Widgets/ScrollableView.h>
#include <GUI/Widgets/ScrollView.h>
#include <GUI/Widgets/Slider.h>
#include <GUI/Widgets/TabView.h>
#include <GUI/Widgets/TextBox.h>
#include <GUI/Widgets/TextView.h>
#include <GUI/Widgets/GroupView.h>
#include <Utils/XMLObjectParser.h>
#include <Storage/StandardPaths.h>
#include <Storage/File.h>


using namespace pugi;

PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PBitmapView);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PButton);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PCheckBox);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PDropdownMenu);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PGroupView);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PListView);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PMVCGridView);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PMVCListView);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PProgressBar);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PRadioButton);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PScrollableView);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PScrollView);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PSlider);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PTabView);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PTextBox);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PTextView);
PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(PView);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewFactory::PViewFactory()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewFactory& PViewFactory::Get()
{
    static PViewFactory factory;
    return factory;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PViewFactory::CreateView(Ptr<PView> parentView, PString&& XML)
{
    PXMLDocument doc;

    if (!doc.Parse(std::move(XML))) {
        return nullptr;
    }

    xml_node rootNode = doc.GetDocumentElement();

    if (!rootNode) {
        return nullptr;
    }
    PViewFactoryContext context;
    return CreateView(context, parentView, rootNode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PViewFactory::CreateView(PViewFactoryContext& context, Ptr<PView> parentView, const pugi::xml_node& xmlNode)
{
    if (!xmlNode) {
        return nullptr;
    }

    if (parentView == nullptr) {
        parentView = ptr_new<PView>(PString::zero);
    }

    parentView->MergeFlags(p_xml_object_parser::parse_flags_attribute<uint32_t>(xmlNode, PViewFlags::FlagMap, "flags", 0));
    parentView->SetLayoutNode(p_xml_object_parser::parse_attribute(xmlNode, "layout", parentView->GetLayoutNode()));
    parentView->SetBorders(p_xml_object_parser::parse_attribute(xmlNode, "layout_borders", parentView->GetBorders()));

    Parse(context, parentView, xmlNode);

    return parentView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PViewFactory::LoadView(Ptr<PView> parentView, const PString& path)
{
    PFile file(PStandardPaths::GetPath(PStandardPath::GUI, path));

    if (!file.IsValid()) {
        return nullptr;
    }
    PString buffer;
    if (!file.Read(buffer)) {
        return nullptr;
    }
    file.Close();
    return CreateView(parentView, std::move(buffer));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PViewFactory::Parse(PViewFactoryContext& context, Ptr<PView> parentView, const pugi::xml_node& xmlNode)
{
    std::vector< std::map<PString, pugi::xml_node>::iterator> localTemplates;
    for (xml_node childNode = xmlNode.first_child(); childNode; childNode = childNode.next_sibling())
    {
        if (strcmp(childNode.name(), "Template") != 0)
        {
            if (*childNode.name() != '_')
            {
                Ptr<PView> childView = CreateInstance<PView>(childNode.name(), context, parentView, childNode);
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

    Ptr<PLayoutNode> layoutNode = parentView->GetLayoutNode();
    if (layoutNode != nullptr)
    {
        PRect innerBorders = context.GetAttribute(xmlNode, "inner_borders", PRect());
        float spacing = context.GetAttribute(xmlNode, "spacing", 0.0f);

        if (spacing != 0.0f || innerBorders.left != 0.0f || innerBorders.top != 0.0f || innerBorders.right != 0.0f || innerBorders.bottom != 0.0f) {
            layoutNode->ApplyInnerBorders(innerBorders, spacing);
        }
    }
    return true;
}
