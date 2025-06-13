// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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
#include <Math/Misc.h>


namespace os
{

struct Color
{
    static constexpr uint8_t Expand5to8(uint8_t src) PALWAYS_INLINE { return uint8_t((src << 3) | (src >> 2)); }
    static constexpr uint8_t Expand6to8(uint8_t src) PALWAYS_INLINE { return uint8_t((src << 2) | (src >> 4)); }

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

    static constexpr Color FromRGB15(uint16_t color) PALWAYS_INLINE
    {
        return Color(uint8_t(((color >> 10) & 0x1f) * 255 / 31),
                     uint8_t(((color >> 5) & 0x1f)  * 255 / 31),
                     uint8_t( (color & 0x1f)        * 255 / 31));
    }
    static constexpr Color FromRGB16(uint16_t color) PALWAYS_INLINE
    {
        return Color(Expand5to8(uint8_t(color >> 11) & 0x1f),
                     Expand6to8(uint8_t(color >> 5)  & 0x3f),
                     Expand5to8(uint8_t(color)       & 0x1f));
    }
    static constexpr Color FromRGB32(uint32_t color) PALWAYS_INLINE  { return Color(color | 0xff000000); }
    static constexpr Color FromRGB32A(uint32_t color) PALWAYS_INLINE  { return Color(color); }
    static constexpr Color FromRGB32A(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) PALWAYS_INLINE  { return Color(red, green, blue, alpha); }
    static constexpr Color FromRGB32AFloat(float red, float green, float blue, float alpha = 1.0f) PALWAYS_INLINE
    {
        return Color(uint8_t(red   * 255.0f),
                     uint8_t(green * 255.0f),
                     uint8_t(blue  * 255.0f),
                     uint8_t(alpha * 255.0f));
    }
    static constexpr Color FromRGB32AFloatClamped(float red, float green, float blue, float alpha = 1.0f) PALWAYS_INLINE
    {
        return Color(uint8_t(std::clamp(int32_t(red   * 255.0f), 0l, 255l)),
                     uint8_t(std::clamp(int32_t(green * 255.0f), 0l, 255l)),
                     uint8_t(std::clamp(int32_t(blue  * 255.0f), 0l, 255l)),
                     uint8_t(std::clamp(int32_t(alpha * 255.0f), 0l, 255l)));
    }

    static constexpr Color Blend(Color srcColor, Color dstColor) PALWAYS_INLINE
    {
        if (srcColor.GetAlpha() == 255)
        {
            return srcColor;
        }
        else if (srcColor.GetAlpha() == 0)
        {
            return dstColor;
        }
        else
        {
            const float alpha    = srcColor.GetAlphaFloat();
            const float alphaInv = 1.0f - alpha;
            return Color::FromRGB32A(uint8_t(float(srcColor.GetRed())   * alpha + float(dstColor.GetRed())   * alphaInv),
                                     uint8_t(float(srcColor.GetGreen()) * alpha + float(dstColor.GetGreen()) * alphaInv),
                                     uint8_t(float(srcColor.GetBlue())  * alpha + float(dstColor.GetBlue())  * alphaInv)
            );
        }
    }

    static constexpr uint16_t Blend15(Color srcColor, Color dstColor) PALWAYS_INLINE
    {
        if (srcColor.GetAlpha() == 255)
        {
            return srcColor.GetColor15();
        }
        else if (srcColor.GetAlpha() == 0)
        {
            return dstColor.GetColor15();
        }
        else
        {
            const float alpha    = srcColor.GetAlphaFloat();
            const float alphaInv = 1.0f - alpha;
            return Color::FromRGB32A(uint8_t(float(srcColor.GetRed())   * alpha + float(dstColor.GetRed())   * alphaInv),
                                     uint8_t(float(srcColor.GetGreen()) * alpha + float(dstColor.GetGreen()) * alphaInv),
                                     uint8_t(float(srcColor.GetBlue())  * alpha + float(dstColor.GetBlue())  * alphaInv)).GetColor15();
        }
    }

    static constexpr uint16_t Blend16(Color srcColor, Color dstColor) PALWAYS_INLINE
    {
        if (srcColor.GetAlpha() == 255)
        {
            return srcColor.GetColor16();
        }
        else if (srcColor.GetAlpha() == 0)
        {
            return dstColor.GetColor16();
        }
        else
        {
            const float alpha    = srcColor.GetAlphaFloat();
            const float alphaInv = 1.0f - alpha;
            return Color::FromRGB32A(uint8_t(float(srcColor.GetRed())   * alpha + float(dstColor.GetRed())   * alphaInv),
                                     uint8_t(float(srcColor.GetGreen()) * alpha + float(dstColor.GetGreen()) * alphaInv),
                                     uint8_t(float(srcColor.GetBlue())  * alpha + float(dstColor.GetBlue())  * alphaInv)).GetColor16();
        }
    }

    static           Color FromColorID(NamedColors colorID);
    static           Color FromColorName(const char* name)   { return FromColorID(NamedColors(String::hash_string_literal_nocase(name))); }
    static           Color FromColorName(const String& name) { return FromColorName(name.c_str()); }

    constexpr inline Color() PALWAYS_INLINE : m_Color(0) {}
    constexpr inline Color(const Color& color) PALWAYS_INLINE  : m_Color(color.m_Color) {}
    explicit constexpr inline Color(uint32_t color32) PALWAYS_INLINE : m_Color(color32) {}
    constexpr inline Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) PALWAYS_INLINE : m_Color((r << 16) | (g << 8) | b | (a << 24)) {}
    
    Color(NamedColors colorID);
    Color(const String& name);

    int operator<=>(const Color& rhs) const = default;

    Color& operator=(const Color&) = default;
    Color& operator*=(float rhs);
    constexpr Color operator*(float rhs) const
    {
        float components[] = { GetRedFloat() * rhs, GetGreenFloat() * rhs, GetBlueFloat() * rhs };
        float brightest = 0.0f;
        for (int i = 0; i < 3; ++i)
        {
            if (components[i] > brightest) {
                brightest = components[i];
            }
        }
        if (brightest > 1.0f)
        {
            const float scale = 1.0f / brightest;
            for (int i = 0; i < 3; ++i)
            {
                components[i] *= scale;
            }
        }
        return FromRGB32AFloat(components[0], components[1], components[2]);
    }

    void Set16(uint16_t color) PALWAYS_INLINE
    {
        SetRGBA(uint8_t(((color >> 11) & 0x1f) * 255 / 31),
                uint8_t(((color >> 5) & 0x3f) * 255 / 63),
                uint8_t((color & 0x1f) * 255 / 31));
    }
    void Set32(uint32_t color) PALWAYS_INLINE                                     { m_Color = color; }
    void SetRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) PALWAYS_INLINE { m_Color = (r << 16) | (g << 8) | b | (a << 24); }
    
    constexpr uint8_t GetRed() const PALWAYS_INLINE   { return uint8_t(m_Color >> 16); }
    constexpr uint8_t GetGreen() const PALWAYS_INLINE { return uint8_t(m_Color >> 8); }
    constexpr uint8_t GetBlue() const PALWAYS_INLINE  { return uint8_t(m_Color); }
    constexpr uint8_t GetAlpha() const PALWAYS_INLINE { return uint8_t(m_Color >> 24); }

    constexpr float GetRedFloat() const PALWAYS_INLINE   { return float(GetRed())   * (1.0f / 255.0f); }
    constexpr float GetGreenFloat() const PALWAYS_INLINE { return float(GetGreen()) * (1.0f / 255.0f); }
    constexpr float GetBlueFloat() const PALWAYS_INLINE  { return float(GetBlue())  * (1.0f / 255.0f); }
    constexpr float GetAlphaFloat() const PALWAYS_INLINE { return float(GetAlpha()) * (1.0f / 255.0f); }

    constexpr Color GetNorimalized() const PALWAYS_INLINE { return GetNorimalized(GetAlpha()); }
    constexpr Color GetNorimalized(uint8_t alpha) const PALWAYS_INLINE { return Color(uint8_t((uint32_t(GetRed()) * alpha + 127) / 255), uint8_t((uint32_t(GetGreen()) * alpha + 127) / 255), uint8_t((uint32_t(GetBlue()) * alpha + 127) / 255)); }

    constexpr uint16_t GetColor15() const PALWAYS_INLINE { return uint16_t(((GetRed() & 0xf8) << 7) | ((GetGreen() & 0xf8) << 2) | ((GetBlue() & 0xf8) >> 3)); }
    constexpr uint16_t GetColor16() const PALWAYS_INLINE { return uint16_t(((GetRed() & 0xf8) << 8) | ((GetGreen() & 0xfc) << 3) | ((GetBlue() & 0xf8) >> 3)); }
    constexpr uint32_t GetColor32() const PALWAYS_INLINE { return m_Color; }

    constexpr Color GetInverted() const PALWAYS_INLINE { return Color(uint8_t(255 - GetRed()), uint8_t(255 - GetGreen()), uint8_t(255 - GetBlue()), GetAlpha()); }
    void  Invert() PALWAYS_INLINE { SetRGBA(uint8_t(255 - GetRed()), uint8_t(255 - GetGreen()), uint8_t(255 - GetBlue()), GetAlpha()); }

    ///////////////////////////////////////////////////////////////////////////////
    /// Calculate the perceptual "red-mean" distance between two colors.
    /// 
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    constexpr float GetColorDistance(const Color& other) const
    {
        const float r1 = GetRedFloat();
        const float r2 = other.GetRedFloat();

        const float deltaR = r1 - r2;
        const float deltaG = GetGreenFloat() - other.GetGreenFloat();
        const float deltaB = GetBlueFloat()  - other.GetBlueFloat();

        const float redMean = (r1 + r2) * 0.5f;

        // Calculate scale weights.
        const float weightR = 2.0f + redMean;   // ≈ 2 … 3
        const float weightG = 4.0f;             // constant
        const float weightB = 3.0f - redMean;   // ≈ 2 … 3

        return weightR * square(deltaR) + weightG * square(deltaG) + weightB * square(deltaB);
    }

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
};

} // namespace

namespace unit_test
{
void TestNamedColors();
}
