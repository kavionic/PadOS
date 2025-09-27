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

#include <GUI/Widgets/Dropdownmenu.h>
#include <GUI/Widgets/TextBox.h>
#include <GUI/Bitmap.h>
#include <Utils/Utils.h>
#include <Utils/XMLFactory.h>
#include <Utils/XMLObjectParser.h>

#include "DropdownMenuPopupView.h"

namespace os
{
using namespace osi;


static const uint32_t g_ArrowBitmapRaster[] =
{
    0b11111111111111111110000000000000,
    0b01111111111111111100000000000000,
    0b00111111111111111000000000000000,
    0b00011111111111110000000000000000,
    0b00001111111111100000000000000000,
    0b00000111111111000000000000000000,
    0b00000011111110000000000000000000,
    0b00000001111100000000000000000000,
    0b00000000111000000000000000000000,
    0b00000000010000000000000000000000
};

static constexpr int    ARROW_WIDTH = 19;
static constexpr int    ARROW_HEIGHT = 10;
static constexpr float  ARROW_BUTTON_ASPECT_RATIO = 0.9f;

static Ptr<Bitmap> g_ArrowBitmap;

const std::map<String, uint32_t> DropdownMenuFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(DropdownMenuFlags, ReadOnly)
};

/** DropdownMenu constructor
 * \param cFrame - The size and position of the edit box and it's associated button.
 * \param pzName - The identifier passed down to the Handler class (Never rendered)
 * \param nResizeMask - Flags describing which edge follows edges of the parent
 *          when the parent is resized (Se View::View())
 * \param nFlags - Various flags passed to the View::View() constructor.
 * \sa os::View, os::Invoker
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

DropdownMenu::DropdownMenu(const String& name, Ptr<View> parent, uint32_t flags) :
    Control(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize)
{
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenu::DropdownMenu(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData) : Control(context, parent, xmlData)
{
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, DropdownMenuFlags::FlagMap, "flags", 0) | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize);
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::Initialize()
{
    m_EditBox = ptr_new<TextBox>("text_view", String::zero, ptr_tmp_cast(this), HasFlags(DropdownMenuFlags::ReadOnly) ? (TextBoxFlags::ReadOnly | TextBoxFlags::RaisedFrame) : 0);
    OnFrameSized(Point(0, 0));

    m_EditBox->SignalTextChanged.Connect(this, &DropdownMenu::SlotTextChanged);

    if (g_ArrowBitmap == nullptr)
    {
        g_ArrowBitmap = ptr_new<Bitmap>(ARROW_WIDTH, ARROW_HEIGHT, EColorSpace::MONO1, g_ArrowBitmapRaster, sizeof(uint32_t));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenu::~DropdownMenu()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::OnFlagsChanged(uint32_t changedFlags)
{
    Control::OnFlagsChanged(changedFlags);
    if ((changedFlags & DropdownMenuFlags::ReadOnly) && m_EditBox != nullptr)
    {
        if (HasFlags(DropdownMenuFlags::ReadOnly)) {
            m_EditBox->MergeFlags(TextBoxFlags::ReadOnly | TextBoxFlags::RaisedFrame);
        } else {
            m_EditBox->ClearFlags(TextBoxFlags::ReadOnly | TextBoxFlags::RaisedFrame);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::DetachedFromScreen()
{
    CloseMenu();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    Point size = (m_StringList.empty()) ? m_EditBox->GetPreferredSize(PrefSizeType::Smallest) : m_EditBox->GetSizeForString(m_StringList[0]);

    if (includeWidth)
    {
        for (size_t i = 1; i < m_StringList.size(); ++i)
        {
            Point curSize = m_EditBox->GetSizeForString(m_StringList[i], true, false);

            if (curSize.x > size.x) {
                size.x = curSize.x;
            }
        }
        size.x += 10.0f + std::round(size.y * ARROW_BUTTON_ASPECT_RATIO);
    }

    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::OnFrameSized(const Point& cDelta)
{
    Rect editFrame = GetBounds();

    m_ArrowFrame = Rect(editFrame.right - std::round(editFrame.Height() * ARROW_BUTTON_ASPECT_RATIO), 0.0f, editFrame.right, editFrame.bottom);

    editFrame.right = m_ArrowFrame.left;
    m_EditBox->SetFrame(editFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::OnScreenFrameMoved(const Point& delta)
{
    LayoutMenuWindow();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DropdownMenu::OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (!m_EditBox->IsEnabled()) {
        return View::OnMouseDown(button, position, event);
    }
    if (m_MenuWindow == nullptr)
    {
        if (!m_StringList.empty()) {
            OpenMenu();
        }
    }
    else
    {
        CloseMenu();
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::OnPaint(const Rect& cUpdateRect)
{
    SetEraseColor(StandardColorID::DefaultBackground);
    DrawFrame(m_ArrowFrame, (m_MenuWindow != nullptr) ? FRAME_RECESSED : FRAME_RAISED);

    Point center(m_ArrowFrame.left + m_ArrowFrame.Width() * 0.5f, m_ArrowFrame.top + m_ArrowFrame.Height() * 0.5f);
    center.Round();

    if (!m_EditBox->IsEnabled())
    {
    }

    const Rect arrowFrame(0.0f, 0.0f, float(ARROW_WIDTH), float(ARROW_HEIGHT));

    SetDrawingMode(DrawingMode::Overlay);
    SetFgColor(0, 0, 0);
    if (m_EditBox->IsEnabled())
    {
        DrawBitmap(g_ArrowBitmap, arrowFrame, center - Point(9.0f, 4.0f));
    }
    else
    {
        SetFgColor(255, 255, 255);
        DrawBitmap(g_ArrowBitmap, arrowFrame, center - Point(8.0f, 4.0f));
        SetFgColor(110, 110, 110);
        DrawBitmap(g_ArrowBitmap, arrowFrame, center - Point(9.0f, 5.0f));
    }
    SetDrawingMode(DrawingMode::Copy);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::OnEnableStatusChanged(bool isEnabled)
{
    m_EditBox->SetEnable(isEnabled);
    Invalidate();
    Flush();
}

/** Add a item to the end of the drop down list.
 * \param pzString - The string to be appended
 * \sa InsertItem(), DeleteItem(), GetItemCount(), GetItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::AppendItem(const String& text)
{
    m_StringList.push_back(text);
    PreferredSizeChanged();
}

/** Insert and item at a given position.
 * \Description:
 *  The new item is inserted before the nPosition'th item.
 * \param nPosition - Zero based index of the item to insert the string in front of
 * \param pzString  - The string to be insert
 * \sa AppendItem(), DeleteItem(), GetItemCount(), GetItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::InsertItem(size_t index, const String& text)
{
    m_StringList.insert(m_StringList.begin() + index, text);
    PreferredSizeChanged();
}

/** Delete a item
 * \par Description:
 *  Delete the item at the given position
 * \param nPosition - The zero based index of the item to delete.
 * \return true if an item was deleted, false if the index was out of range.
 * \sa AppendItem(), InsertItem(), GetItemCount(), GetItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////


bool DropdownMenu::DeleteItem(size_t index)
{
    if (index < m_StringList.size())
    {
        m_StringList.erase(m_StringList.begin() + index);
        PreferredSizeChanged();
        return true;
    }
    return false;
}

/** Get the item count
 * \return The number of items in the menu
 * \sa GetItem(), AppendItem(), InsertItem(), DeleteItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

size_t DropdownMenu::GetItemCount() const
{
    return m_StringList.size();
}

/** Delete all items
 * \par Description:
 *  Delete all the items in the menu
 * \param nPosition - The zero based index of the item to delete.
 * \return true if an item was deleted, false if the index was out of range.
 * \sa AppendItem(), InsertItem(), GetItemCount(), GetItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::Clear()
{
    m_StringList.clear();
    PreferredSizeChanged();
}

/** Get one of the item strings.
 * \par Description:
 *  Returns one of the items encapsulated in a stl string.
 * \param nItem - Zero based index of the item to return.
 * \return const reference to an stl string containing the item string
 * \sa GetItemCount(), AppendItem(), InsertItem(), DeleteItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

const String& DropdownMenu::GetItem(size_t index) const
{
    assert(index < m_StringList.size());
    return(m_StringList[index]);
}

/** Get the current selection
 * \return The index of the selected item, or -1 if no item is selected.
 * \sa SetSelection(), SetSelectionMessage()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

size_t DropdownMenu::GetSelection() const
{
    return m_Selection;
}

/** Set current selection
 * \par Description:
 *  The given item will be highlighted when the menu is opened
 *  and the item will be copied into the edit box.
 *  If the notify parameter is true and the selection differs from the current
 *  the "SelectionMessage" (see SetSelectionMessage()) will be sendt to the
 *  target set by SetTarget().
 * \param nItem - The new selection
 * \param bNotify - If true a notification will be sent if the new selection
 *          differ from the current.
 * \sa GetSelection(), SetSelectionMessage(), GetSelectionMessage(), Invoker::SetTarget()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

void DropdownMenu::SetSelection(size_t index, bool notify)
{
    if (index < m_StringList.size())
    {
        m_EditBox->SetText(m_StringList[index]);

        if (index != m_Selection)
        {
            m_Selection = index;
            if (notify) {
                SignalSelectionChanged(m_Selection, true, this);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const String& DropdownMenu::GetCurrentString() const
{
    return m_EditBox->GetText();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::SetCurrentString(const String& cString)
{
    m_EditBox->SetText(cString);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::OpenMenu()
{
    Ptr<DropdownMenuPopupWindow> pcMenuView = ptr_new<DropdownMenuPopupWindow>(m_StringList, m_Selection);
    m_MenuWindow = pcMenuView;

    pcMenuView->SignalSelectionChanged.Connect(this, &DropdownMenu::SlotSelectionChanged);

    LayoutMenuWindow();
    pcMenuView->MakeSelectionVisible();

    Application* app = GetApplication();
    if (app != nullptr) {
        app->AddView(m_MenuWindow, ViewDockType::PopupWindow);
    }
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::CloseMenu()
{
    if (m_MenuWindow != nullptr)
    {
        Application* app = GetApplication();
        if (app != nullptr)
        {
            app->RemoveView(m_MenuWindow);
            m_MenuWindow = nullptr;
            Invalidate();
        }
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::LayoutMenuWindow()
{
    if (m_MenuWindow != nullptr)
    {
        Rect screenFrame(100.0f, 0.0f, 800.0f, 480.0f); // FIXME: Query this data from the application server.

        screenFrame.Resize(5.0f, 20.0f, -5.0f, -20.0f);

        Rect menuFrame = GetBounds();
        ConvertToScreen(&menuFrame);

        const Point menuSize = m_MenuWindow->GetPreferredSize(PrefSizeType::Smallest);

        if (menuFrame.Width() < menuSize.x) {
            menuFrame.right = menuFrame.left + menuSize.x;
        }
        if (menuFrame.right > screenFrame.right) {
            menuFrame += Point(screenFrame.right - menuFrame.right, 0.0f);
        }
        if (menuFrame.left < screenFrame.left) {
            menuFrame.left = screenFrame.left;
        }

        const float spaceAbove = menuFrame.top - screenFrame.top;
        const float spaceBelow = screenFrame.bottom - menuFrame.bottom;
        const bool menuBelow = spaceBelow >= menuSize.y || spaceBelow >= spaceAbove;

        if (menuBelow)
        {
            menuFrame.top = menuFrame.bottom;
            menuFrame.bottom = menuFrame.top + menuSize.y;
            if (menuFrame.bottom > screenFrame.bottom) menuFrame.bottom = screenFrame.bottom;
        }
        else
        {
            menuFrame.bottom = menuFrame.top;
            menuFrame.top = menuFrame.bottom - menuSize.y;
            if (menuFrame.top < screenFrame.top) menuFrame.top = screenFrame.top;
        }

        m_MenuWindow->SetFrame(menuFrame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::SlotTextChanged(const String& text, bool finalUpdate)
{
    SignalTextChanged(text, finalUpdate, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenu::SlotSelectionChanged(size_t selection, bool finalUpdate)
{
    if (finalUpdate || selection != m_Selection)
    {
        m_Selection = selection;

        if (finalUpdate && m_Selection < m_StringList.size()) {
            m_EditBox->SetText(m_StringList[m_Selection]);
        }

        if (m_Selection < m_StringList.size())
        {
            if (m_SendIntermediateEvents || finalUpdate)
            {
                SignalSelectionChanged(m_Selection, finalUpdate, this);
            }
        }
    }
    if (finalUpdate) {
        CloseMenu();
    }
}

} // namespace os
