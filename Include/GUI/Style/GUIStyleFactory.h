// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <GUI/Style/GUIStyleInfo.h>
#include <Ptr/Ptr.h>

class PApplication;
class PGUIStyle;

class PGUIStyleFactory
{
public:
    using GetVariantsCallback = std::vector<PGUIStyleVariantInfo> (*)();
    using CreateStyleCallback = Ptr<const PGUIStyle> (*)(PApplication* application, const PString& variant);

    static PGUIStyleFactory& Get();

    // Register during factory initialization, before application threads start.
    // Style definitions are read-only after startup; variants are queried on demand.
    void RegisterStyle(const PString& name, GetVariantsCallback getVariants, CreateStyleCallback factory);
    std::vector<PGUIStyleInfo> GetStyles() const;
    std::vector<PGUIStyleVariantInfo> GetVariants(const PString& name) const;
    bool HasVariant(const PString& name, const PString& variant) const;
    Ptr<const PGUIStyle> CreateStyle(const PString& name, const PString& variant, PApplication* application) const;

private:
    struct StyleDefinition
    {
        GetVariantsCallback GetVariants;
        CreateStyleCallback Factory;
    };

    std::map<PString, StyleDefinition> m_Styles;
};
