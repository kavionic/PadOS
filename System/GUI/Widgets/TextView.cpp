// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 15.04.2018 16:38:59

#include "System/Platform.h"

#include "GUI/Widgets/TextView.h"
#include "Utils/Utils.h"
#include <Utils/UTF8Utils.h>
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

void PTextView::OnFontChanged(Ptr<const PFont> newFont)
{
    m_TextWidth = GetStringWidth(m_Text);
    m_IsWordWrappingValid = false;
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

                const float textArea = m_TextWidth * (fontHeight.descender + fontHeight.ascender + fontHeight.line_gap);

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

            minSize->y = std::max(1.0f, float(m_LineWraps.size()))
                * (fontHeight.descender + fontHeight.ascender + fontHeight.line_gap);
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
            size.y = fontHeight.descender + fontHeight.ascender;
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
    const PRect bounds = GetBounds();
    const PFontHeight fontHeight = GetFontHeight();
    PPoint baseline(0.0f, fontHeight.ascender);

    if (!m_LineWraps.empty())
    {
        const float lineHeight = fontHeight.descender + fontHeight.ascender + fontHeight.line_gap;
        const Ptr<const PFont> font = GetFont();
        const bool fillTextBand = font != nullptr && GetDrawingMode() == PDrawingMode::Copy
            && font->GetDescription().Rotation == 0.0f && font->GetDescription().Shear == 90.0f;
        if (!fillTextBand) {
            EraseRect(bounds);
        }

        PRect lineFrame = bounds;
        const char* lineStart = m_Text.c_str();
        for (size_t i = 0; i < m_LineWraps.size(); ++i)
        {
            const char* nextLine = m_Text.c_str() + m_LineWraps[i];
            const size_t lineLength = nextLine - lineStart;
            if (fillTextBand)
            {
                // Keep line spacing uniform across zero when scrolling.
                const float pixelBaseline = floorf(baseline.y + GetScrollOffset().y);
                const float baselineCenter = pixelBaseline + ((pixelBaseline < 0.0f) ? -0.5f : 0.5f);
                const PPoint position(baseline.x, baselineCenter - GetScrollOffset().y);
                const float nextTop = pixelBaseline + lineHeight - fontHeight.ascender + 0.5f - GetScrollOffset().y;
                lineFrame.bottom = (i + 1 == m_LineWraps.size()) ? bounds.bottom : std::min(bounds.bottom, nextTop);
                DrawString(PString(lineStart, lineLength), position, lineFrame);
                lineFrame.top = std::max(bounds.top, lineFrame.bottom);
            }
            else
            {
                DrawString(PString(lineStart, lineLength), baseline);
            }
            lineStart = nextLine;
            baseline.y += lineHeight;
        }
    }
    else
    {
        DrawString(m_Text, baseline, bounds);
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
    Ptr<const PFont> font = GetFont();

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
            if (lineLength < 1)
            {
                size_t characterLength = 0;
                utf8_to_unicode(currentLineStart, remainingChars, characterLength);
                lineLength = int(characterLength);
            }

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

