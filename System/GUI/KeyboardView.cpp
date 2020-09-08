// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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

#include <GUI/Bitmap.h>
#include <GUI/KeyboardView.h>
#include <Utils/Utils.h>


namespace os
{

static const uint32_t g_BackspaceRaster[] =
{
    0b000000000011111111111111111111,
    0b000000000111111111111111111111,
    0b000000001111111111111111111111,
    0b000000011111111111111111111111,
    0b000000111111111111111111111111,
    0b000001111111100011111111000111,
    0b000011111111110001111110001111,
    0b000111111111111000111100011111,
    0b001111111111111100011000111111,
    0b011111111111111110000001111111,
    0b111111111111111111000111111111,
    0b011111111111111110000001111111,
    0b001111111111111100011000111111,
    0b000111111111111000111100011111,
    0b000011111111110001111110001111,
    0b000001111111111111111111111111,
    0b000000111111111111111111111111,
    0b000000011111111111111111111111,
    0b000000001111111111111111111111,
    0b000000000111111111111111111111,
    0b000000000011111111111111111111,
};

static const uint32_t g_ShiftRaster[] =
{
    0b000000000001000000000000000000,
    0b000000000011100000000000000000,
    0b000000000111110000000000000000,
    0b000000001111111000000000000000,
    0b000000011111111100000000000000,
    0b000000111111111110000000000000,
    0b000001111111111111000000000000,
    0b000011111111111111100000000000,
    0b000111111111111111110000000000,
    0b001111111111111111111000000000,
    0b011111111111111111111100000000,
    0b111111111111111111111110000000,
    0b000000011111111100000000000000,
    0b000000011111111100000000000000,
    0b000000011111111100000000000000,
    0b000000011111111100000000000000,
    0b000000011111111100000000000000,
    0b000000011111111100000000000000,
    0b000000011111111100000000000000,
    0b000000011111111100000000000000,
    0b000000011111111100000000000000,
    0b000000011111111100000000000000,
};

static const uint32_t g_EnterRaster[] =
{
    0b000000000000000000000000111111,
    0b000000000000000000000000111111,
    0b000000000000000000000000111111,
    0b000000000000000000000000111111,
    0b000000000100000000000000111111,
    0b000000001100000000000000111111,
    0b000000111100000000000000111111,
    0b000001111100000000000000111111,
    0b000011111100000000000000111111,
    0b000111111100000000000000111111,
    0b001111111111111111111111111111,
    0b011111111111111111111111111111,
    0b111111111111111111111111111111,
    0b011111111111111111111111111111,
    0b001111111111111111111111111111,
    0b000111111100000000000000000000,
    0b000011111100000000000000000000,
    0b000001111100000000000000000000,
    0b000000111100000000000000000000,
    0b000000011100000000000000000000,
    0b000000001100000000000000000000,
    0b000000000100000000000000000000,
};

static const uint32_t g_SpaceRaster[] =
{
    0b111100000000000000000000001111,
    0b111100000000000000000000001111,
    0b111100000000000000000000001111,
    0b111100000000000000000000001111,
    0b111111111111111111111111111111,
    0b111111111111111111111111111111,
    0b111111111111111111111111111111,
};

static constexpr IRect KEY_BM_FRAME_BACKSPACE(0, 0, 32, sizeof(g_BackspaceRaster) / sizeof(uint32_t));
static constexpr IRect KEY_BM_FRAME_SHIFT(0, KEY_BM_FRAME_BACKSPACE.bottom, 23, KEY_BM_FRAME_BACKSPACE.bottom + sizeof(g_ShiftRaster) / sizeof(uint32_t));
static constexpr IRect KEY_BM_FRAME_ENTER(0, KEY_BM_FRAME_SHIFT.bottom, 32, KEY_BM_FRAME_SHIFT.bottom + sizeof(g_EnterRaster) / sizeof(uint32_t));
static constexpr IRect KEY_BM_FRAME_SPACE(0, KEY_BM_FRAME_ENTER.bottom, 32, KEY_BM_FRAME_ENTER.bottom + sizeof(g_SpaceRaster) / sizeof(uint32_t));

static constexpr IPoint KEYS_BITMAP_SIZE(32, KEY_BM_FRAME_BACKSPACE.Height() + KEY_BM_FRAME_SHIFT.Height() + KEY_BM_FRAME_ENTER.Height() + KEY_BM_FRAME_SPACE.Height());

static constexpr float W_NUM = 1.0f / 10.0f;
static constexpr float W_CHR = 1.0f / 11.0f;
static constexpr float W_05  = 0.5f / 11.0f;
static constexpr float W_15  = 1.5f / 11.0f;
static constexpr float W_20  = 2.0f / 11.0f;

static constexpr Point KEY_SPACING(5.0f, 5.0f);

struct KLNode
{
    constexpr KLNode(float width, KeyCodes keyCode) : m_Width(width), m_KeyCode(keyCode) {}
    constexpr KLNode(float width, const char* keyCode) : m_Width(width), m_KeyCode(KeyCodes(utf8_to_unicode(keyCode))) {}
    float       m_Width;
    KeyCodes    m_KeyCode;
};

using K = KLNode;
using KC = KeyCodes;
static constexpr std::array<std::array<KLNode, 11>, 5> g_KeyMap
{
    std::array<K, 11>{K(W_NUM, KC::NUM_1), K(W_NUM, KC::NUM_2), K(W_NUM, KC::NUM_3), K(W_NUM, KC::NUM_4), K(W_NUM, KC::NUM_5), K(W_NUM, KC::NUM_6), K(W_NUM, KC::NUM_7), K(W_NUM, KC::NUM_8), K(W_NUM, KC::NUM_9), K(W_NUM, KC::NUM_0), K(0.0f, KC::NONE)},
    std::array<K, 11>{K(W_CHR, KC::Q), K(W_CHR, KC::W), K(W_CHR, KC::E), K(W_CHR, KC::R), K(W_CHR, KC::T), K(W_CHR, KC::Y), K(W_CHR, KC::U), K(W_CHR, KC::I), K(W_CHR, KC::O), K(W_CHR, KC::P), K(W_CHR, KC::AA)},
    std::array<K, 11>{K(W_CHR, KC::A), K(W_CHR, KC::S), K(W_CHR, KC::D), K(W_CHR, KC::F), K(W_CHR, KC::G), K(W_CHR, KC::H), K(W_CHR, KC::J), K(W_CHR, KC::K), K(W_CHR, KC::L), K(W_CHR, KC::OE), K(W_CHR, KC::AE)},
    std::array<K, 11>{K(W_15, KeyCodes::SHIFT),   K(W_05, KC::NONE), K(W_CHR, KC::Z), K(W_CHR, KC::X), K(W_CHR, KC::C), K(W_CHR, KC::V), K(W_CHR, KC::B), K(W_CHR, KC::N), K(W_CHR, KC::M), K(W_05, KC::NONE), K(W_15, KeyCodes::BACKSPACE)},
    std::array<K, 11>{K(W_20, KeyCodes::SYMBOLS), K(W_05, KC::NONE), K(0.0f, KC::NONE), K(W_CHR, ":"), K(W_CHR, "/"), K(W_20, KC::SPACE), K(W_CHR, "."), K(W_CHR, ","), K(0.0f, KC::NONE), K(W_05, KC::NONE), K(W_20, KeyCodes::ENTER)},
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KeyboardView::KeyboardView(const String& name, Ptr<View> parent, uint32_t flags) : View(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize)
{
    const FontHeight fontHeight = GetFontHeight();
    m_KeyHeight = fontHeight.descender - fontHeight.ascender + 8.0f;

    m_KeysBitmap = ptr_new<Bitmap>(KEYS_BITMAP_SIZE.x, KEYS_BITMAP_SIZE.y, ColorSpace::MONO1, Bitmap::SHARE_FRAMEBUFFER);

    uint8_t* raster = m_KeysBitmap->LockRaster();
    if (raster != nullptr)
    {
        for (int y = 0; y < KEY_BM_FRAME_BACKSPACE.Height(); ++y)
        {
            memcpy(raster, &g_BackspaceRaster[y], sizeof(g_BackspaceRaster[y]));
            raster = add_bytes_to_pointer(raster, m_KeysBitmap->GetBytesPerRow());
        }
        for (int y = 0; y < KEY_BM_FRAME_SHIFT.Height(); ++y)
        {
            memcpy(raster, &g_ShiftRaster[y], sizeof(g_ShiftRaster[y]));
            raster = add_bytes_to_pointer(raster, m_KeysBitmap->GetBytesPerRow());
        }
        for (int y = 0; y < KEY_BM_FRAME_ENTER.Height(); ++y)
        {
            memcpy(raster, &g_EnterRaster[y], sizeof(g_EnterRaster[y]));
            raster = add_bytes_to_pointer(raster, m_KeysBitmap->GetBytesPerRow());
        }
        for (int y = 0; y < KEY_BM_FRAME_SPACE.Height(); ++y)
        {
            memcpy(raster, &g_SpaceRaster[y], sizeof(g_SpaceRaster[y]));
            raster = add_bytes_to_pointer(raster, m_KeysBitmap->GetBytesPerRow());
        }
        m_KeysBitmap->UnlockRaster();
    }

    m_KeyButtons.reserve(g_KeyMap.size() * g_KeyMap[0].size());

    Point position(0.0f, 0.0f);
    for (size_t rowIndex = 0; rowIndex < g_KeyMap.size(); ++rowIndex)
    {
        const auto& row = g_KeyMap[rowIndex];

        position.x = 0.0f;
        for (const KLNode& node : row)
        {
            if (node.m_KeyCode != KeyCodes::NONE) {
                m_KeyButtons.emplace_back(position, node.m_Width, node.m_KeyCode);
            }
            position.x += node.m_Width;
        }
        position.y += 1.0f;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    Point size(11.0f * 40.0f, (m_KeyHeight + KEY_SPACING.y) * float(g_KeyMap.size()) + KEY_SPACING.y);

    *minSize = size;
    *maxSize = size;
    maxSize->x = LAYOUT_MAX_SIZE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::FrameSized(const Point& delta)
{
    const Rect bounds = GetBounds();

    const float viewWidth = bounds.Width() - KEY_SPACING.x;

    for (auto& button : m_KeyButtons)
    {
        Point position(round(button.m_RelativePos.x * viewWidth) + KEY_SPACING.x, round(button.m_RelativePos.y * (m_KeyHeight + KEY_SPACING.y)) + KEY_SPACING.y);
        button.m_Frame = Rect(position.x, position.y, position.x + floorf(viewWidth * button.m_Width - KEY_SPACING.x), position.y + m_KeyHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::Paint(const Rect& updateRect)
{
    Region clearRegion(updateRect);
    for (const auto& button : m_KeyButtons)
    {
        if (updateRect.DoIntersect(button.m_Frame)) {
            clearRegion.Exclude(button.m_Frame);
        }
    }
    for (const IRect& rect : clearRegion.m_Rects) {
        EraseRect(rect);
    }
    for (size_t i = 0; i < m_KeyButtons.size(); ++i)
    {
        const KeyButton& button = m_KeyButtons[i];

        DrawButton(button, i == m_PressedButton);
    }
    SetEraseColor(StandardColorID::DefaultBackground);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KeyboardView::OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (m_HitButton != MouseButton_e::None) {
        return View::OnTouchDown(pointID, position, event);
    }
    m_HitButton = pointID;
    m_HitPos = position;

    SetPressedButton(GetButtonIndex(position));

    MakeFocus(pointID, true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KeyboardView::OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return View::OnTouchUp(pointID, position, event);
    }

    if (m_PressedButton != INVALID_INDEX)
    {
        KeyButton& button = m_KeyButtons[m_PressedButton];

        bool resetCapsLock = false;
        if (button.m_KeyCode == KeyCodes::SHIFT)
        {
            if (m_CapsLockMode == CapsLockMode::Off) {
                m_CapsLockMode = CapsLockMode::Single;
            }
            else if (m_CapsLockMode == CapsLockMode::Single) {
                m_CapsLockMode = CapsLockMode::Locked;
            }
            else {
                m_CapsLockMode = CapsLockMode::Off;
            }
            Invalidate();
        }
        else if (button.m_KeyCode == KeyCodes::SYMBOLS)
        {
            m_SymbolsActive = !m_SymbolsActive;
            Invalidate();
        }
        else if (button.m_KeyCode != KeyCodes::SPACE)
        {
            resetCapsLock = true;
        }
        SignalKeyPressed(button.m_KeyCode, GetKeyText(button), this);
        if (resetCapsLock && m_CapsLockMode == CapsLockMode::Single) {
            m_CapsLockMode = CapsLockMode::Off;
            Invalidate();
        }
        SetPressedButton(INVALID_INDEX);
    }

    m_HitButton = MouseButton_e::None;
    MakeFocus(pointID, false);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KeyboardView::OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return View::OnTouchMove(pointID, position, event);
    }
    if ((position - m_HitPos).LengthSqr() > 20.0f * 20.0f) {
        SetPressedButton(INVALID_INDEX);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String KeyboardView::GetKeyText(const KeyButton& button) const
{
    if (button.m_KeyCode > KeyCodes::LAST_SPECIAL)
    {
        String text;

        if (button.m_KeyCode == KeyCodes::TAB) {
            text = "\t";
        } else if (button.m_KeyCode == KeyCodes::ENTER) {
            text = "\n";
        } else if (m_CapsLockMode != CapsLockMode::Off) {
            text.append(uint32_t(button.m_KeyCode));
        } else {
            text.append(tolower(uint32_t(button.m_KeyCode)));
        }
        return text;
    }
    return String::zero;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::DrawButton(const KeyButton& button, bool pressed)
{
    Color textColor(NamedColors::black);

    if (pressed)
    {
        SetEraseColor(NamedColors::mediumblue);
    }
    else if (button.m_KeyCode == KeyCodes::SHIFT)
    {
        SetEraseColor(m_CapsLockMode == CapsLockMode::Locked ? NamedColors::darkblue : NamedColors::dimgray);
        if (m_CapsLockMode == CapsLockMode::Single) {
            textColor = NamedColors::darkblue;
        }
    }
    else
    {
        SetEraseColor(NamedColors::dimgray);
    }

    DrawFrame(button.m_Frame, FRAME_RAISED);
    SetFgColor(textColor);

    Rect bitmapFrame(0.0f, 0.0f, 0.0f, 0.0f);

    switch (KeyCodes(button.m_KeyCode))
    {
        case KeyCodes::ENTER:           bitmapFrame = KEY_BM_FRAME_ENTER; break;
        case KeyCodes::BACKSPACE:       bitmapFrame = KEY_BM_FRAME_BACKSPACE; break;
        case KeyCodes::SPACE:           bitmapFrame = KEY_BM_FRAME_SPACE; break;
        case KeyCodes::DELETE:          break;
        case KeyCodes::CURSOR_LEFT:     break;
        case KeyCodes::CURSOR_RIGHT:    break;
        case KeyCodes::CURSOR_UP:       break;
        case KeyCodes::CURSOR_DOWN:     break;
        case KeyCodes::HOME:            break;
        case KeyCodes::END:             break;
        case KeyCodes::TAB:             break;
        case KeyCodes::SHIFT:           bitmapFrame = KEY_BM_FRAME_SHIFT; break;
        case KeyCodes::CTRL:            break;
        case KeyCodes::ALT:             break;
        case KeyCodes::SYMBOLS:         break;
        default: break;

    }
    if (bitmapFrame.Width() > 0.0f)
    {
        SetDrawingMode(DrawingMode::Overlay);
        DrawBitmap(m_KeysBitmap, bitmapFrame, button.m_Frame.TopLeft() + Point(round((button.m_Frame.Width() - bitmapFrame.Width()) * 0.5f), round((button.m_Frame.Height() - bitmapFrame.Height()) * 0.5f)));
        SetDrawingMode(DrawingMode::Copy);
    }
    else
    {
        SetBgColor(GetEraseColor());

        String label;
        if (button.m_KeyCode != KeyCodes::SYMBOLS)
        {
            label = GetKeyText(button);
        }
        else {
            label = m_SymbolsActive ? "ABC" : "!#1";
        }
        const float labelWidth = GetStringWidth(label);

        DrawString(label, button.m_Frame.TopLeft() + Point(round((button.m_Frame.Width() - labelWidth) * 0.5f), 4.0f));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::SetPressedButton(size_t index)
{
    if (index != m_PressedButton)
    {
        size_t prevIndex = m_PressedButton;
        m_PressedButton = index;

        if (prevIndex != INVALID_INDEX)
        {
            Invalidate(m_KeyButtons[prevIndex].m_Frame);
        }
        if (m_PressedButton != INVALID_INDEX)
        {
            Invalidate(m_KeyButtons[m_PressedButton].m_Frame);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KeyboardView::GetButtonIndex(const Point& position) const
{
    for (size_t i = 0; i < m_KeyButtons.size(); ++i)
    {
        if (m_KeyButtons[i].m_Frame.DoIntersect(position)) {
            return i;
        }
    }
    return INVALID_INDEX;
}

} // namespace os
