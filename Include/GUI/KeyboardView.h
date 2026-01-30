// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.09.2020 22:30

#pragma once

#include <stdint.h>

#include <GUI/View.h>
#include <Threads/EventTimer.h>
#include <Utils/UTF8Utils.h>
#include <Ptr/NoPtr.h>

class PMenu;
class PMenuItem;


struct PKeyboardViewStyle : public PtrTarget
{
    PDynamicColor BackgroundColor        = PDynamicColor(PNamedColors::black /*StandardColorID::DefaultBackground*/);
    PDynamicColor NormalTextColor        = PDynamicColor(PNamedColors::white);
    PDynamicColor SelectedTextColor      = PDynamicColor(PNamedColors::darkblue);
    PDynamicColor LockedTextColor        = PDynamicColor(PNamedColors::white);
    PDynamicColor ExtraTextColor         = PDynamicColor(PNamedColors::gainsboro);
    PDynamicColor NormalButtonColor      = PDynamicColor(PNamedColors::dimgray);
    PDynamicColor PressedButtonColor     = PDynamicColor(PNamedColors::mediumblue);
    PDynamicColor SelectedButtonColor    = PDynamicColor(PNamedColors::dimgray);
    PDynamicColor LockedButtonColor      = PDynamicColor(PNamedColors::darkblue);

    Ptr<PFont>   LargeFont = ptr_new<PFont>(PFontID::e_FontLarge);
    Ptr<PFont>   SmallFont = ptr_new<PFont>(PFontID::e_FontSmall);
};


struct PKeyButton
{
    PKeyButton(const PPoint& relativePos, float relativeWidth, float relativeHeight, PKeyCodes normalKeyCode, PKeyCodes lowerKeyCode, PKeyCodes extraKeyCode, PKeyCodes symbolKeyCode) noexcept
        : m_Width(relativeWidth), m_Height(relativeHeight), m_RelativePos(relativePos), m_NormalKeyCode(normalKeyCode), m_LowerKeyCode(lowerKeyCode), m_ExtraKeyCode(extraKeyCode), m_SymbolKeyCode(symbolKeyCode) {}

    PRect        m_Frame;
    float       m_Width;
    float       m_Height;
    PPoint       m_RelativePos;
    PKeyCodes    m_NormalKeyCode;
    PKeyCodes    m_LowerKeyCode;
    PKeyCodes    m_ExtraKeyCode;
    PKeyCodes    m_SymbolKeyCode;
};

enum class PKeyboardLayoutType
{
    Normal,
    Symbols,
    Numerical
};

struct PKeyboardLayout
{
    void Layout(const PRect& frame, const PPoint keySpacing, float rowHeight)
    {
        const float viewWidth  = frame.Width()  - keySpacing.x;
        const float viewHeight = frame.Height() - keySpacing.y;

        for (auto& button : m_KeyButtons)
        {
            PPoint position(std::round(button.m_RelativePos.x * viewWidth) + keySpacing.x, std::round(button.m_RelativePos.y * viewHeight) + keySpacing.y);
            button.m_Frame = PRect(position.x, position.y, position.x + std::floor(viewWidth * button.m_Width - keySpacing.x), position.y + std::floor(viewHeight * button.m_Height - keySpacing.y));
        }
    }
    void Clear() { m_KeyButtons.clear(); m_KeyRows = 0; }

    PKeyboardLayoutType      Type;
    std::vector<PKeyButton>  m_KeyButtons;
    size_t                  m_KeyRows = 0;
};

enum class PCapsLockMode : uint8_t
{
    Off,
    Single,
    Locked
};

namespace PKeyboardViewFlags
{
static constexpr uint32_t Numerical = 0x01 << PViewFlags::FirstUserBit;
}

class PKeyboardView : public PView
{
public:
    PKeyboardView(const PString& name, Ptr<PView> parent = nullptr, uint32_t flags = 0);

    static Ptr<PKeyboardViewStyle> GetDefaultStyle() { return s_DefaultStyle; }

    // From ViewBase:
    virtual void OnFlagsChanged(uint32_t changedFlags) override;

    // From View:
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void OnPaint(const PRect& updateRect) override;

    virtual bool OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool OnLongPress(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;

    // From KeyboardView:
    bool LoadKeyboard(const PString& name);

    Signal<void, PKeyCodes, const PString&, PKeyboardView*> SignalKeyPressed;
private:
    void    LoadConfig(PString& outSelectedKeyboard);
    void    SaveConfig(const PString& selectedKeyboard);

    void    SetLayout(PKeyboardLayout* layout);
    PString GetKeyText(PKeyCodes keyCode) const;
    void    DrawButton(const PKeyButton& button, bool pressed);
    void    SetPressedButton(size_t index);
    size_t  GetButtonIndex(const PPoint& position) const;
    void    SlotRepeatTimer();
    void    SlotLayoutSelected(Ptr<PMenuItem> item);

    static NoPtr<PKeyboardViewStyle> s_DefaultStyle;
    Ptr<PKeyboardViewStyle>          m_Style = s_DefaultStyle;

    Ptr<PBitmap>                 m_KeysBitmap;
    PEventTimer                  m_RepeatTimer;

    float                       m_KeyHeight = 0.0f;

    std::vector<PString>        m_Keyboards;
    size_t                      m_SelectedKeyboard;
    PKeyboardLayout              m_DefaultLayout;
    std::vector<PKeyboardLayout> m_SymbolLayouts;
    PKeyboardLayout*             m_CurrentLayout = &m_DefaultLayout;

    PMouseButton               m_HitButton = PMouseButton::None;
    PPoint                       m_HitPos;
    size_t                      m_PressedButton = INVALID_INDEX;
    PCapsLockMode                m_CapsLockMode  = PCapsLockMode::Single;
    size_t                      m_SymbolPage = 0;
    int                         m_PrevCursorPos = 0;
    Ptr<PMenu>                   m_LayoutSelectMenu;
    bool                        m_IsDraggingCursor = false;
    bool                        m_SymbolsActive = false;

};
