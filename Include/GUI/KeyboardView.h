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

namespace os
{
class Menu;
class MenuItem;

struct KeyboardViewStyle : public PtrTarget
{
    DynamicColor BackgroundColor        = DynamicColor(NamedColors::black /*StandardColorID::DefaultBackground*/);
    DynamicColor NormalTextColor        = DynamicColor(NamedColors::white);
    DynamicColor SelectedTextColor      = DynamicColor(NamedColors::darkblue);
    DynamicColor LockedTextColor        = DynamicColor(NamedColors::white);
    DynamicColor ExtraTextColor         = DynamicColor(NamedColors::gainsboro);
    DynamicColor NormalButtonColor      = DynamicColor(NamedColors::dimgray);
    DynamicColor PressedButtonColor     = DynamicColor(NamedColors::mediumblue);
    DynamicColor SelectedButtonColor    = DynamicColor(NamedColors::dimgray);
    DynamicColor LockedButtonColor      = DynamicColor(NamedColors::darkblue);

    Ptr<Font>   LargeFont = ptr_new<Font>(Font_e::e_FontLarge);
    Ptr<Font>   SmallFont = ptr_new<Font>(Font_e::e_FontSmall);
};


struct KeyButton
{
    KeyButton(const Point& relativePos, float relativeWidth, float relativeHeight, KeyCodes normalKeyCode, KeyCodes lowerKeyCode, KeyCodes extraKeyCode, KeyCodes symbolKeyCode) noexcept
        : m_Width(relativeWidth), m_Height(relativeHeight), m_RelativePos(relativePos), m_NormalKeyCode(normalKeyCode), m_LowerKeyCode(lowerKeyCode), m_ExtraKeyCode(extraKeyCode), m_SymbolKeyCode(symbolKeyCode) {}

    Rect        m_Frame;
    float       m_Width;
    float       m_Height;
    Point       m_RelativePos;
    KeyCodes    m_NormalKeyCode;
    KeyCodes    m_LowerKeyCode;
    KeyCodes    m_ExtraKeyCode;
    KeyCodes    m_SymbolKeyCode;
};

enum class KeyboardLayoutType
{
    Normal,
    Symbols,
    Numerical
};

struct KeyboardLayout
{
    void Layout(const Rect& frame, const Point keySpacing, float rowHeight)
    {
        const float viewWidth  = frame.Width()  - keySpacing.x;
        const float viewHeight = frame.Height() - keySpacing.y;

        for (auto& button : m_KeyButtons)
        {
            Point position(std::round(button.m_RelativePos.x * viewWidth) + keySpacing.x, std::round(button.m_RelativePos.y * viewHeight) + keySpacing.y);
            button.m_Frame = Rect(position.x, position.y, position.x + std::floor(viewWidth * button.m_Width - keySpacing.x), position.y + std::floor(viewHeight * button.m_Height - keySpacing.y));
        }
    }
    void Clear() { m_KeyButtons.clear(); m_KeyRows = 0; }

    KeyboardLayoutType      Type;
    std::vector<KeyButton>  m_KeyButtons;
    size_t                  m_KeyRows = 0;
};

enum class CapsLockMode : uint8_t
{
    Off,
    Single,
    Locked
};

namespace KeyboardViewFlags
{
static constexpr uint32_t Numerical = 0x01 << ViewFlags::FirstUserBit;
}

class KeyboardView : public View
{
public:
    KeyboardView(const PString& name, Ptr<View> parent = nullptr, uint32_t flags = 0);

    static Ptr<KeyboardViewStyle> GetDefaultStyle() { return s_DefaultStyle; }

    // From ViewBase:
    virtual void OnFlagsChanged(uint32_t changedFlags) override;

    // From View:
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void OnFrameSized(const Point& delta) override;
    virtual void OnPaint(const Rect& updateRect) override;

    virtual bool OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool OnLongPress(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;

    // From KeyboardView:
    bool LoadKeyboard(const PString& name);

    Signal<void, KeyCodes, const PString&, KeyboardView*> SignalKeyPressed;
private:
    void    LoadConfig(PString& outSelectedKeyboard);
    void    SaveConfig(const PString& selectedKeyboard);

    void    SetLayout(KeyboardLayout* layout);
    PString GetKeyText(KeyCodes keyCode) const;
    void    DrawButton(const KeyButton& button, bool pressed);
    void    SetPressedButton(size_t index);
    size_t  GetButtonIndex(const Point& position) const;
    void    SlotRepeatTimer();
    void    SlotLayoutSelected(Ptr<MenuItem> item);

    static NoPtr<KeyboardViewStyle> s_DefaultStyle;
    Ptr<KeyboardViewStyle>          m_Style = s_DefaultStyle;

    Ptr<Bitmap>                 m_KeysBitmap;
    EventTimer                  m_RepeatTimer;

    float                       m_KeyHeight = 0.0f;

    std::vector<PString>        m_Keyboards;
    size_t                      m_SelectedKeyboard;
    KeyboardLayout              m_DefaultLayout;
    std::vector<KeyboardLayout> m_SymbolLayouts;
    KeyboardLayout*             m_CurrentLayout = &m_DefaultLayout;

    MouseButton_e               m_HitButton = MouseButton_e::None;
    Point                       m_HitPos;
    size_t                      m_PressedButton = INVALID_INDEX;
    CapsLockMode                m_CapsLockMode  = CapsLockMode::Single;
    size_t                      m_SymbolPage = 0;
    int                         m_PrevCursorPos = 0;
    Ptr<Menu>                   m_LayoutSelectMenu;
    bool                        m_IsDraggingCursor = false;
    bool                        m_SymbolsActive = false;

};

} // namespace os

