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

#include <GUI/MenuSeparator.h>
#include <GUI/Menu.h>

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MenuSeparator::MenuSeparator() : MenuItem(String::zero)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MenuSeparator::~MenuSeparator()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point MenuSeparator::GetContentSize()
{
    Ptr<Menu> menu = GetSuperMenu();

    if (menu != nullptr)
    {
        if (menu->GetLayout() == MenuLayout::Horizontal) {
            return Point(6.0f, 0.0f);
        } else {
            return Point(0.0f, 6.0f);
        }
    }
    return Point(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MenuSeparator::Draw(Ptr<View> targetView)
{
    Ptr<Menu> menu = GetSuperMenu();

    if (menu == nullptr) {
        return;
    }
    Rect frame = GetFrame();
    Rect viewFrame = targetView->GetBounds();

    if (menu->GetLayout() == MenuLayout::Horizontal)
    {
        frame.top = viewFrame.top;
        frame.bottom = viewFrame.bottom;
    }
    else
    {
        frame.left = viewFrame.left;
        frame.right = viewFrame.right;
    }
    targetView->SetFgColor(StandardColorID::MenuBackground);
    targetView->FillRect(frame);

    if (menu->GetLayout() == MenuLayout::Horizontal)
    {
        float x = floor(frame.left + (frame.Width() + 1.0f) * 0.5f);
        targetView->SetFgColor(StandardColorID::Shadow);
        targetView->DrawLine(Point(x, frame.top + 2.0f), Point(x, frame.bottom - 2.0f));
        x += 1.0f;
        targetView->SetFgColor(StandardColorID::Shine);
        targetView->DrawLine(Point(x, frame.top + 2.0f), Point(x, frame.bottom - 2.0f));
    }
    else
    {
        float y = floor(frame.top + (frame.Height() + 1.0f) * 0.5f);
        targetView->SetFgColor(StandardColorID::Shadow);
        targetView->DrawLine(Point(frame.left + 4.0f, y), Point(frame.right - 4.0f, y));
        y += 1.0f;
        targetView->SetFgColor(StandardColorID::Shine);
        targetView->DrawLine(Point(frame.left + 4.0f, y), Point(frame.right - 4.0f, y));
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MenuSeparator::DrawContent(Ptr<View> targetView)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MenuSeparator::Highlight(bool highlight)
{
}

} // namespace os
