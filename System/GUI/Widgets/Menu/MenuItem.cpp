// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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

#include <GUI/Widgets/MenuItem.h>
#include <GUI/Widgets/Menu.h>
#include <GUI/Bitmap.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMenuItem::PMenuItem(const PString& label, int id) : m_ID(id), m_Label(label)
{
    m_SuperMenu       = nullptr;
    m_SubMenu         = nullptr;
    m_IsHighlighted    = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMenuItem::PMenuItem(Ptr<PMenu> menu) : m_Label(menu->GetLabel())
{
    m_SuperMenu       = nullptr;
    m_SubMenu         = menu;
    m_IsHighlighted    = false;

    menu->m_SuperItem = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMenuItem::~PMenuItem()
{
    if (m_SubMenu != nullptr) {
        m_SubMenu->m_SuperItem = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PMenu> PMenuItem::GetSubMenu() const
{
    return m_SubMenu;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PMenu> PMenuItem::GetSuperMenu() const
{
    return ptr_tmp_cast(m_SuperMenu);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PMenuItem::GetFrame() const
{
    return m_Frame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PMenuItem::GetContentSize()
{
    Ptr<PMenu> menu = GetSuperMenu();

    PPoint size(4.0f, PMenu::LEFT_ARROW_HEIGHT + 4.0f);

    if (m_SubMenu != nullptr) {
        size.x += PMenu::LEFT_ARROW_WIDTH + 15.0f;
    }

    if (!m_Label.empty() && menu != nullptr) {
        PFontHeight sHeight = menu->GetFontHeight();

        size.x += menu->GetStringWidth(GetLabel());
        size.y = std::max(size.y, sHeight.ascender + sHeight.descender + sHeight.line_gap);
//        return Point(menu->GetStringWidth(GetLabel()) + 4, sHeight.ascender + sHeight.descender + sHeight.line_gap);
    }
    return size; // Point(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PMenuItem::GetLabel() const
{
    return m_Label;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PMenuItem::GetContentLocation() const
{
    if (m_SuperMenu != nullptr) {
        return PPoint(m_Frame.left, m_Frame.top);
    } else {
        return PPoint(m_Frame.left, m_Frame.top);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenuItem::Draw(Ptr<PView> targetView)
{
    Ptr<PMenu> menu = GetSuperMenu();

    if (menu == nullptr) {
        return;
    }
    if (m_Label.empty()) {
        return;
    }
    PRect frame = GetFrame();

    PFontHeight fontHeight = targetView->GetFontHeight();

    if (m_IsHighlighted) {
        targetView->SetFgColor(PStandardColorID::MenuBackgroundSelected);
    } else {
        targetView->SetFgColor(PStandardColorID::MenuBackground);
    }

    targetView->FillRect(GetFrame());

    if (m_IsHighlighted) {
        targetView->SetFgColor(PStandardColorID::MenuTextSelected);
        targetView->SetBgColor(PStandardColorID::MenuBackgroundSelected);
    } else {
        targetView->SetFgColor(PStandardColorID::MenuText);
        targetView->SetBgColor(PStandardColorID::MenuBackground);
    }

    float vCharHeight = fontHeight.ascender + fontHeight.descender + fontHeight.line_gap;
    float y = std::round(frame.top + (frame.Height() - vCharHeight) * 0.5f + fontHeight.ascender /*+ fontHeight.line_gap * 0.5f*/);

    targetView->DrawString(m_Label, PPoint(frame.left + 2.0f, y));

    if (m_SubMenu != nullptr)
    {
        Ptr<PBitmap> bitmap = menu->GetArrowRightBitmap();

        PRect arrowBounds = bitmap->GetBounds();

        targetView->SetDrawingMode(PDrawingMode::Overlay);
        targetView->DrawBitmap(bitmap, arrowBounds, PPoint(frame.right - arrowBounds.Width() - 5.0f, std::round(frame.top + (frame.Height() - arrowBounds.Height()) * 0.5f)));
        targetView->SetDrawingMode(PDrawingMode::Copy);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenuItem::DrawContent(Ptr<PView> targetView)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenuItem::Highlight(bool bHighlight)
{
    m_IsHighlighted = bHighlight;
    if (m_SuperMenu != nullptr)
    {
        m_SuperMenu->InvalidateItem(this);
    }
//    Draw();
}
