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
// Created: 15.04.2018 16:38:59

#include "System/Platform.h"

#include "GUI/Widgets/TextView.h"
#include "Utils/Utils.h"
#include "Utils/XMLObjectParser.h"


const std::map<PString, uint32_t> PTextViewFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(PTextViewFlags, IncludeLineGap),
    DEFINE_FLAG_MAP_ENTRY(PTextViewFlags, MultiLine),
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextView::PTextView(const PString& name, const PString& text, Ptr<PView> parent, uint32_t flags) : PView(name, parent, flags | PViewFlags::WillDraw)
{
    SetText(text);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextView::PTextView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PView(context, parent, xmlData)
{
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, PTextViewFlags::FlagMap, "flags", 0) | PViewFlags::WillDraw);

    SetText(context.GetAttribute(xmlData, "text", PString::zero));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextView::~PTextView()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextView::SetText(const PString& text)
{
//    ProfileTimer timer("TextView::SetText()");    
    m_Text = text;

    m_TextWidth = GetStringWidth(m_Text);

    if (HasFlags(PTextViewFlags::MultiLine))
    {
        m_IsWordWrappingValid = false;
    }
    PreferredSizeChanged();
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextView::OnFrameSized(const PPoint& delta)
{
    PView::OnFrameSized(delta);

    if (delta.x != 0.0f && HasFlags(PTextViewFlags::MultiLine))
    {
        m_IsWordWrappingValid = false;
        PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextView::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    PFontHeight fontHeight = GetFontHeight();
    if (HasFlags(PTextViewFlags::MultiLine))
    {
        if (includeWidth)
        {
            if (m_AspectRatio != 0.0f)
            {
                const PFontHeight fontHeight = GetFontHeight();

                const float textArea = m_TextWidth * (fontHeight.descender - fontHeight.ascender + fontHeight.line_gap);

                minSize->x = ceilf(sqrtf(textArea) * sqrtf(m_AspectRatio));
                maxSize->x = minSize->x;
            }
            else
            {
                minSize->x = 0.0f;
                maxSize->x = COORD_MAX;
            }
        }
        if (includeHeight)
        {
            UpdateWordWrapping();

            minSize->y = std::max(1.0f, float(m_LineWraps.size())) * (fontHeight.descender - fontHeight.ascender + fontHeight.line_gap);
            if (!HasFlags(PTextViewFlags::IncludeLineGap)) {
                minSize->y -= fontHeight.line_gap;
            }
            maxSize->y = minSize->y;
        }
    }
    else
    {
        PPoint size;
        if (includeWidth) {
            size.x = m_TextWidth;
        }
        if (includeHeight)
        {
            size.y = fontHeight.descender - fontHeight.ascender;
            if (HasFlags(PTextViewFlags::IncludeLineGap)) {
                size.y += fontHeight.line_gap;
            }
        }
        *minSize = size;
        *maxSize = size;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextView::OnPaint(const PRect& updateRect)
{
    if (UpdateWordWrapping()) {
        PreferredSizeChanged();
    }

    SetEraseColor(GetBgColor());
    PRect bounds = GetBounds();
    EraseRect(bounds);

    MovePenTo(0.0f, 0.0f);

    if (!m_LineWraps.empty())
    {
        const PFontHeight fontHeight = GetFontHeight();
        const float lineHeight = fontHeight.descender - fontHeight.ascender + fontHeight.line_gap;

        const char* lineStart = m_Text.c_str();
        for (size_t nextLineStart : m_LineWraps)
        {
            const char* nextLine = m_Text.c_str() + nextLineStart;
            size_t lineLength = nextLine - lineStart;
            DrawString(PString(lineStart, lineLength));
            lineStart = nextLine;
            MovePenBy(0.0f, lineHeight);
        }
    }
    else
    {
        DrawString(m_Text);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTextView::UpdateWordWrapping()
{
    if (m_IsWordWrappingValid) {
        return false;
    }
    m_IsWordWrappingValid = true;
    if (!HasFlags(PTextViewFlags::MultiLine))
    {
        const bool wasEmpty = m_LineWraps.empty();
        m_LineWraps.clear();
        return !wasEmpty;
    }
    Ptr<PFont> font = GetFont();

    if (font == nullptr)
    {
        m_LineWraps.clear();
        return false;
    }
    size_t oldLineCount = m_LineWraps.size();
    float wrapWidth;
    wrapWidth = GetBounds().Width();

    size_t line = 0;

    if (wrapWidth > 0.0f)
    {
        const char* currentLineStart = m_Text.c_str();
        size_t      remainingChars = m_Text.size();
        for (; remainingChars != 0; ++line)
        {
            int lineLength = font->GetStringLength(currentLineStart, remainingChars, wrapWidth);
            if (lineLength < 1) lineLength = 1;

            for (size_t i = 0; i < lineLength; ++i)
            {
                if (currentLineStart[i] == '\n')
                {
                    lineLength = i;
                    break;
                }
            }

            if (lineLength != remainingChars)
            {
                int lastWordEnd = lineLength;
                while (lastWordEnd > 0 && !isspace(currentLineStart[lastWordEnd])) lastWordEnd--;
                if (lastWordEnd != 0) {
                    lineLength = lastWordEnd;
                }
            }

            // Make the next line start after any trailing white-spaces.
            while (lineLength < remainingChars && isspace(currentLineStart[lineLength])) lineLength++;
            const size_t nextLineStart = (currentLineStart - m_Text.c_str()) + lineLength;
            if (line < m_LineWraps.size()) {
                m_LineWraps[line] = nextLineStart;
            } else {
                m_LineWraps.push_back(nextLineStart);
            }
            remainingChars -= lineLength;
            currentLineStart += lineLength;
        }
    }
    m_LineWraps.resize(line);

    return line != oldLineCount;
}

