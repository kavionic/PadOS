// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/GUIStyleFactory.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGUIStyleFactory& PGUIStyleFactory::Get()
{
    static PGUIStyleFactory factory;
    return factory;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGUIStyleFactory::RegisterStyle(const PString& name, GetVariantsCallback getVariants, CreateStyleCallback factory)
{
    if (name.empty() || getVariants == nullptr || factory == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (!m_Styles.emplace(name, StyleDefinition{getVariants, factory}).second) {
        PERROR_THROW_CODE(PErrorCode::EXIST);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<PGUIStyleInfo> PGUIStyleFactory::GetStyles() const
{
    std::vector<PGUIStyleInfo> styles;
    styles.reserve(m_Styles.size());
    for (const auto& [name, definition] : m_Styles) {
        styles.push_back(PGUIStyleInfo{name, definition.GetVariants()});
    }
    return styles;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<PGUIStyleVariantInfo> PGUIStyleFactory::GetVariants(const PString& name) const
{
    const auto iterator = m_Styles.find(name);
    if (iterator == m_Styles.end()) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return iterator->second.GetVariants();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PGUIStyleFactory::HasVariant(const PString& name, const PString& variant) const
{
    const auto iterator = m_Styles.find(name);
    bool hasVariant = false;
    if (iterator != m_Styles.end())
    {
        const std::vector<PGUIStyleVariantInfo> variants = iterator->second.GetVariants();
        hasVariant = std::ranges::find(variants, variant, &PGUIStyleVariantInfo::Name) != variants.end();
    }
    return hasVariant;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PGUIStyle> PGUIStyleFactory::CreateStyle(const PString& name, const PString& variant, PApplication* application) const
{
    const auto iterator = m_Styles.find(name);
    if (iterator == m_Styles.end()) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    const StyleDefinition& definition = iterator->second;
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    Ptr<const PGUIStyle> style = definition.Factory(application, variant);
    if (style == nullptr) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    return style;
}
