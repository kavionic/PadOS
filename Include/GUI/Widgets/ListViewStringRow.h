// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Widgets/ListViewRow.h>
#include <Utils/String.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PListViewStringRow : public PListViewRow
{
public:
    PListViewStringRow() {}
    virtual ~PListViewStringRow() {}

    virtual void    AttachToView(Ptr<PView> view, int column) override;
    virtual void    SetRect(const PRect& rect, size_t column) override;
    void            AppendString(const PString& string);
    void            SetString(size_t index, const PString& string);
    const PString&  GetString(size_t index) const;
    virtual float   GetWidth(Ptr<PView> view, size_t column) override;
    virtual float   GetHeight(Ptr<PView> view) override;
    virtual void    Paint(const PRect& frame, Ptr<PView> view, size_t column, bool selected, bool highlighted, bool hasFocus) override;
    virtual bool    IsLessThan(Ptr<const PListViewRow> other, size_t column) const override;

private:
    std::vector<std::pair<PString, float>> m_Strings;
};
