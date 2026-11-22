// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include <GUI/Style/GUIStyle.h>
#include <Utils/String.h>

class PDefaultGUIStyle : public PGUIStyle
{
public:
    explicit PDefaultGUIStyle(PApplication* application = nullptr);

    static PString GetName();
    static std::vector<PString> GetVariants();
    static Ptr<const PGUIStyle> CreateStyle(PApplication* application, const PString& variant);
};
