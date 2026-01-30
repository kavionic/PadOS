// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
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
#include <GUI/Widgets/Menu.h>
#include <GUI/Widgets/MenuItem.h>
#include <GUI/Font.h>
#include <GUI/Bitmap.h>
#include "MenuRenderView.h"


const std::map<PString, uint32_t> PMenuFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(PMenuFlags, NoKeyboardFocus)
};

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

PMenu::PMenu(const PString& title, PMenuLayout layout, uint32_t flags)
    : PView("_menu_", nullptr, flags | PViewFlags::WillDraw | PViewFlags::ClearBackground)
    , m_Title(title)
{
    m_ItemBorders = PRect(3, 2, 3, 2);
    m_Layout = layout;

    m_ContentView = ptr_new<PMenuRenderView>(this);

    SetScrolledView(m_ContentView);

    m_OpenTimer.SignalTrigged.Connect(this, &PMenu::OpenSelection);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMenu::~PMenu()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PMenu::GetLabel() const
{
    return m_Title;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMenuLayout PMenu::GetLayout() const
{
    return m_Layout;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenu::OnFrameSized(const PPoint& delta)
{
    PRect contentFrame = GetBounds();
    contentFrame.Resize(2.0f, 2.0f, -2.0f, -2.0f);

    m_ContentView->SetFrame(contentFrame);

    if (delta.x > 0.0f)
    {
        PRect bounds = GetBounds();

        bounds.left = bounds.right - delta.x;
        Invalidate(bounds);
        Flush();
    }
    else if (delta.x < 0.0f)
    {
        PRect bounds = GetBounds();

        bounds.left = bounds.right - 2.0f;
        Invalidate(bounds);
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenu::StartOpenTimer(TimeValNanos delay)
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

void PMenu::OpenSelection()
{
    Ptr<PMenuItem> selectedItem = FindMarked();

    if (selectedItem == nullptr) {
        return;
    }

    for (Ptr<PMenuItem> item : m_Items)
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

    PRect frame = selectedItem->GetFrame();
    PPoint screenPos;

    PPoint screenResolution(PDesktop().GetResolution());

    PPoint topLeft(0.0f, 0.0f);

    m_ContentView->ConvertToScreen(&topLeft);

    if (m_Layout == PMenuLayout::Horizontal)
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
    selectedItem->m_SubMenu->Open(screenPos, PMenuLocation::TopLeft);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenu::SelectItem(Ptr<PMenuItem> item)
{
    Ptr<PMenuItem> prev = FindMarked();

    if (item == prev) {
        return;
    }
    if (prev != nullptr) {
        prev->Highlight(false);
    }
    if (item != nullptr)
    {
        item->Highlight(true);

        if (m_Layout == PMenuLayout::Horizontal) {
            OpenSelection();
        } else {
            StartOpenTimer(0.2);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenu::SelectPrev()
{
    if (m_Items.empty()) {
        return;
    }
    Ptr<PMenuItem> previous;

    for (Ptr<PMenuItem> item : m_Items)
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
    if (m_Layout == PMenuLayout::Vertical)
    {
        Ptr<PMenu> super = GetSuperMenu();
        if (super != nullptr && super->m_Layout == PMenuLayout::Horizontal) {
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

void PMenu::SelectNext()
{
    for (size_t i = 0; i < m_Items.size(); ++i)
    {
        Ptr<PMenuItem> item = m_Items[i];
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

void PMenu::Open(const PPoint& screenPosition, PMenuLocation relativeLocation)
{
    if (m_IsOpen) {
        return;
    }
    const PPoint resolution = PDesktop().GetResolution();
    const PPoint size = GetPreferredSize(PPrefSizeType::Smallest);
    const PRect  bounds(0, 0, size.x, size.y);
    PRect  frame = bounds;
    m_IsOpen = true;

    switch (relativeLocation)
    {
        case PMenuLocation::Auto:
            frame += screenPosition;
            if (size.y < screenPosition.y - 20.0f || screenPosition.y > resolution.y * 0.5f)
            {
                frame += PPoint(-std::round(size.x * 0.5f), -size.y - 20.0f);
            }
            else
            {
                frame += PPoint(-std::round(size.x * 0.5f), 20.0f);
            }
            break;
        case PMenuLocation::Center:
            frame += (screenPosition - frame.Size() * 0.5f).GetRounded();
            break;
        case PMenuLocation::TopLeft:
            frame += screenPosition;
            break;
        case PMenuLocation::BottomLeft:
            frame += screenPosition - PPoint(0.0f, -size.y);
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
    if (!HasFlags(PMenuFlags::NoKeyboardFocus)) {
        SetKeyboardFocus(true);
    }

    PApplication* app = PApplication::GetCurrentApplication();

    if (app != nullptr) {
        app->AddView(ptr_tmp_cast(this), PViewDockType::RootLevelView);
    }

    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenu::Close(bool bCloseChilds, bool bCloseParent, Ptr<PMenuItem> selectedItem)
{
    Ptr<PMenu> self = ptr_tmp_cast(this);
    for (Ptr<PMenuItem> item : m_Items)
    {
        if (item->m_IsHighlighted) {
            item->Highlight(false);
        }
    }
    if (bCloseChilds)
    {
        for (Ptr<PMenuItem> item : m_Items)
        {
            if (item->m_SubMenu != nullptr) {
                item->m_SubMenu->Close(bCloseChilds, bCloseParent, selectedItem);
            }
        }
    }

    if (m_IsOpen)
    {
        m_IsOpen = false;
        PApplication* app = GetApplication();

        if (app != nullptr) {
            app->RemoveView(self);
        }

        Ptr<PMenu> parent = GetSuperMenu();
        if (parent != nullptr)
        {
            if (parent->m_IsOpen && !HasFlags(PMenuFlags::NoKeyboardFocus)) {
                parent->SetKeyboardFocus(true);
            }
            parent->m_HasOpenChildren = false;
        }
        SignalItemSelected(selectedItem, self);
    }

    if (bCloseParent)
    {
        Ptr<PMenu> parent = GetSuperMenu();
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

void PMenu::OnKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& event)
{
    switch (keyCode)
    {
        case PKeyCodes::CURSOR_UP:
            if (m_Layout == PMenuLayout::Horizontal) {
            } else {
                SelectPrev();
                StartOpenTimer(1.0);
            }
            break;
        case PKeyCodes::CURSOR_DOWN:
            if (m_Layout == PMenuLayout::Horizontal) {
                OpenSelection();
            } else {
                SelectNext();
                StartOpenTimer(1.0);
            }
            break;
        case PKeyCodes::CURSOR_LEFT:
            if (m_Layout == PMenuLayout::Horizontal)
            {
                SelectPrev();
                OpenSelection();
            }
            else
            {
                Ptr<PMenu> super = GetSuperMenu();
                if (super != nullptr)
                {
                    if (super->m_Layout == PMenuLayout::Vertical) {
                        Close(false, false, nullptr);
                    } else {
                        super->SelectPrev();
                        super->OpenSelection();
                    }
                }
            }
            break;
        case PKeyCodes::CURSOR_RIGHT:
            if (m_Layout == PMenuLayout::Horizontal)
            {
                SelectNext();
                OpenSelection();
            }
            else
            {
                Ptr<PMenu> super = GetSuperMenu();
                if (super != nullptr)
                {
                    Ptr<PMenuItem> item = FindMarked();
                    if (item != nullptr && item->m_SubMenu != nullptr) {
                        OpenSelection();
                    } else if (super->m_Layout == PMenuLayout::Horizontal) {
                        super->SelectNext();
                        super->OpenSelection();
                    }
                }
            }
            break;
        case PKeyCodes::ENTER:
        {
            Ptr<PMenuItem> item = FindMarked();
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
        case PKeyCodes::ESCAPE:
            Close(false, false, nullptr);
            Flush();
            break;
        default:
            PView::OnKeyDown(keyCode, text, event);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenu::OnPaint(const PRect& updateRect)
{
    SetFgColor(PStandardColorID::MenuBackground);
    FillRect(GetBounds());

    DrawFrame(GetBounds(), FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PMenuItem> PMenu::AddStringItem(const PString& label, int id)
{
    return AddItem(ptr_new<PMenuItem>(label, id));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PMenuItem> PMenu::AddItem(Ptr<PMenuItem> item)
{
    return AddItem(item, m_Items.size());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PMenuItem> PMenu::AddItem(Ptr<PMenuItem> item, size_t index)
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

Ptr<PMenuItem> PMenu::AddSubMenu(Ptr<PMenu> menu)
{
    return AddSubMenu(menu, m_Items.size());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PMenuItem> PMenu::AddSubMenu(Ptr<PMenu> menu, size_t index)
{
    return AddItem(ptr_new<PMenuItem>(menu), index);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PMenuItem> PMenu::RemoveItem(size_t index)
{
    if (index < m_Items.size())
    {
        Ptr<PMenuItem> item = m_Items[index];
        m_Items.erase(m_Items.begin() + index);
        InvalidateLayout();
        return item;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PMenu::RemoveItem(Ptr<PMenuItem> item)
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

bool PMenu::RemoveItem(Ptr<PMenu> menu)
{
    auto i = std::find_if(m_Items.begin(), m_Items.end(), [&menu](const Ptr<PMenuItem>& item) { return item->m_SubMenu == menu; });

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

Ptr<PMenuItem> PMenu::GetItemAt(PPoint position) const
{
    for (Ptr<PMenuItem> item : m_Items)
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

Ptr<PMenuItem> PMenu::GetItemAt(size_t index) const
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

Ptr<PMenu> PMenu::GetSubMenuAt(size_t index) const
{
    Ptr<PMenuItem> item = GetItemAt(index);

    if (item != nullptr) {
        return item->m_SubMenu;
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PMenu::GetItemCount() const
{
    return m_Items.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PMenu::GetIndexOf(Ptr<PMenuItem> item) const
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

size_t PMenu::GetIndexOf(Ptr<PMenu> menu) const
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

Ptr<PMenuItem> PMenu::FindItem(int id) const
{
    for (Ptr<PMenuItem> item : m_Items)
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

Ptr<PMenuItem> PMenu::FindItem(const PString& name) const
{
    for (Ptr<PMenuItem> item : m_Items)
    {
        if (item->m_SubMenu != nullptr)
        {
            Ptr<PMenuItem> subItem = item->m_SubMenu->FindItem(name);
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

Ptr<PMenuItem> PMenu::FindMarked() const
{
    for (Ptr<PMenuItem> item : m_Items)
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

Ptr<PMenu> PMenu::GetSuperMenu() const
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

Ptr<PMenuItem> PMenu::GetSuperItem()
{
    return ptr_tmp_cast(m_SuperItem);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenu::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    *minSize = m_ContentSize + PPoint(4.0f, 4.0f);
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenu::InvalidateLayout()
{
    m_ContentSize = PPoint(0.0f, 0.0f);

    switch (m_Layout)
    {
        case PMenuLayout::Horizontal:
            for (Ptr<PMenuItem> item : m_Items)
            {
                PPoint   itemContentSize = item->GetContentSize();

                item->m_Frame = PRect(m_ContentSize.x + m_ItemBorders.left, m_ItemBorders.top, m_ContentSize.x + itemContentSize.x + m_ItemBorders.left, itemContentSize.y + m_ItemBorders.top);
                m_ContentSize.x += itemContentSize.x + m_ItemBorders.left + m_ItemBorders.right;

                if (itemContentSize.y + m_ItemBorders.top + m_ItemBorders.bottom > m_ContentSize.y) {
                    m_ContentSize.y = itemContentSize.y + m_ItemBorders.top + m_ItemBorders.bottom;
                }
            }
            break;
        case PMenuLayout::Vertical:
            for (Ptr<PMenuItem> item : m_Items)
            {
                PPoint   itemContentSize = item->GetContentSize();

                item->m_Frame = PRect(m_ItemBorders.left, m_ContentSize.y + m_ItemBorders.top, itemContentSize.x + m_ItemBorders.left, m_ContentSize.y + itemContentSize.y + m_ItemBorders.top);

                m_ContentSize.y += itemContentSize.y + m_ItemBorders.top + m_ItemBorders.bottom;

                if (itemContentSize.x + m_ItemBorders.left + m_ItemBorders.right > m_ContentSize.x) {
                    m_ContentSize.x = itemContentSize.x + m_ItemBorders.left + m_ItemBorders.right;
                }
            }
            for (Ptr<PMenuItem> item : m_Items) {
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

void PMenu::InvalidateItem(PMenuItem* item)
{
    m_ContentView->Invalidate(item->GetFrame());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PBitmap> PMenu::GetArrowRightBitmap()
{
    if (m_ArrowRightBitmap == nullptr) {
        m_ArrowRightBitmap = ptr_new<PBitmap>(LEFT_ARROW_WIDTH, LEFT_ARROW_HEIGHT, PEColorSpace::MONO1, g_ArrowBitmapRaster, sizeof(uint32_t));
    }
    return m_ArrowRightBitmap;
}
