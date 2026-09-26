// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Widgets/MenuSeparator.h>
#include <GUI/Widgets/Menu.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMenuSeparator::PMenuSeparator() : PMenuItem(PString::zero)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMenuSeparator::~PMenuSeparator()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PMenuSeparator::GetContentSize()
{
    Ptr<PMenu> menu = GetSuperMenu();

    if (menu != nullptr)
    {
        if (menu->GetLayout() == PMenuLayout::Horizontal) {
            return PPoint(6.0f, 0.0f);
        } else {
            return PPoint(0.0f, 6.0f);
        }
    }
    return PPoint(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenuSeparator::Draw(Ptr<PView> targetView)
{
    Ptr<PMenu> menu = GetSuperMenu();

    if (menu == nullptr) {
        return;
    }
    PRect frame = GetFrame();
    PRect viewFrame = targetView->GetBounds();

    if (menu->GetLayout() == PMenuLayout::Horizontal)
    {
        frame.top = viewFrame.top;
        frame.bottom = viewFrame.bottom;
    }
    else
    {
        frame.left = viewFrame.left;
        frame.right = viewFrame.right;
    }
    targetView->SetFgColor(PStandardColorID::MenuBackground);
    targetView->FillRect(frame);

    if (menu->GetLayout() == PMenuLayout::Horizontal)
    {
        float x = std::floor(frame.left + (frame.Width() + 1.0f) * 0.5f);
        targetView->SetFgColor(PStandardColorID::Shadow);
        targetView->DrawLine(PPoint(x, frame.top + 2.0f), PPoint(x, frame.bottom - 2.0f));
        x += 1.0f;
        targetView->SetFgColor(PStandardColorID::Shine);
        targetView->DrawLine(PPoint(x, frame.top + 2.0f), PPoint(x, frame.bottom - 2.0f));
    }
    else
    {
        float y = std::floor(frame.top + (frame.Height() + 1.0f) * 0.5f);
        targetView->SetFgColor(PStandardColorID::Shadow);
        targetView->DrawLine(PPoint(frame.left + 4.0f, y), PPoint(frame.right - 4.0f, y));
        y += 1.0f;
        targetView->SetFgColor(PStandardColorID::Shine);
        targetView->DrawLine(PPoint(frame.left + 4.0f, y), PPoint(frame.right - 4.0f, y));
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenuSeparator::DrawContent(Ptr<PView> targetView)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenuSeparator::Highlight(bool highlight)
{
}
