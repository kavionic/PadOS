// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2020 16:30:00

#pragma once

#include <GUI/Style/GroupViewStyleData.h>
#include <GUI/StyledView.h>


class PGroupView : public PStyledView
{
public:
    PGroupView(const PString& name = PString::zero, PViewFlags flags = PViewFlags::zero);
    PGroupView(PViewFactoryContext& context, const pugi::xml_node& xmlData);

    // From View:
    virtual void OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void OnStyleChanged() override;
    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void OnPaint(const PRect& updateRect) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    void SetLabel(const PString& label);
    const PString& GetLabel() const;

private:
    void NotifyStylePropertyChanged(PGroupViewProperties properties);

    PGroupViewStyleData m_GroupViewStyleData;
};
