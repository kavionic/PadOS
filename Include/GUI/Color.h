// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 02.04.2018 17:54:36
 
#pragma once

#include <GUI/NamedColors.h>
#include <GUI/GUIDefines.h>


namespace os
{

struct Color
{
    static constexpr uint8_t Expand5to8(uint8_t src) { return uint8_t((src << 3) | (src >> 2)); }
    static constexpr uint8_t Expand6to8(uint8_t src) { return uint8_t((src << 2) | (src >> 4)); }

    static constexpr Color FromCMAP8(uint8_t colorIndex)
    {
        constexpr uint8_t normalColors[] = { 0, 51, 102, 152, 203, 255 };
        constexpr uint8_t fillerColors[] = { 228, 178, 128 };

        if (colorIndex < 32) {
            uint8_t c = uint8_t(colorIndex * 8U);
            return Color(c, c, c, 255);
        } else if (colorIndex < 33) {
            return Color(255, 255, 255, 255);
        } else if (colorIndex < 36) {
            return Color(fillerColors[colorIndex - 33], 0, 0, 255);
        } else if (colorIndex < 39) {
            return Color(0, fillerColors[colorIndex - 36], 0, 255);
        } else if (colorIndex < 41) {
            return Color(0, 0, fillerColors[colorIndex - 39], 255);
        } else if (colorIndex < 255) {
            const int index = colorIndex - 40;
            return Color(normalColors[(index / 36) % 6], normalColors[(index / 6) % 6], normalColors[index % 6], 255);
        } else {
            return Color(255, 4, 255, 0);
        }
    }

    static constexpr Color FromRGB15(uint16_t color) { return Color(uint8_t(((color >> 10) & 0x1f) * 255 / 31), uint8_t(((color >> 5) & 0x1f) * 255 / 31), uint8_t((color & 0x1f) * 255 / 31), 255); }
    static constexpr Color FromRGB16(uint16_t color) { return Color(Expand5to8(uint8_t(color >> 11) & 0x1f), Expand6to8(uint8_t(color >> 5) & 0x3f), Expand5to8(uint8_t(color) & 0x1f), 255); }
    static constexpr Color FromRGB32A(uint32_t color) { return Color(color); }
    static constexpr Color FromRGB32A(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) { return Color(red, green, blue, alpha); }
    static constexpr Color FromRGB32AFloat(float red, float green, float blue, float alpha = 1.0f) { return Color(
        uint8_t(std::clamp(int(red * 255.0f), 0, 255)),
        uint8_t(std::clamp(int(green * 255.0f), 0, 255)),
        uint8_t(std::clamp(int(blue * 255.0f), 0, 255)),
        uint8_t(std::clamp(int(alpha * 255.0f), 0, 255)));
    }
    static           Color FromColorID(NamedColors colorID);
    static           Color FromColorName(const char* name)   { return FromColorID(NamedColors(String::hash_string_literal_nocase(name))); }
    static           Color FromColorName(const String& name) { return FromColorName(name.c_str()); }

    constexpr Color() : m_Color(0) {}
    constexpr Color(const Color& color) : m_Color(color.m_Color) {}
    explicit constexpr Color(uint32_t color32) : m_Color(color32) {}
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : m_Color((r << 16) | (g << 8) | b | (a << 24)) {}
    
    Color(NamedColors colorID);
    Color(const String& name);

    Color& operator=(const Color&) = default;
    Color& operator*=(float rhs);
    Color operator*(float rhs) const;

    void Set16(uint16_t color)                                     { SetRGBA(uint8_t(((color >> 11) & 0x1f) * 255 / 31), uint8_t(((color >> 5) & 0x3f) * 255 / 63), uint8_t((color & 0x1f) * 255 / 31), 255); }
    void Set32(uint32_t color)                                     { m_Color = color; }
    void SetRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { m_Color = (r << 16) | (g << 8) | b | (a << 24); }
    
    uint8_t GetRed() const { return uint8_t((m_Color >> 16) & 0xff); }
    uint8_t GetGreen() const { return uint8_t((m_Color >> 8) & 0xff); }
    uint8_t GetBlue() const { return uint8_t(m_Color & 0xff); }
    uint8_t GetAlpha() const { return uint8_t((m_Color >> 24) & 0xff); }

    float GetRedFloat() const { return float(GetRed()) / 255.0f; }
    float GetGreenFloat() const { return float(GetGreen()) / 255.0f; }
    float GetBlueFloat() const { return float(GetBlue()) / 255.0f; }
    float GetAlphaFloat() const { return float(GetAlpha()) / 255.0f; }

    Color GetNorimalized() const { return GetNorimalized(GetAlpha()); }
    Color GetNorimalized(uint8_t alpha) const { return Color(uint8_t((uint32_t(GetRed()) * alpha + 127) / 255), uint8_t((uint32_t(GetGreen()) * alpha + 127) / 255), uint8_t((uint32_t(GetBlue()) * alpha + 127) / 255)); }

    uint16_t GetColor15() const { return uint16_t(((GetRed() & 0xf8) << 7) | ((GetGreen() & 0xf8) << 2) | ((GetBlue() & 0xf8) >> 3)); }
    uint16_t GetColor16() const { return uint16_t(((GetRed() & 0xf8) << 8) | ((GetGreen() & 0xfc) << 3) | ((GetBlue() & 0xf8) >> 3)); }
    uint32_t GetColor32() const { return m_Color; }

    Color GetInverted() const { return Color(uint8_t(255 - GetRed()), uint8_t(255 - GetGreen()), uint8_t(255 - GetBlue()), GetAlpha()); }
    void  Invert() { SetRGBA(uint8_t(255 - GetRed()), uint8_t(255 - GetGreen()), uint8_t(255 - GetBlue()), GetAlpha()); }

    uint32_t m_Color;
};

class DynamicColor : public Color
{
public:
    constexpr DynamicColor() {}
    constexpr DynamicColor(const DynamicColor& color) : Color(color.m_Color) {}
    explicit constexpr DynamicColor(const Color& color) : Color(color.m_Color) {}
    explicit constexpr DynamicColor(uint32_t color32) : Color(color32) {}
    constexpr DynamicColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : Color(r, g, b, a) {}

    DynamicColor(NamedColors colorID) : Color(colorID) {}
    DynamicColor(const String& name) : Color(name) {}

    DynamicColor(StandardColorID colorID);

private:
//    StandardColorID m_StandardColorID = StandardColorID::None;
};

} // namespace

namespace unit_test
{
void TestNamedColors();
}
