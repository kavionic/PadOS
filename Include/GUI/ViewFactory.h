// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 15.06.2020 22:55:22

#pragma once

#include <pugixml.hpp>

#include <Utils/XMLFactory.h>
#include <Utils/FactoryAutoRegistrator.h>
#include <Utils/String.h>
#include <GUI/ViewFactoryContext.h>
#include <Storage/File.h>


class PView;


class PXMLDocument
{
public:
    PXMLDocument() { m_Parser = new pugi::xml_document(); }
    ~PXMLDocument() { delete m_Parser; }

    bool Load(const PString& path)
    {
        m_Buffer.clear();
        {
            PFile file(path);
            if (!file.IsValid()) {
                return false;
            }
            if (!file.Read(m_Buffer)) {
                return false;
            }
        }
        return m_Parser->load_buffer_inplace(&m_Buffer[0], m_Buffer.size());
    }
    bool Parse(PString&& data)
    {
        m_Buffer = std::move(data);
        return m_Parser->load_buffer_inplace(&m_Buffer[0], m_Buffer.size());
    }

    pugi::xml_node GetDocumentElement() { return m_Parser->document_element(); }

    pugi::xml_document* m_Parser;
    PString             m_Buffer;
};


class PViewFactory : public PXMLFactory<PViewFactoryContext&, Ptr<PView>, const pugi::xml_node&>
{
public:
    PViewFactory();
    static PViewFactory& Get();

    Ptr<PView> CreateView(Ptr<PView> parentView, PString&& XML);
    Ptr<PView> CreateView(PViewFactoryContext& context, Ptr<PView> parentView, const pugi::xml_node& xmlNode);
    Ptr<PView> LoadView(Ptr<PView> parentView, const PString& path);

private:
    bool Parse(PViewFactoryContext& context, Ptr<PView> parentView, const pugi::xml_node& xmlNode);
};

#define PVIEW_FACTORY_REGISTER_CLASS(CLASS) PFactoryAutoRegistrator<CLASS> \
    g_ViewFactoryRegistrationHelper___##CLASS([] { \
            PViewFactory::Get().RegisterClass(#CLASS, [](PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) { \
                    return ptr_new<CLASS>(context, parent, xmlData); \
                }); \
        });

#define PVIEW_FACTORY_REGISTER_CLASS_RPREFIX(CLASS) PFactoryAutoRegistrator<CLASS> \
    g_ViewFactoryRegistrationHelper___##CLASS([] { \
            PViewFactory::Get().RegisterClass(#CLASS + 1, [](PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) { \
                    return ptr_new<CLASS>(context, parent, xmlData); \
                }); \
        });
