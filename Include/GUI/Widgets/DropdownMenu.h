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

namespace os
{

class TextBox;
class DropdownMenu;


namespace DropdownMenuFlags
{
// When the DropdownMenu is in read-only mode the user will not be able
// to edit the contents of the edit box.It will also make the menu open
// when the user click inside the edit box.

static constexpr uint32_t ReadOnly    = 0x01 << ViewFlags::FirstUserBit;

extern const std::map<PString, uint32_t> FlagMap;
}


/** Edit box with an asociated item-menu.
 * \ingroup gui
 * \par Description:
 *
 * \sa TextBox
 * \author  Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class DropdownMenu : public Control
{
public:
    DropdownMenu(const PString& name = PString::zero, Ptr<View> parent = nullptr, uint32_t flags = 0);
    DropdownMenu(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);
    ~DropdownMenu();

    // From View:
    virtual void    OnFlagsChanged(uint32_t changedFlags) override;
    virtual void    DetachedFromScreen() override;
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void    OnFrameSized(const Point& cDelta) override;
    virtual void    OnScreenFrameMoved(const Point& delta) override;
    virtual bool    OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual void    OnPaint(const Rect& cUpdateRect) override;

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

    Signal<void (size_t index, bool finalUpdate, DropdownMenu* sourceMenu)>         SignalSelectionChanged;
    Signal<void (const PString& text, bool finalUpdate, DropdownMenu* sourceMenu)>   SignalTextChanged;
private:
    void    Initialize();
    void    OpenMenu();
    void    CloseMenu();
    void    LayoutMenuWindow();
    void    SlotTextChanged(const PString& text, bool finalUpdate);
    void    SlotSelectionChanged(size_t selection, bool finalUpdate);

    Ptr<View>               m_MenuWindow;
    Ptr<TextBox>            m_EditBox;
    Rect                    m_ArrowFrame;
    std::vector<PString>    m_StringList;
    size_t                  m_Selection = 0;
    bool                    m_SendIntermediateEvents = false;
};

}
