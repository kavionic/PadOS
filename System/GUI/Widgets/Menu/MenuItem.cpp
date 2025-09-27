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

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MenuItem::MenuItem(const String& label, int id) : m_ID(id), m_Label(label)
{
    m_SuperMenu       = nullptr;
    m_SubMenu         = nullptr;
    m_IsHighlighted    = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MenuItem::MenuItem(Ptr<Menu> menu) : m_Label(menu->GetLabel())
{
    m_SuperMenu       = nullptr;
    m_SubMenu         = menu;
    m_IsHighlighted    = false;

    menu->m_SuperItem = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MenuItem::~MenuItem()
{
    if (m_SubMenu != nullptr) {
        m_SubMenu->m_SuperItem = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Menu> MenuItem::GetSubMenu() const
{
    return m_SubMenu;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Menu> MenuItem::GetSuperMenu() const
{
    return ptr_tmp_cast(m_SuperMenu);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect MenuItem::GetFrame() const
{
    return m_Frame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point MenuItem::GetContentSize()
{
    Ptr<Menu> menu = GetSuperMenu();

    Point size(4.0f, Menu::LEFT_ARROW_HEIGHT + 4.0f);

    if (m_SubMenu != nullptr) {
        size.x += Menu::LEFT_ARROW_WIDTH + 15.0f;
    }

    if (!m_Label.empty() && menu != nullptr) {
        FontHeight sHeight = menu->GetFontHeight();

        size.x += menu->GetStringWidth(GetLabel());
        size.y = std::max(size.y, sHeight.ascender + sHeight.descender + sHeight.line_gap);
//        return Point(menu->GetStringWidth(GetLabel()) + 4, sHeight.ascender + sHeight.descender + sHeight.line_gap);
    }
    return size; // Point(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String MenuItem::GetLabel() const
{
    return m_Label;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point MenuItem::GetContentLocation() const
{
    if (m_SuperMenu != nullptr) {
        return Point(m_Frame.left, m_Frame.top);
    } else {
        return Point(m_Frame.left, m_Frame.top);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MenuItem::Draw(Ptr<View> targetView)
{
    Ptr<Menu> menu = GetSuperMenu();

    if (menu == nullptr) {
        return;
    }
    if (m_Label.empty()) {
        return;
    }
    Rect frame = GetFrame();

    FontHeight fontHeight = targetView->GetFontHeight();

    if (m_IsHighlighted) {
        targetView->SetFgColor(StandardColorID::MenuBackgroundSelected);
    } else {
        targetView->SetFgColor(StandardColorID::MenuBackground);
    }

    targetView->FillRect(GetFrame());

    if (m_IsHighlighted) {
        targetView->SetFgColor(StandardColorID::MenuTextSelected);
        targetView->SetBgColor(StandardColorID::MenuBackgroundSelected);
    } else {
        targetView->SetFgColor(StandardColorID::MenuText);
        targetView->SetBgColor(StandardColorID::MenuBackground);
    }

    float vCharHeight = fontHeight.ascender + fontHeight.descender + fontHeight.line_gap;
    float y = std::round(frame.top + (frame.Height() - vCharHeight) * 0.5f + fontHeight.ascender /*+ fontHeight.line_gap * 0.5f*/);

    targetView->DrawString(m_Label, Point(frame.left + 2.0f, y));

    if (m_SubMenu != nullptr)
    {
        Ptr<Bitmap> bitmap = menu->GetArrowRightBitmap();

        Rect arrowBounds = bitmap->GetBounds();

        targetView->SetDrawingMode(DrawingMode::Overlay);
        targetView->DrawBitmap(bitmap, arrowBounds, Point(frame.right - arrowBounds.Width() - 5.0f, std::round(frame.top + (frame.Height() - arrowBounds.Height()) * 0.5f)));
        targetView->SetDrawingMode(DrawingMode::Copy);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MenuItem::DrawContent(Ptr<View> targetView)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MenuItem::Highlight(bool bHighlight)
{
    m_IsHighlighted = bHighlight;
    if (m_SuperMenu != nullptr)
    {
        m_SuperMenu->InvalidateItem(this);
    }
//    Draw();
}

} // namespace os;
