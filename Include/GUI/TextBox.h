// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 22.08.2020 15:30

#pragma once

#include <GUI/Control.h>
#include <GUI/TextEditView.h>


namespace os
{

namespace TextBoxFlags
{
static constexpr uint32_t IncludeLineGap = 0x01 << ViewFlags::FirstUserBit;
static constexpr uint32_t RaisedFrame    = 0x02 << ViewFlags::FirstUserBit;
static constexpr uint32_t ReadOnly       = 0x04 << ViewFlags::FirstUserBit;

extern const std::map<String, uint32_t> FlagMap;
}


class TextBox : public Control
{
public:
    TextBox(const String& name, const String& text, Ptr<View> parent = nullptr, uint32_t flags = 0);
    TextBox(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData);

    // From View:
    virtual void    CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const override;
    virtual void    FrameSized(const Point& delta) override;
    virtual void    Paint(const Rect& updateRect) override;

    // From TextBox:
    void            SetText(const String& text) { m_Editor->SetText(text); }
    const String&   GetText() const { return m_Editor->GetText(); }

    Point           GetSizeForString(const String& text, bool includeWidth = true, bool includeHeight = true) const;

    void            HandleKeyPress(KeyCodes keyCode, const String& text) { m_Editor->HandleKeyPress(keyCode, text); }

    Signal<void, const String&, bool, TextBox*> SignalTextChanged;
private:
    void Initialize(const String& text);

    Ptr<TextEditView> m_Editor;

    TextBox(const TextBox&) = delete;
    TextBox& operator=(const TextBox&) = delete;
};

    
    
} // namespace os
