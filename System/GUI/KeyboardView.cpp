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

#include <sys/fcntl.h>
#include <pugixml/src/pugixml.hpp>

#include <GUI/Bitmap.h>
#include <GUI/ViewFactory.h>
#include <GUI/KeyboardView.h>
#include <Utils/Utils.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFilesystem.h>

using namespace pugi;

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

static constexpr Point KEY_SPACING(5.0f, 5.0f);

NoPtr<KeyboardViewStyle> KeyboardView::s_DefaultStyle;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KeyboardView::KeyboardView(const String& name, Ptr<View> parent, uint32_t flags) : View(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize)
{
    const FontHeight fontHeight = m_Style->LargeFont->GetHeight();
    m_KeyHeight = fontHeight.descender - fontHeight.ascender + 11.0f;

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

    int dir = FileIO::Open("/sdcard/Rainbow3D/System/Keyboards", O_RDONLY);
    if (dir >= 0)
    {
        kernel::dir_entry entry;
        for (int i = 0; FileIO::ReadDirectory(dir, &entry, sizeof(entry)) == 1; ++i)
        {
            String name(entry.d_name);
            if (name.compare_nocase("numerical.xml") == 0) {
                continue;
            }
            if (name.size() > 4 && strcmp(&name[name.size() - 4], ".xml") == 0)
            {
                name.resize(name.size() - 4);
                m_Keyboards.push_back(name);
            }
        }
    }
    if (HasFlags(KeyboardViewFlags::Numerical))
    {
        LoadKeyboard("/sdcard/Rainbow3D/System/Keyboards/Numerical.xml");
    }
    else
    {
        if (!m_Keyboards.empty())
        {
            if (m_SelectedKeyboard >= m_Keyboards.size()) m_SelectedKeyboard = 0;
            LoadKeyboard(String("/sdcard/Rainbow3D/System/Keyboards/") + m_Keyboards[m_SelectedKeyboard] + String(".xml"));
        }
    }
    m_RepeatTimer.SignalTrigged.Connect(this, &KeyboardView::SlotRepeatTimer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KeyboardView::LoadKeyboard(const String& path)
{
    int file = FileIO::Open(path.c_str(), O_RDONLY);
    if (file == -1) {
        return false;
    }
    struct stat stats;

    if (FileIO::ReadStats(file, &stats) == -1)
    {
        FileIO::Close(file);
        return false;
    }
    std::vector<char> buffer;
    buffer.resize(stats.st_size + 1);

    ssize_t bytesRead = FileIO::Read(file, buffer.data(), stats.st_size);
    FileIO::Close(file);

    if (bytesRead != stats.st_size) {
        return false;
    }
    buffer[stats.st_size] = '\0';

    XMLDocument doc;

    if (!doc.Parse(std::move(buffer))) {
        return false;
    }

    xml_node rootNode = doc.GetDocumentElement();

    if (!rootNode) {
        return false;
    }

    m_CurrentLayout = &m_DefaultLayout;
    m_SymbolsActive = false;

    m_DefaultLayout.Clear();
    m_SymbolLayouts.clear();
    m_SymbolLayouts.reserve(2);

    for (xml_node layoutNode = rootNode.first_child(); layoutNode; layoutNode = layoutNode.next_sibling())
    {
        if (strcmp(layoutNode.name(), "Layout") != 0) {
            continue;
        }
        String typeName = xml_object_parser::parse_attribute(layoutNode, "type", String::zero);
        KeyboardLayout* layout = nullptr;

        if (typeName.compare_nocase("normal") == 0 || typeName.compare_nocase("numerical") == 0) {
            layout = &m_DefaultLayout;
        } else if (typeName.compare_nocase("symbols") == 0) {
            layout = &m_SymbolLayouts.emplace_back();
        }

        float totalHeight = 0.0f;
        for (xml_node rowNode = layoutNode.first_child(); rowNode; rowNode = rowNode.next_sibling())
        {
            if (strcmp(rowNode.name(), "KeyRow") == 0) {
                totalHeight += 1.0f;
            }
        }
        Point position(0.0f, 0.0f);
        for (xml_node rowNode = layoutNode.first_child(); rowNode; rowNode = rowNode.next_sibling())
        {
            if (strcmp(rowNode.name(), "KeyRow") != 0) {
                continue;
            }
            float totalWidth = 0.0f;
            for (xml_node keyNode = rowNode.first_child(); keyNode; keyNode = keyNode.next_sibling())
            {
                if (strcmp(keyNode.name(), "Key") == 0) {
                    totalWidth += xml_object_parser::parse_attribute(keyNode, "width", 1.0f);
                }
            }
            position.x = 0.0f;
            for (xml_node keyNode = rowNode.first_child(); keyNode; keyNode = keyNode.next_sibling())
            {
                if (strcmp(keyNode.name(), "Key") != 0) {
                    continue;
                }

                float   width = xml_object_parser::parse_attribute(keyNode, "width", 1.0f) / totalWidth;
                KeyCodes normalKeyCode = xml_object_parser::parse_attribute(keyNode, "normal", KeyCodes::NONE);
                KeyCodes lowerKeyCode = xml_object_parser::parse_attribute(keyNode, "lower", KeyCodes(std::towlower(int(normalKeyCode))));
                KeyCodes extraKeyCode = xml_object_parser::parse_attribute(keyNode, "extra", KeyCodes::NONE);
                KeyCodes symbolKeyCode = xml_object_parser::parse_attribute(keyNode, "symbol", KeyCodes::NONE);

                if (normalKeyCode != KeyCodes::NONE) {
                    layout->m_KeyButtons.emplace_back(position, width, 1.0f / totalHeight, normalKeyCode, lowerKeyCode, extraKeyCode, symbolKeyCode);
                }
                position.x += width;
            }
            position.y += 1.0f / totalHeight;
            layout->m_KeyRows++;
        }
    }
    m_CurrentLayout->Layout(GetBounds(), KEY_SPACING, m_KeyHeight + KEY_SPACING.y);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::OnFlagsChanged(uint32_t changedFlags)
{
    if (changedFlags & KeyboardViewFlags::Numerical)
    {
        m_SymbolsActive = false;
        m_SymbolPage = 0;
        if (HasFlags(KeyboardViewFlags::Numerical))
        {
            LoadKeyboard("/sdcard/Rainbow3D/System/Keyboards/Numerical.xml");
        }
        else
        {
            if (!m_Keyboards.empty())
            {
                if (m_SelectedKeyboard >= m_Keyboards.size()) m_SelectedKeyboard = 0;
                LoadKeyboard(String("/sdcard/Rainbow3D/System/Keyboards/") + m_Keyboards[m_SelectedKeyboard] + String(".xml"));
            }
        }
        Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    Point size(11.0f * 40.0f, (m_KeyHeight + KEY_SPACING.y) * std::max(5.0f, float(m_CurrentLayout->m_KeyRows)) + KEY_SPACING.y);

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
    m_CurrentLayout->Layout(bounds, KEY_SPACING, m_KeyHeight + KEY_SPACING.y);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::Paint(const Rect& updateRect)
{
    Region clearRegion(updateRect);
    for (const auto& button : m_CurrentLayout->m_KeyButtons)
    {
        if (updateRect.DoIntersect(button.m_Frame)) {
            clearRegion.Exclude(button.m_Frame);
        }
    }
    for (const IRect& rect : clearRegion.m_Rects) {
        EraseRect(rect);
    }
    for (size_t i = 0; i < m_CurrentLayout->m_KeyButtons.size(); ++i)
    {
        const KeyButton& button = m_CurrentLayout->m_KeyButtons[i];
        if (button.m_Frame.DoIntersect(updateRect)) {
            DrawButton(button, i == m_PressedButton);
        }
    }
    SetEraseColor(m_Style->BackgroundColor);
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

    if (m_PressedButton != INVALID_INDEX)
    {
        KeyButton& button = m_CurrentLayout->m_KeyButtons[m_PressedButton];

        if (button.m_NormalKeyCode == KeyCodes::BACKSPACE)
        {
            SignalKeyPressed(button.m_NormalKeyCode, GetKeyText((m_CapsLockMode == CapsLockMode::Off) ? button.m_LowerKeyCode : button.m_NormalKeyCode), this);
            m_RepeatTimer.Set(KEYREPEAT_DELAY, true);
            m_RepeatTimer.Start(true);
        }
    }
    MakeFocus(pointID, true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KeyboardView::OnLongPress(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return false;
    }
    bool cancelPress = false;
    if (m_PressedButton != INVALID_INDEX)
    {
        KeyButton& button = m_CurrentLayout->m_KeyButtons[m_PressedButton];

        if (button.m_NormalKeyCode == KeyCodes::SHIFT)
        {
            if (!m_SymbolsActive)
            {
                if (m_CapsLockMode == CapsLockMode::Off) {
                    m_CapsLockMode = CapsLockMode::Locked;
                } else {
                    m_CapsLockMode = CapsLockMode::Off;
                }
                cancelPress = true;
            }
        }
        else if (button.m_NormalKeyCode == KeyCodes::ENTER && !HasFlags(KeyboardViewFlags::Numerical))
        {
            if (!m_Keyboards.empty())
            {
                m_SelectedKeyboard++;
                if (m_SelectedKeyboard >= m_Keyboards.size()) m_SelectedKeyboard = 0;
                LoadKeyboard(String("/sdcard/Rainbow3D/System/Keyboards/") + m_Keyboards[m_SelectedKeyboard] + String(".xml"));
                cancelPress = true;
            }
        }
        else if (button.m_ExtraKeyCode != KeyCodes::NONE)
        {
            SignalKeyPressed(button.m_ExtraKeyCode, GetKeyText(button.m_ExtraKeyCode), this);
            cancelPress = true;
        }
    }
    if (cancelPress)
    {
        SetPressedButton(INVALID_INDEX);
        m_HitButton = MouseButton_e::None;
        MakeFocus(pointID, false);
        Invalidate();
    }
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
        KeyButton& button = m_CurrentLayout->m_KeyButtons[m_PressedButton];
        SetPressedButton(INVALID_INDEX);

        bool resetCapsLock = false;
        if (button.m_NormalKeyCode == KeyCodes::SHIFT)
        {
            if (m_SymbolsActive)
            {
                if (!m_SymbolLayouts.empty())
                {
                    m_SymbolPage++;
                    if (m_SymbolPage >= m_SymbolLayouts.size()) {
                        m_SymbolPage = 0;
                    }
                    SetLayout(&m_SymbolLayouts[m_SymbolPage]);
                }
            }
            else
            {
                if (m_CapsLockMode == CapsLockMode::Off) {
                    m_CapsLockMode = CapsLockMode::Single;
                } else if (m_CapsLockMode == CapsLockMode::Single) {
                    m_CapsLockMode = CapsLockMode::Locked;
                } else {
                    m_CapsLockMode = CapsLockMode::Off;
                }
            }
            Invalidate();
        }
        else if (button.m_NormalKeyCode == KeyCodes::SYMBOLS)
        {
            m_SymbolsActive = !m_SymbolsActive;
            if (m_SymbolsActive)
            {
                if (!m_SymbolLayouts.empty()) {
                    SetLayout(&m_SymbolLayouts[m_SymbolPage]);
                }
                else {
                    m_SymbolsActive = false;
                }
            }
            else
            {
                SetLayout(&m_DefaultLayout);
            }
        }
        else if (button.m_NormalKeyCode != KeyCodes::BACKSPACE && button.m_NormalKeyCode != KeyCodes::SPACE && button.m_NormalKeyCode != KeyCodes::ENTER)
        {
            resetCapsLock = true;
        }
        if (!m_RepeatTimer.IsRunning()) {
            SignalKeyPressed(button.m_NormalKeyCode, GetKeyText((m_CapsLockMode == CapsLockMode::Off) ? button.m_LowerKeyCode : button.m_NormalKeyCode), this);
        }
        if (resetCapsLock && m_CapsLockMode == CapsLockMode::Single)
        {
            m_CapsLockMode = CapsLockMode::Off;
            Invalidate();
        }
    }
    m_RepeatTimer.Stop();
    m_IsDraggingCursor = false;
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

    if (m_IsDraggingCursor)
    {
        int cursorPos = int(round((position.x - m_HitPos.x) / 20.0f));
        while (cursorPos < m_PrevCursorPos)
        {
            m_PrevCursorPos--;
            SignalKeyPressed(KeyCodes::CURSOR_LEFT, String::zero, this);
        }
        while (cursorPos > m_PrevCursorPos)
        {
            m_PrevCursorPos++;
            SignalKeyPressed(KeyCodes::CURSOR_RIGHT, String::zero, this);
        }
    }
    else if (m_PressedButton != INVALID_INDEX)
    {
        if ((position - m_HitPos).LengthSqr() > square(BEGIN_DRAG_OFFSET))
        {
            KeyButton& button = m_CurrentLayout->m_KeyButtons[m_PressedButton];
            if (button.m_NormalKeyCode == KeyCodes::SPACE)
            {
                m_IsDraggingCursor = true;
                m_PrevCursorPos = int(round((position.x - m_HitPos.x) / 20.0f));
            }
            SetPressedButton(INVALID_INDEX);
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::SetLayout(KeyboardLayout* layout)
{
    m_CurrentLayout = layout;

    m_CurrentLayout->Layout(GetBounds(), KEY_SPACING, m_KeyHeight + KEY_SPACING.y);
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String KeyboardView::GetKeyText(KeyCodes keyCode) const
{
    if (keyCode > KeyCodes::LAST_SPECIAL)
    {
        String text;

        if (keyCode == KeyCodes::TAB) {
            text = "\t";
        } else if (keyCode == KeyCodes::ENTER) {
            text = "\n";
        } else {
            text.append(uint32_t(keyCode));
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
    Color textColor = m_Style->NormalTextColor;

    if (pressed)
    {
        SetEraseColor(m_Style->PressedButtonColor);
    }
    else if (!m_SymbolsActive && button.m_NormalKeyCode == KeyCodes::SHIFT)
    {
        switch (m_CapsLockMode)
        {
            case os::CapsLockMode::Off:     SetEraseColor(m_Style->NormalButtonColor);      break;
            case os::CapsLockMode::Single:  SetEraseColor(m_Style->SelectedButtonColor); textColor = m_Style->SelectedTextColor;   break;
            case os::CapsLockMode::Locked:  SetEraseColor(m_Style->LockedButtonColor);   textColor = m_Style->LockedTextColor;     break;
        }
    }
    else
    {
        SetEraseColor(m_Style->NormalButtonColor);
    }

    DrawFrame(button.m_Frame, FRAME_RAISED);
    SetFgColor(textColor);

    Rect bitmapFrame(0.0f, 0.0f, 0.0f, 0.0f);

    switch (KeyCodes(button.m_NormalKeyCode))
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
        case KeyCodes::SHIFT:
            if (!m_SymbolsActive) {
                bitmapFrame = KEY_BM_FRAME_SHIFT;
            }
            break;
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
        if (button.m_NormalKeyCode == KeyCodes::SYMBOLS) {
            label = m_SymbolsActive ? "ABC" : "!#1";
        }
        else if (button.m_NormalKeyCode == KeyCodes::SHIFT && m_SymbolsActive) {
            label = String::format_string("%ld/%ld", m_SymbolPage + 1, m_SymbolLayouts.size());
        } else {
            label = GetKeyText((m_CapsLockMode == CapsLockMode::Off) ? button.m_LowerKeyCode : button.m_NormalKeyCode);
        }
        String smallLabel = GetKeyText(button.m_ExtraKeyCode);

        SetFont(m_Style->LargeFont);
        const FontHeight fontHeight = m_Style->LargeFont->GetHeight();
        const float labelWidth = GetStringWidth(label);
        const float labelHeight = fontHeight.descender - fontHeight.ascender;

        Point labelPos = button.m_Frame.TopLeft() + Point(round((button.m_Frame.Width() - labelWidth) * 0.5f), ceilf((button.m_Frame.Height() - labelHeight) * 0.5f));

        if (!smallLabel.empty()) {
            labelPos.y += 2.0f;
        }

        DrawString(label, labelPos);

        if (!smallLabel.empty())
        {
            const float smallLabelWidth = GetStringWidth(smallLabel);
            SetFont(m_Style->SmallFont);
            SetFgColor(m_Style->ExtraTextColor);
            DrawString(smallLabel, button.m_Frame.TopRight() + Point(-smallLabelWidth - 6.0f, 2.0f));
        }
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
            Invalidate(m_CurrentLayout->m_KeyButtons[prevIndex].m_Frame);
        }
        if (m_PressedButton != INVALID_INDEX)
        {
            Invalidate(m_CurrentLayout->m_KeyButtons[m_PressedButton].m_Frame);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KeyboardView::GetButtonIndex(const Point& position) const
{
    for (size_t i = 0; i < m_CurrentLayout->m_KeyButtons.size(); ++i)
    {
        if (m_CurrentLayout->m_KeyButtons[i].m_Frame.DoIntersect(position)) {
            return i;
        }
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KeyboardView::SlotRepeatTimer()
{
    if (m_PressedButton != INVALID_INDEX)
    {
        KeyButton& button = m_CurrentLayout->m_KeyButtons[m_PressedButton];

        if (button.m_NormalKeyCode == KeyCodes::BACKSPACE)
        {
            SignalKeyPressed(KeyCodes::BACKSPACE, String::zero, this);

            m_RepeatTimer.Set(KEYREPEAT_REPEAT, true);
            m_RepeatTimer.Start(true);
        }
    }
}

} // namespace os
