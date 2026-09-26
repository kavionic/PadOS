// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 23.05.2025 21:30

#pragma once

#include <GUI/View.h>


class PScrollableView : public PView
{
public:
    PScrollableView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PScrollableView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual PPoint CalculateContentSize() const override;
    virtual void OnLayoutChanged() override;

    void SetContentView(Ptr<PView> contentView);

private:
    Ptr<PView> m_ContentView;
};
