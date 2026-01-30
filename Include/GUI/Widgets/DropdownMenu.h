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

#pragma once

#include <GUI/Widgets/Control.h>
#include <GUI/Font.h>

#include <vector>
#include <string>

namespace osi
{
class DropdownMenuPopupWindow;
}

class PTextBox;
class PDropdownMenu;


namespace PDropdownMenuFlags
{
// When the DropdownMenu is in read-only mode the user will not be able
// to edit the contents of the edit box.It will also make the menu open
// when the user click inside the edit box.

static constexpr uint32_t ReadOnly    = 0x01 << PViewFlags::FirstUserBit;

extern const std::map<PString, uint32_t> FlagMap;
}


/** Edit box with an asociated item-menu.
 * \ingroup gui
 * \par Description:
 *
 * \sa TextBox
 * \author  Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class PDropdownMenu : public PControl
{
public:
    PDropdownMenu(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PDropdownMenu(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);
    ~PDropdownMenu();

    // From View:
    virtual void    OnFlagsChanged(uint32_t changedFlags) override;
    virtual void    DetachedFromScreen() override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void    OnFrameSized(const PPoint& cDelta) override;
    virtual void    OnScreenFrameMoved(const PPoint& delta) override;
    virtual bool    OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual void    OnPaint(const PRect& cUpdateRect) override;

    // From Control:
    virtual void    OnEnableStatusChanged(bool isEnabled) override;

//    void    SetReadOnly(bool bFlag = true);
//    bool    GetReadOnly() const;

    void            AppendItem(const PString& text);
    void            InsertItem(size_t index, const PString& text);
    bool            DeleteItem(size_t index);
    size_t          GetItemCount() const;
    void            Clear();
    const PString&  GetItem(size_t index) const;

    size_t          GetSelection() const;
    void            SetSelection(size_t index, bool notify = true);

    const PString&  GetCurrentString() const;
    void            SetCurrentString(const PString& string);

    Signal<void (size_t index, bool finalUpdate, PDropdownMenu* sourceMenu)>         SignalSelectionChanged;
    Signal<void (const PString& text, bool finalUpdate, PDropdownMenu* sourceMenu)>   SignalTextChanged;
private:
    void    Initialize();
    void    OpenMenu();
    void    CloseMenu();
    void    LayoutMenuWindow();
    void    SlotTextChanged(const PString& text, bool finalUpdate);
    void    SlotSelectionChanged(size_t selection, bool finalUpdate);

    Ptr<PView>               m_MenuWindow;
    Ptr<PTextBox>            m_EditBox;
    PRect                    m_ArrowFrame;
    std::vector<PString>    m_StringList;
    size_t                  m_Selection = 0;
    bool                    m_SendIntermediateEvents = false;
};
