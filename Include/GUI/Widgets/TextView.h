// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 15.04.2018 16:38:59

#pragma once

#include <GUI/View.h>


namespace PTextViewFlags
{
static constexpr uint32_t IncludeLineGap    = 0x01 << PViewFlags::FirstUserBit;
static constexpr uint32_t MultiLine         = 0x02 << PViewFlags::FirstUserBit;
extern const std::map<PString, uint32_t> FlagMap;
}

class PTextView : public PView
{
public:
    PTextView(const PString& name = PString::zero, const PString& text = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
	PTextView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);
    ~PTextView();
    
    void SetText(const PString& text);
    const PString& GetText() const { return m_Text; }

    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    virtual void OnPaint(const PRect& updateRect) override;
        
private:
    bool UpdateWordWrapping();

    PString m_Text;
    float   m_AspectRatio = 3.0f;
    float   m_TextWidth = 0.0f;
    bool    m_IsWordWrappingValid = true;
    std::vector<size_t> m_LineWraps;
    PTextView(const PTextView&) = delete;
    PTextView& operator=(const PTextView&) = delete;
};
