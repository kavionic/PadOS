// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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


#include <GUI/ProgressBar.h>
#include <GUI/Font.h>

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ProgressBar::ProgressBar(const std::string& name, Ptr<View> parent, Orientation orientation, uint32_t flags)
    : View(name, parent, flags | ViewFlags::WillDraw | ViewFlags::ClearBackground)
{
    m_Progress = 0.0f;
    m_Orientation = orientation;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ProgressBar::Paint(const Rect& updateRect)
{
    Rect bounds = GetNormalizedBounds();
    bounds.Floor();

    DrawFrame(bounds, FRAME_RECESSED | FRAME_TRANSPARENT);

    float barLength = (m_Orientation == Orientation::Horizontal) ? bounds.Width() : bounds.Height();
    barLength = ceil((barLength - 4.0f) * m_Progress);

    Rect barFrame = bounds;
    barFrame.Resize(2.0f, 2.0f, -2.0f, -2.0f);
    Rect restFrame = barFrame;

    if (barLength < 1.0f)
    {
        FillRect(restFrame, GetBgColor());
    }
    else
    {
        if (m_Orientation == Orientation::Horizontal)
        {
            barFrame.right = barFrame.left + barLength;
            if (barFrame.right > bounds.right - 2.0f) {
                barFrame.right = bounds.right - 2.0f;
            }
            restFrame.left = barFrame.right;
        }
        else
        {
            barFrame.top = barFrame.bottom - barLength;
            if (barFrame.right < bounds.right + 2.0f) {
                barFrame.top = bounds.top + 2.0f;
            }
            restFrame.bottom = barFrame.top;
        }

        if (barLength >= 1.0f) {
            FillRect(barFrame, Color(0x66, 0x88, 0xbb));
        }
        if (restFrame.IsValid()) {
            FillRect(restFrame, GetBgColor());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ProgressBar::FrameSized(const Point& delta)
{
    Rect bounds(GetBounds());
    bool needFlush = false;
    if ((m_Orientation == Orientation::Horizontal && delta.x != 0.0f) || (m_Orientation == Orientation::Vertical && delta.y != 0.0f))
    {
        Rect damage = bounds;
        damage.Resize(2.0f, 2.0f, -2.0f, -2.0f);
        Invalidate(damage);
        needFlush = true;
    }
    if (delta.x != 0.0f)
    {
        Rect damage = bounds;

        damage.left = damage.right - std::max(2.0f, delta.x + 2.0f);
        Invalidate(damage);
        needFlush = true;
    }
    if (delta.y != 0.0f)
    {
        Rect damage = bounds;

        damage.top = damage.bottom - std::max(2.0f, delta.y + 2.0f);
        Invalidate(damage);
        needFlush = true;
    }
    if (needFlush) {
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ProgressBar::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    minSize->x = 50.0f;
    minSize->y = 24.0f;

    maxSize->x = LAYOUT_MAX_SIZE;
    maxSize->y = minSize->y;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ProgressBar::SetProgress(float value)
{
    m_Progress = value;
    Invalidate();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float ProgressBar::GetProgress() const
{
    return m_Progress;
}
