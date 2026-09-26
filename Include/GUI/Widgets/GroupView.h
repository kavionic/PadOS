// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2020 16:30:00

#pragma once

#include <GUI/View.h>


class PGroupView : public PView
{
public:
    PGroupView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PGroupView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    // From View:
    virtual void OnPaint(const PRect& updateRect) override;

private:
    PString m_Label;
};
