// This file is part of PadOS.
//
// Copyright (c) 1999-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/DropdownMenu.h>
#include <GUI/Widgets/TextBox.h>
#include <Utils/Utils.h>
#include <Utils/XMLFactory.h>
#include <Utils/XMLObjectParser.h>

#include "DropdownMenuPopupView.h"

using namespace osi;


const std::map<PString, PDropdownMenuFlags> PDROPDOWN_MENU_FLAG_MAP
{
    DEFINE_FLAG_MAP_ENTRY(PDropdownMenuFlag, ReadOnly)
};

/** DropdownMenu constructor
 * \param name - The identifier passed down to the Handler class (Never rendered)
 * \param parent - If not null, add as a child to this view.
 * \param flags - Various flags passed to the View::View() constructor.
 * \sa os::View, os::Invoker
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

PDropdownMenu::PDropdownMenu(const PString& name, PDropdownMenuFlags flags) :
    PControl(name, flags | PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize)
{
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDropdownMenu::PDropdownMenu(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PControl(context, xmlData)
{
    const PDropdownMenuFlags flags = context.GetFlagsAttribute<PDropdownMenuFlags>(
        xmlData,
        PDROPDOWN_MENU_FLAG_MAP,
        "flags",
        PDropdownMenuFlags::zero);
    MergeFlags(flags | PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize);
    Initialize();
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

void PDropdownMenu::OnFlagsChanged(PViewFlags changedFlags)
{
    PControl::OnFlagsChanged(changedFlags);
    if (PDropdownMenuFlags(changedFlags).Has(PDropdownMenuFlag::ReadOnly) && m_EditBox != nullptr)
    {
        m_DropdownMenuStyleData.IsReadOnly = HasFlags(PDropdownMenuFlag::ReadOnly);
        if (m_DropdownMenuStyleData.IsReadOnly) {
            m_EditBox->MergeFlags(PTextBoxFlag::ReadOnly | PTextBoxFlag::RaisedFrame);
        } else {
            m_EditBox->ClearFlags(PTextBoxFlag::ReadOnly | PTextBoxFlag::RaisedFrame);
        }
        NotifyStylePropertyChanged(PDropdownMenuProperty::ReadOnly);
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

void PDropdownMenu::OnFontChanged(Ptr<const PFont> newFont)
{
    m_EditBox->SetFont(newFont);
    m_DropdownMenuStyleData.EditorSize = m_EditBox->GetSizeForString(PString::zero);
    NotifyStylePropertyChanged(PDropdownMenuProperty::Font);
    if (m_MenuWindow != nullptr)
    {
        m_MenuWindow->SetFont(newFont);
        LayoutMenuWindow();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::OnStyleChanged()
{
    m_DropdownMenuStyleData.StyleData.Reset();
    CloseMenu();
    m_DropdownMenuStyleData.EditorSize = m_EditBox->GetSizeForString(PString::zero);
    LayoutEditor();
    PControl::OnStyleChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->DropdownMenuGetPreferredSize(*this, m_DropdownMenuStyleData, minSize, maxSize);
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::OnFrameSized(const PPoint& cDelta)
{
    PControl::OnFrameSized(cDelta);
    NotifyStylePropertyChanged(PDropdownMenuProperty::Frame);
    LayoutMenuWindow();
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

void PDropdownMenu::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    const PGUIStyle* style = GetStyle();
    if (style == nullptr || !style->DropdownMenuHitTest(*this, m_DropdownMenuStyleData, position))
    {
        PView::OnPointerDown(pointerID, position, event, phase);
        return;
    }
    if (m_MenuWindow == nullptr)
    {
        if (!m_DropdownMenuStyleData.Items.empty()) {
            OpenMenu();
        }
    }
    else
    {
        CloseMenu();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::OnPaint(const PRect& cUpdateRect)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->DropdownMenuPaint(*this, GetBounds(), m_DropdownMenuStyleData);
    } else {
        PView::OnPaint(cUpdateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::OnEnableStatusChanged(bool isEnabled)
{
    m_DropdownMenuStyleData.IsEnabled = isEnabled;
    m_EditBox->SetEnable(isEnabled);
    NotifyStylePropertyChanged(PDropdownMenuProperty::Enabled);
    Flush();
}

/** Add a item to the end of the drop down list.
 * \param text - The string to be appended
 * \sa InsertItem(), DeleteItem(), GetItemCount(), GetItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::AppendItem(const PString& text)
{
    m_DropdownMenuStyleData.Items.push_back(text);
    NotifyStylePropertyChanged(PDropdownMenuProperty::Items);
}

/** Insert and item at a given position.
 * \par Description:
 *  The new item is inserted before the nPosition'th item.
 * \param index - Zero based index of the item to insert the string in front of
 * \param text  - The string to be insert
 * \sa AppendItem(), DeleteItem(), GetItemCount(), GetItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::InsertItem(size_t index, const PString& text)
{
    m_DropdownMenuStyleData.Items.insert(m_DropdownMenuStyleData.Items.begin() + index, text);
    NotifyStylePropertyChanged(PDropdownMenuProperty::Items);
}

/** Delete a item
 * \par Description:
 *  Delete the item at the given position
 * \param index - The zero based index of the item to delete.
 * \return true if an item was deleted, false if the index was out of range.
 * \sa AppendItem(), InsertItem(), GetItemCount(), GetItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////


bool PDropdownMenu::DeleteItem(size_t index)
{
    if (index < m_DropdownMenuStyleData.Items.size())
    {
        m_DropdownMenuStyleData.Items.erase(m_DropdownMenuStyleData.Items.begin() + index);
        NotifyStylePropertyChanged(PDropdownMenuProperty::Items);
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
    return m_DropdownMenuStyleData.Items.size();
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
    m_DropdownMenuStyleData.Items.clear();
    NotifyStylePropertyChanged(PDropdownMenuProperty::Items);
}

/** Get one of the item strings.
 * \par Description:
 *  Returns one of the items encapsulated in a stl string.
 * \param index - Zero based index of the item to return.
 * \return const reference to an stl string containing the item string
 * \sa GetItemCount(), AppendItem(), InsertItem(), DeleteItem()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

const PString& PDropdownMenu::GetItem(size_t index) const
{
    assert(index < m_DropdownMenuStyleData.Items.size());
    return(m_DropdownMenuStyleData.Items[index]);
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
 * \param index - The new selection
 * \param notify - If true a notification will be sent if the new selection
 *          differ from the current.
 * \sa GetSelection(), SetSelectionMessage(), GetSelectionMessage(), Invoker::SetTarget()
 * \author  Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::SetSelection(size_t index, bool notify)
{
    if (index < m_DropdownMenuStyleData.Items.size())
    {
        m_EditBox->SetText(m_DropdownMenuStyleData.Items[index]);

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

void PDropdownMenu::Initialize()
{
    m_DropdownMenuStyleData.IsReadOnly = HasFlags(PDropdownMenuFlag::ReadOnly);
    m_DropdownMenuStyleData.IsEnabled = IsEnabled();
    const PTextBoxFlags editBoxFlags = m_DropdownMenuStyleData.IsReadOnly
        ? (PTextBoxFlag::ReadOnly | PTextBoxFlag::RaisedFrame)
        : PTextBoxFlags::zero;
    m_EditBox = ptr_new<PTextBox>("text_view", PString::zero, editBoxFlags);
    m_EditBox->SetEnable(IsEnabled());
    m_EditBox->SetFont(GetFont());
    m_DropdownMenuStyleData.Text = &m_EditBox->GetText();
    m_DropdownMenuStyleData.EditorSize = m_EditBox->GetSizeForString(PString::zero);
    AddChild(m_EditBox);
    LayoutEditor();
    m_EditBox->SignalTextChanged.Connect(this, &PDropdownMenu::SlotTextChanged);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::OpenMenu()
{
    PApplication* application = GetApplication();
    if (application == nullptr || GetStyle() == nullptr) {
        return;
    }
    Ptr<DropdownMenuPopupWindow> menuView = ptr_new<DropdownMenuPopupWindow>(m_DropdownMenuStyleData.Items, m_Selection);
    m_MenuWindow = menuView;
    menuView->SignalSelectionChanged.Connect(this, &PDropdownMenu::SlotSelectionChanged);
    menuView->SignalPreferredSizeChanged.Connect(this, &PDropdownMenu::LayoutMenuWindow);
    menuView->SetFont(GetFont());

    application->AddView(m_MenuWindow, PViewDockType::PopupWindow);
    LayoutMenuWindow();
    menuView->MakeSelectionVisible();
    m_DropdownMenuStyleData.IsOpen = true;
    NotifyStylePropertyChanged(PDropdownMenuProperty::Open);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::CloseMenu()
{
    if (m_MenuWindow != nullptr)
    {
        PApplication* application = GetApplication();
        if (application != nullptr) {
            application->RemoveView(m_MenuWindow);
        }
        m_MenuWindow.Reset();
        m_DropdownMenuStyleData.IsOpen = false;
        NotifyStylePropertyChanged(PDropdownMenuProperty::Open);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::LayoutMenuWindow()
{
    if (m_MenuWindow != nullptr && m_MenuWindow->GetStyle() != nullptr)
    {
        PRect screenFrame = PApplication::GetScreenFrame();

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

void PDropdownMenu::LayoutEditor()
{
    if (m_EditBox != nullptr)
    {
        const PGUIStyle* style = GetStyle();
        m_EditBox->SetFrame((style != nullptr) ? style->DropdownMenuGetEditorFrame(*this, m_DropdownMenuStyleData) : GetBounds());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::NotifyStylePropertyChanged(PDropdownMenuProperties properties)
{
    if (properties.Has(PDropdownMenuProperty::Items) && m_MenuWindow != nullptr)
    {
        if (m_DropdownMenuStyleData.Items.empty()) {
            CloseMenu();
        } else {
            ptr_static_cast<DropdownMenuPopupWindow>(m_MenuWindow)->ItemsChanged();
        }
    }
    const PGUIStyle* style = GetStyle();
    if (style != nullptr)
    {
        style->DropdownMenuPropertyChanged(*this, m_DropdownMenuStyleData, properties);
    }
    else
    {
        PreferredSizeChanged();
        Invalidate();
    }
    LayoutEditor();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDropdownMenu::SlotTextChanged(const PString& text, bool finalUpdate)
{
    NotifyStylePropertyChanged(PDropdownMenuProperty::Text);
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

        if (finalUpdate && m_Selection < m_DropdownMenuStyleData.Items.size()) {
            m_EditBox->SetText(m_DropdownMenuStyleData.Items[m_Selection]);
        }

        if (m_Selection < m_DropdownMenuStyleData.Items.size())
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
