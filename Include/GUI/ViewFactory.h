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

#pragma once

#include <pugixml/src/pugixml.hpp>

#include <Utils/XMLFactory.h>
#include <GUI/ViewFactoryContext.h>
#include <Storage/File.h>


namespace os
{
class String;
class View;


class XMLDocument
{
public:
    XMLDocument() { m_Parser = new pugi::xml_document(); }
    ~XMLDocument() { delete m_Parser; }

    bool Load(const String& path)
    {
        m_Buffer.clear();
        {
            File file(path);
            if (!file.IsValid()) {
                return false;
            }
            if (!file.Read(m_Buffer)) {
                return false;
            }
        }
        return m_Parser->load_buffer_inplace(&m_Buffer[0], m_Buffer.size());
    }
    bool Parse(String&& data)
    {
        m_Buffer = std::move(data);
        return m_Parser->load_buffer_inplace(&m_Buffer[0], m_Buffer.size());
    }

    pugi::xml_node GetDocumentElement() { return m_Parser->document_element(); }

    pugi::xml_document* m_Parser;
    String              m_Buffer;
};


class ViewFactory : public XMLFactory<ViewFactoryContext&, Ptr<View>, const pugi::xml_node&>
{
public:
    ViewFactory();
    static ViewFactory& Get();

    Ptr<View> CreateView(Ptr<View> parentView, String&& XML);
    Ptr<View> CreateView(ViewFactoryContext& context, Ptr<View> parentView, const pugi::xml_node& xmlNode);
    Ptr<View> LoadView(Ptr<View> parentView, const String& path);

private:
    bool Parse(ViewFactoryContext& context, Ptr<View> parentView, const pugi::xml_node& xmlNode);
};

#define VIEW_FACTORY_REGISTER_CLASS(CLASS) struct FactoryRegistrationHelper##CLASS { FactoryRegistrationHelper##CLASS() { ViewFactory::Get().RegisterClass(#CLASS, [](ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData) { return ptr_new<CLASS>(context, parent, xmlData); }); } } g_FactoryRegistrationHelper##CLASS

} // namespace
