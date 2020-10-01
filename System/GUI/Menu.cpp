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

#include <stdio.h>
#include <assert.h>

#include <GUI/Desktop.h>
#include <GUI/Menu.h>
#include <GUI/MenuItem.h>
#include <GUI/Font.h>
#include <GUI/Bitmap.h>
#include "MenuRenderView.h"


namespace os
{

static const uint32_t g_ArrowBitmapRaster[] =
{

    0b10000000000000000000000000000000,
    0b11000000000000000000000000000000,
    0b11100000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11111000000000000000000000000000,
    0b11111100000000000000000000000000,
    0b11111110000000000000000000000000,
    0b11111111000000000000000000000000,
    0b11111111100000000000000000000000,
    0b11111111110000000000000000000000,
    0b11111111100000000000000000000000,
    0b11111111000000000000000000000000,
    0b11111110000000000000000000000000,
    0b11111100000000000000000000000000,
    0b11111000000000000000000000000000,
    0b11110000000000000000000000000000,
    0b11100000000000000000000000000000,
    0b11000000000000000000000000000000,
    0b10000000000000000000000000000000,
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Menu::Menu(const String& title, MenuLayout layout, uint32_t flags)
    : View("_menu_", nullptr, flags | ViewFlags::WillDraw | ViewFlags::ClearBackground)
    , m_Title(title)
{
    m_ItemBorders = Rect(3, 2, 3, 2);
    m_Layout = layout;

    m_ContentView = ptr_new<MenuRenderView>(this);

    SetScrolledView(m_ContentView);

    m_OpenTimer.SignalTrigged.Connect(this, &Menu::OpenSelection);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Menu::~Menu()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String Menu::GetLabel() const
{
    return m_Title;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MenuLayout Menu::GetLayout() const
{
    return m_Layout;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::FrameSized(const Point& delta)
{
    Rect contentFrame = GetBounds();
    contentFrame.Resize(2.0f, 2.0f, -2.0f, -2.0f);

    m_ContentView->SetFrame(contentFrame);

    if (delta.x > 0.0f)
    {
        Rect bounds = GetBounds();

        bounds.left = bounds.right - delta.x;
        Invalidate(bounds);
        Flush();
    }
    else if (delta.x < 0.0f)
    {
        Rect bounds = GetBounds();

        bounds.left = bounds.right - 2.0f;
        Invalidate(bounds);
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::StartOpenTimer(TimeValMicros delay)
{
    if (!m_OpenTimer.IsRunning())
    {
        m_OpenTimer.Set(delay, true);
        m_OpenTimer.Start(true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::OpenSelection()
{
    Ptr<MenuItem> selectedItem = FindMarked();

    if (selectedItem == nullptr) {
        return;
    }

    for (Ptr<MenuItem> item : m_Items)
    {
        if (item == selectedItem) {
            continue;
        }
        if (item->m_SubMenu != nullptr && item->m_SubMenu->m_IsOpen) {
            item->m_SubMenu->Close(true, false, nullptr);
            break;
        }
    }

    if (selectedItem->m_SubMenu == nullptr || selectedItem->m_SubMenu->m_IsOpen) {
        return;
    }

    Rect frame = selectedItem->GetFrame();
    Point screenPos;

    Point screenResolution(Desktop().GetResolution());

    Point topLeft(0.0f, 0.0f);

    m_ContentView->ConvertToScreen(&topLeft);

    if (m_Layout == MenuLayout::Horizontal)
    {
        screenPos.x = topLeft.x + frame.left - m_ItemBorders.left;

        float nHeight = selectedItem->m_SubMenu->m_ContentSize.y;
        if (topLeft.y + GetBounds().Height() + 1.0f + nHeight <= screenResolution.y) {
            screenPos.y = topLeft.y + GetBounds().Height() + 1.0f;
        } else {
            screenPos.y = topLeft.y - nHeight;
        }
    }
    else
    {
        float nHeight = selectedItem->m_SubMenu->m_ContentSize.y;

        screenPos.x = topLeft.x + GetBounds().Width() + 1.0f - 4.0f;
        if (topLeft.y + frame.top - m_ItemBorders.top + nHeight <= screenResolution.y) {
            screenPos.y = topLeft.y + frame.top - m_ItemBorders.top;
        } else {
            screenPos.y = topLeft.y + frame.bottom - nHeight + m_ItemBorders.top + selectedItem->m_SubMenu->m_ItemBorders.bottom;
        }
    }
    m_HasOpenChildren = true;
    selectedItem->m_SubMenu->Open(screenPos, MenuLocation::TopLeft);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::SelectItem(Ptr<MenuItem> item)
{
    Ptr<MenuItem> prev = FindMarked();

    if (item == prev) {
        return;
    }
    if (prev != nullptr) {
        prev->Highlight(false);
    }
    if (item != nullptr)
    {
        item->Highlight(true);

        if (m_Layout == MenuLayout::Horizontal) {
            OpenSelection();
        } else {
            StartOpenTimer(0.2);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::SelectPrev()
{
    if (m_Items.empty()) {
        return;
    }
    Ptr<MenuItem> previous;

    for (Ptr<MenuItem> item : m_Items)
    {
        if (item->m_IsHighlighted)
        {
            if (previous != nullptr)
            {
                SelectItem(previous);
                Flush();
            }
            return;
        }
        previous = item;
    }
    if (m_Layout == MenuLayout::Vertical)
    {
        Ptr<Menu> super = GetSuperMenu();
        if (super != nullptr && super->m_Layout == MenuLayout::Horizontal) {
            Close(false, false, nullptr);
        }
    }
    else
    {
        SelectItem(m_Items[0]);
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::SelectNext()
{
    for (size_t i = 0; i < m_Items.size(); ++i)
    {
        Ptr<MenuItem> item = m_Items[i];
        if (item->m_IsHighlighted || i == m_Items.size() - 1)
        {
            if (i + 1 < m_Items.size()) {
                SelectItem(m_Items[i + 1]);
            } else {
                SelectItem(item);
            }
            Flush();
            return;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::Open(const Point& screenPosition, MenuLocation relativeLocation)
{
    if (m_IsOpen) {
        return;
    }
    const Point resolution = Desktop().GetResolution();
    const Point size = GetPreferredSize(PrefSizeType::Smallest);
    const Rect  bounds(0, 0, size.x, size.y);
    Rect  frame = bounds;
    m_IsOpen = true;

    switch (relativeLocation)
    {
        case os::MenuLocation::Auto:
            frame += screenPosition;
            if (size.y < screenPosition.y - 20.0f || screenPosition.y > resolution.y * 0.5f)
            {
                frame += Point(-round(size.x * 0.5f), -size.y - 20.0f);
            }
            else
            {
                frame += Point(-round(size.x * 0.5f), 20.0f);
            }
            break;
        case os::MenuLocation::TopLeft:
            frame += screenPosition;
            break;
        case os::MenuLocation::BottomLeft:
            frame += screenPosition - Point(0.0f, -size.y);
            break;
        default:
            break;
    }

    static constexpr float SCREEN_BORDERS = 20.0f;

    if (frame.bottom > resolution.y - SCREEN_BORDERS)
    {
        frame.top += resolution.y - frame.bottom - SCREEN_BORDERS;
        frame.bottom = resolution.y - SCREEN_BORDERS;
    }
    if (frame.top < SCREEN_BORDERS)
    {
        frame.bottom += SCREEN_BORDERS - frame.top;
        frame.top = SCREEN_BORDERS;
        if (frame.bottom > resolution.y - SCREEN_BORDERS) {
            frame.bottom = resolution.y - SCREEN_BORDERS;
        }
    }
    if (frame.right > resolution.x - SCREEN_BORDERS)
    {
        frame.left += resolution.x - frame.right - SCREEN_BORDERS;
        frame.right = resolution.x - SCREEN_BORDERS;
    }
    if (frame.left < SCREEN_BORDERS)
    {
        frame.right += SCREEN_BORDERS - frame.left;
        frame.left = SCREEN_BORDERS;
        if (frame.right > resolution.x - SCREEN_BORDERS) {
            frame.right = resolution.x - SCREEN_BORDERS;
        }
    }

    SetFrame(frame);
    SetKeyboardFocus(true);
    SelectItem(m_Items.empty() ? nullptr : m_Items[0]);

    Application* app = Application::GetCurrentApplication();

    if (app != nullptr) {
        app->AddView(ptr_tmp_cast(this), ViewDockType::TopLevelView);
    }

    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::Close(bool bCloseChilds, bool bCloseParent, Ptr<MenuItem> selectedItem)
{
    Ptr<Menu> self = ptr_tmp_cast(this);
    for (Ptr<MenuItem> item : m_Items)
    {
        if (item->m_IsHighlighted) {
            item->Highlight(false);
        }
    }
    if (bCloseChilds)
    {
        for (Ptr<MenuItem> item : m_Items)
        {
            if (item->m_SubMenu != nullptr) {
                item->m_SubMenu->Close(bCloseChilds, bCloseParent, selectedItem);
            }
        }
    }

    if (m_IsOpen)
    {
        m_IsOpen = false;
        Application* app = GetApplication();

        if (app != nullptr) {
            app->RemoveView(self);
        }

        Ptr<Menu> parent = GetSuperMenu();
        if (parent != nullptr)
        {
            if (parent->m_IsOpen) {
                parent->SetKeyboardFocus(true);
            }
            parent->m_HasOpenChildren = false;
        }
        SignalItemSelected(selectedItem, self);
    }

    if (bCloseParent)
    {
        Ptr<Menu> parent = GetSuperMenu();
        if (parent != nullptr) {
            parent->Close(false, true, selectedItem);
        } else if (selectedItem != nullptr) {
            selectedItem->SignalClicked(selectedItem);
        }
    }
    else if (selectedItem != nullptr)
    {
        selectedItem->SignalClicked(selectedItem);
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//void Menu::WindowActivated(bool bIsActive)
//{
//    if (!bIsActive && !m_HasOpenChildren)
//    {
//        Close(false, true);
//
//        if (m_hTrackPort != -1) {
//            MenuItem* pcItem = nullptr;
//            if (send_msg(m_hTrackPort, 1, &pcItem, sizeof(pcItem)) < 0) {
//                printf("Error: Menu::WindowActivated() failed to send message to m_hTrackPort\n");
//            }
//        }
//    }
//}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::OnKeyDown(KeyCodes keyCode, const String& text, const KeyEvent& event)
{
    switch (keyCode)
    {
        case KeyCodes::CURSOR_UP:
            if (m_Layout == MenuLayout::Horizontal) {
            } else {
                SelectPrev();
                StartOpenTimer(1.0);
            }
            break;
        case KeyCodes::CURSOR_DOWN:
            if (m_Layout == MenuLayout::Horizontal) {
                OpenSelection();
            } else {
                SelectNext();
                StartOpenTimer(1.0);
            }
            break;
        case KeyCodes::CURSOR_LEFT:
            if (m_Layout == MenuLayout::Horizontal)
            {
                SelectPrev();
                OpenSelection();
            }
            else
            {
                Ptr<Menu> super = GetSuperMenu();
                if (super != nullptr)
                {
                    if (super->m_Layout == MenuLayout::Vertical) {
                        Close(false, false, nullptr);
                    } else {
                        super->SelectPrev();
                        super->OpenSelection();
                    }
                }
            }
            break;
        case KeyCodes::CURSOR_RIGHT:
            if (m_Layout == MenuLayout::Horizontal)
            {
                SelectNext();
                OpenSelection();
            }
            else
            {
                Ptr<Menu> super = GetSuperMenu();
                if (super != nullptr)
                {
                    Ptr<MenuItem> item = FindMarked();
                    if (item != nullptr && item->m_SubMenu != nullptr) {
                        OpenSelection();
                    } else if (super->m_Layout == MenuLayout::Horizontal) {
                        super->SelectNext();
                        super->OpenSelection();
                    }
                }
            }
            break;
        case KeyCodes::ENTER:
        {
            Ptr<MenuItem> item = FindMarked();
            if (item != nullptr)
            {
                if (item->m_SubMenu != nullptr)
                {
                    OpenSelection();
                }
                else
                {
                    Close(false, true, item);
                }
            }
            break;
        }
        case KeyCodes::ESCAPE:
            Close(false, false, nullptr);
            Flush();
            break;
        default:
            View::OnKeyDown(keyCode, text, event);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::Paint(const Rect& updateRect)
{
    SetFgColor(StandardColorID::MenuBackground);
    FillRect(GetBounds());

    DrawFrame(GetBounds(), FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::AddStringItem(const String& label, int id)
{
    return AddItem(ptr_new<MenuItem>(label, id));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::AddItem(Ptr<MenuItem> item)
{
    return AddItem(item, m_Items.size());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::AddItem(Ptr<MenuItem> item, size_t index)
{
    if (index > m_Items.size()) {
        return nullptr;
    }

    item->m_SuperMenu = this;
    m_Items.insert(m_Items.begin() + index, item);
    InvalidateLayout();

    return item;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::AddSubMenu(Ptr<Menu> menu)
{
    return AddSubMenu(menu, m_Items.size());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::AddSubMenu(Ptr<Menu> menu, size_t index)
{
    return AddItem(ptr_new<MenuItem>(menu), index);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::RemoveItem(size_t index)
{
    if (index < m_Items.size())
    {
        Ptr<MenuItem> item = m_Items[index];
        m_Items.erase(m_Items.begin() + index);
        InvalidateLayout();
        return item;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Menu::RemoveItem(Ptr<MenuItem> item)
{
    auto i = std::find(m_Items.begin(), m_Items.end(), item);

    if (i != m_Items.end())
    {
        m_Items.erase(i);
        InvalidateLayout();
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Menu::RemoveItem(Ptr<Menu> menu)
{
    auto i = std::find_if(m_Items.begin(), m_Items.end(), [&menu](const Ptr<MenuItem>& item) { return item->m_SubMenu == menu; });

    if (i != m_Items.end())
    {
        m_Items.erase(i);
        InvalidateLayout();
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::GetItemAt(Point position) const
{
    for (Ptr<MenuItem> item : m_Items)
    {
        if (item->m_Frame.DoIntersect(position)) {
            return item;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::GetItemAt(size_t index) const
{
    if (index < m_Items.size()) {
        return m_Items[index];
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Menu> Menu::GetSubMenuAt(size_t index) const
{
    Ptr<MenuItem> item = GetItemAt(index);

    if (item != nullptr) {
        return item->m_SubMenu;
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t Menu::GetItemCount() const
{
    return m_Items.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t Menu::GetIndexOf(Ptr<MenuItem> item) const
{
    for (size_t i = 0; i < m_Items.size(); ++i)
    {
        if (m_Items[i] == item) {
            return i;
        }
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t Menu::GetIndexOf(Ptr<Menu> menu) const
{
    for (size_t i = 0; i < m_Items.size(); ++i)
    {
        if (m_Items[i]->m_SubMenu == menu) {
            return i;
        }
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::FindItem(int id) const
{
    for (Ptr<MenuItem> item : m_Items)
    {
        if (item->m_ID == id) {
            return item;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::FindItem(const String& name) const
{
    for (Ptr<MenuItem> item : m_Items)
    {
        if (item->m_SubMenu != nullptr)
        {
            Ptr<MenuItem> subItem = item->m_SubMenu->FindItem(name);
            if (subItem != nullptr) {
                return subItem;
            }
        }
        else
        {
            if (item->m_Label == name) {
                return item;
            }
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::FindMarked() const
{
    for (Ptr<MenuItem> item : m_Items)
    {
        if (item->m_IsHighlighted) {
            return item;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Menu> Menu::GetSuperMenu() const
{
    if (m_SuperItem != nullptr) {
        return ptr_tmp_cast(m_SuperItem->m_SuperMenu);
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MenuItem> Menu::GetSuperItem()
{
    return ptr_tmp_cast(m_SuperItem);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    *minSize = m_ContentSize + Point(4.0f, 4.0f);
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::InvalidateLayout()
{
    m_ContentSize = Point(0.0f, 0.0f);

    switch (m_Layout)
    {
        case MenuLayout::Horizontal:
            for (Ptr<MenuItem> item : m_Items)
            {
                Point   itemContentSize = item->GetContentSize();

                item->m_Frame = Rect(m_ContentSize.x + m_ItemBorders.left, m_ItemBorders.top, m_ContentSize.x + itemContentSize.x + m_ItemBorders.left, itemContentSize.y + m_ItemBorders.top);
                m_ContentSize.x += itemContentSize.x + m_ItemBorders.left + m_ItemBorders.right;

                if (itemContentSize.y + m_ItemBorders.top + m_ItemBorders.bottom > m_ContentSize.y) {
                    m_ContentSize.y = itemContentSize.y + m_ItemBorders.top + m_ItemBorders.bottom;
                }
            }
            break;
        case MenuLayout::Vertical:
            for (Ptr<MenuItem> item : m_Items)
            {
                Point   itemContentSize = item->GetContentSize();

                item->m_Frame = Rect(m_ItemBorders.left, m_ContentSize.y + m_ItemBorders.top, itemContentSize.x + m_ItemBorders.left, m_ContentSize.y + itemContentSize.y + m_ItemBorders.top);

                m_ContentSize.y += itemContentSize.y + m_ItemBorders.top + m_ItemBorders.bottom;

                if (itemContentSize.x + m_ItemBorders.left + m_ItemBorders.right > m_ContentSize.x) {
                    m_ContentSize.x = itemContentSize.x + m_ItemBorders.left + m_ItemBorders.right;
                }
            }
            for (Ptr<MenuItem> item : m_Items) {
                item->m_Frame.right = m_ContentSize.x - m_ItemBorders.left - m_ItemBorders.right;
            }
            break;
    }
    PreferredSizeChanged();
    m_ContentView->ContentSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Menu::InvalidateItem(MenuItem* item)
{
    m_ContentView->Invalidate(item->GetFrame());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Bitmap> Menu::GetArrowRightBitmap()
{
    if (m_ArrowRightBitmap == nullptr) {
        m_ArrowRightBitmap = ptr_new<Bitmap>(LEFT_ARROW_WIDTH, LEFT_ARROW_HEIGHT, ColorSpace::MONO1, g_ArrowBitmapRaster, sizeof(uint32_t));
    }
    return m_ArrowRightBitmap;
}

} // namespace os;
