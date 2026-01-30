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

static Ptr<PBitmap> g_ArrowBitmap;

const std::map<PString, uint32_t> PDropdownMenuFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(PDropdownMenuFlags, ReadOnly)
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

PDropdownMenu::PDropdownMenu(const PString& name, Ptr<PView> parent, uint32_t flags) :
    PControl(name, parent, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize)
{
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDropdownMenu::PDropdownMenu(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PControl(context, parent, xmlData)
{
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, PDropdownMenuFlags::FlagMap, "flags", 0) | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize);
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::Initialize()
{
    m_EditBox = ptr_new<PTextBox>("text_view", PString::zero, ptr_tmp_cast(this), HasFlags(PDropdownMenuFlags::ReadOnly) ? (PTextBoxFlags::ReadOnly | PTextBoxFlags::RaisedFrame) : 0);
    OnFrameSized(PPoint(0, 0));

    m_EditBox->SignalTextChanged.Connect(this, &PDropdownMenu::SlotTextChanged);

    if (g_ArrowBitmap == nullptr)
    {
        g_ArrowBitmap = ptr_new<PBitmap>(ARROW_WIDTH, ARROW_HEIGHT, PEColorSpace::MONO1, g_ArrowBitmapRaster, sizeof(uint32_t));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDropdownMenu::~PDropdownMenu()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::OnFlagsChanged(uint32_t changedFlags)
{
    PControl::OnFlagsChanged(changedFlags);
    if ((changedFlags & PDropdownMenuFlags::ReadOnly) && m_EditBox != nullptr)
    {
        if (HasFlags(PDropdownMenuFlags::ReadOnly)) {
            m_EditBox->MergeFlags(PTextBoxFlags::ReadOnly | PTextBoxFlags::RaisedFrame);
        } else {
            m_EditBox->ClearFlags(PTextBoxFlags::ReadOnly | PTextBoxFlags::RaisedFrame);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::DetachedFromScreen()
{
    CloseMenu();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    PPoint size = (m_StringList.empty()) ? m_EditBox->GetPreferredSize(PPrefSizeType::Smallest) : m_EditBox->GetSizeForString(m_StringList[0]);

    if (includeWidth)
    {
        for (size_t i = 1; i < m_StringList.size(); ++i)
        {
            PPoint curSize = m_EditBox->GetSizeForString(m_StringList[i], true, false);

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

void PDropdownMenu::OnFrameSized(const PPoint& cDelta)
{
    PRect editFrame = GetBounds();

    m_ArrowFrame = PRect(editFrame.right - std::round(editFrame.Height() * ARROW_BUTTON_ASPECT_RATIO), 0.0f, editFrame.right, editFrame.bottom);

    editFrame.right = m_ArrowFrame.left;
    m_EditBox->SetFrame(editFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::OnScreenFrameMoved(const PPoint& delta)
{
    LayoutMenuWindow();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDropdownMenu::OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (!m_EditBox->IsEnabled()) {
        return PView::OnMouseDown(button, position, event);
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

void PDropdownMenu::OnPaint(const PRect& cUpdateRect)
{
    SetEraseColor(PStandardColorID::DefaultBackground);
    DrawFrame(m_ArrowFrame, (m_MenuWindow != nullptr) ? FRAME_RECESSED : FRAME_RAISED);

    PPoint center(m_ArrowFrame.left + m_ArrowFrame.Width() * 0.5f, m_ArrowFrame.top + m_ArrowFrame.Height() * 0.5f);
    center.Round();

    if (!m_EditBox->IsEnabled())
    {
    }

    const PRect arrowFrame(0.0f, 0.0f, float(ARROW_WIDTH), float(ARROW_HEIGHT));

    SetDrawingMode(PDrawingMode::Overlay);
    SetFgColor(0, 0, 0);
    if (m_EditBox->IsEnabled())
    {
        DrawBitmap(g_ArrowBitmap, arrowFrame, center - PPoint(9.0f, 4.0f));
    }
    else
    {
        SetFgColor(255, 255, 255);
        DrawBitmap(g_ArrowBitmap, arrowFrame, center - PPoint(8.0f, 4.0f));
        SetFgColor(110, 110, 110);
        DrawBitmap(g_ArrowBitmap, arrowFrame, center - PPoint(9.0f, 5.0f));
    }
    SetDrawingMode(PDrawingMode::Copy);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::OnEnableStatusChanged(bool isEnabled)
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

void PDropdownMenu::AppendItem(const PString& text)
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

void PDropdownMenu::InsertItem(size_t index, const PString& text)
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


bool PDropdownMenu::DeleteItem(size_t index)
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

size_t PDropdownMenu::GetItemCount() const
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

void PDropdownMenu::Clear()
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

const PString& PDropdownMenu::GetItem(size_t index) const
{
    assert(index < m_StringList.size());
    return(m_StringList[index]);
}

/** Get the current selection
 * \return The index of the selected item, or -1 if no item is selected.
 * \sa SetSelection(), SetSelectionMessage()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

size_t PDropdownMenu::GetSelection() const
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

void PDropdownMenu::SetSelection(size_t index, bool notify)
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

const PString& PDropdownMenu::GetCurrentString() const
{
    return m_EditBox->GetText();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::SetCurrentString(const PString& cString)
{
    m_EditBox->SetText(cString);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::OpenMenu()
{
    Ptr<DropdownMenuPopupWindow> pcMenuView = ptr_new<DropdownMenuPopupWindow>(m_StringList, m_Selection);
    m_MenuWindow = pcMenuView;

    pcMenuView->SignalSelectionChanged.Connect(this, &PDropdownMenu::SlotSelectionChanged);

    LayoutMenuWindow();
    pcMenuView->MakeSelectionVisible();

    PApplication* app = GetApplication();
    if (app != nullptr) {
        app->AddView(m_MenuWindow, PViewDockType::PopupWindow);
    }
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::CloseMenu()
{
    if (m_MenuWindow != nullptr)
    {
        PApplication* app = GetApplication();
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

void PDropdownMenu::LayoutMenuWindow()
{
    if (m_MenuWindow != nullptr)
    {
        PRect screenFrame(100.0f, 0.0f, 800.0f, 480.0f); // FIXME: Query this data from the application server.

        screenFrame.Resize(5.0f, 20.0f, -5.0f, -20.0f);

        PRect menuFrame = GetBounds();
        ConvertToScreen(&menuFrame);

        const PPoint menuSize = m_MenuWindow->GetPreferredSize(PPrefSizeType::Smallest);

        if (menuFrame.Width() < menuSize.x) {
            menuFrame.right = menuFrame.left + menuSize.x;
        }
        if (menuFrame.right > screenFrame.right) {
            menuFrame += PPoint(screenFrame.right - menuFrame.right, 0.0f);
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

void PDropdownMenu::SlotTextChanged(const PString& text, bool finalUpdate)
{
    SignalTextChanged(text, finalUpdate, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::SlotSelectionChanged(size_t selection, bool finalUpdate)
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
