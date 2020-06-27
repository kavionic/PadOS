// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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


namespace os
{

struct Color
{
    Color() : m_Color(0) {}
    Color(const Color& color) { m_Color = color.m_Color; }
    explicit Color(uint32_t color32) { m_Color = color32; }
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { m_Color = (r << 16) | (g << 8) | b | (a << 24); }

    void Set16(uint16_t color)                                     { SetRGBA(((color >> 11) & 0x1f) * 255 / 31, ((color >> 6) & 0x3f) * 255 / 63, (color & 0x1f) * 255 / 31, 255); }
    void Set32(uint32_t color)                                     { m_Color = color; }
    void SetRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { m_Color = (r << 16) | (g << 8) | b | (a << 24); }
    
    uint8_t GetRed() const { return (m_Color >> 16) & 0xff; }
    uint8_t GetGreen() const { return (m_Color >> 8) & 0xff; }
    uint8_t GetBlue() const { return m_Color & 0xff; }
    uint8_t GetAlpha() const { return (m_Color >> 24) & 0xff; }

    Color GetNorimalized() const { return GetNorimalized(GetAlpha()); }
    Color GetNorimalized(uint8_t alpha) const { return Color((uint32_t(GetRed()) * alpha + 127) / 255, (uint32_t(GetGreen()) * alpha + 127) / 255, (uint32_t(GetBlue()) * alpha + 127) / 255); }

    uint16_t GetColor16() const { return ((GetRed() & 0xf8) << 8) | ((GetGreen() & 0xfc) << 3) | ((GetBlue() & 0xf8) >> 3); }
    uint32_t GetColor32() const { return m_Color; }


    uint32_t m_Color;
};

} // namespace
