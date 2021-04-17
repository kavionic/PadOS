// This file is part of PadOS.
//
// Copyright (C) 2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.04.2021 15:40

#include <GUI/Dialogs/DialogBase.h>
#include <GUI/Widgets/Button.h>

namespace os
{

class DialogBoxView : public View
{
public:
    DialogBoxView(const String& title, const String& text, DialogButtonSets buttonSet);

    Ptr<View>   SetContentView(Ptr<View> contentView);
    Ptr<Button> AddButton(const String& label, DialogButtonID buttonID);

    Ptr<Button> FindButton(int32_t buttonID) const;
    Ptr<Button> FindButton(const String& buttonName) const;

public:
    Signal<void, DialogButtonID> SignalSelected;

private:
    void SlotButtonClicked(MouseButton_e mouseButton, ButtonBase* button);

    Ptr<View>                   m_ContentView;
    Ptr<View>                   m_ButtonContainer;
    std::vector<Ptr<Button>>    m_Buttons;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DialogBase::DialogBase(const String& title, const String& text, DialogButtonSets buttonSet) : Window(title)
{
    m_DialogView = ptr_new<DialogBoxView>(title, text, buttonSet);

    m_DialogView->SignalSelected.Connect(this, &DialogBase::SlotSelected);

    SetClient(m_DialogView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DialogBase::~DialogBase()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> DialogBase::SetContentView(Ptr<View> contentView)
{
    return m_DialogView->SetContentView(contentView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Button> DialogBase::AddButton(const String& label, DialogButtonID buttonID)
{
    return m_DialogView->AddButton(label, buttonID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Button> DialogBase::AddButton(const String& label, int32_t buttonID)
{
    return AddButton(label, DialogButtonID(buttonID));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DialogBase::SlotSelected(DialogButtonID buttonID)
{
    m_ButtonClicked = buttonID;
    OnActivated(buttonID);
    Close();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Button> DialogBase::FindButton(DialogButtonID buttonID) const
{
    return FindButton(int32_t(buttonID));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Button> DialogBase::FindButton(int32_t buttonID) const
{
    return m_DialogView->FindButton(buttonID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Button> DialogBase::FindButton(const String& buttonName) const
{
    return m_DialogView->FindButton(buttonName);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DialogButtonID DialogBase::Go(Ptr<View> owner)
{
    Ptr<DialogBase> self = ptr_tmp_cast(this); // Make sure we are not deleted before returning.

    Open((owner != nullptr) ? owner->GetApplication() : nullptr);

    Looper* looper = GetLooper();

    if (looper != nullptr)
    {
        while (HasFlags(ViewFlags::IsAttachedToScreen) && (owner == nullptr || owner->HasFlags(ViewFlags::IsAttachedToScreen)) && looper->Tick());
    }
    if (HasFlags(ViewFlags::IsAttachedToScreen)) {
        Close();
    }
    return m_ButtonClicked;
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DialogBoxView::DialogBoxView(const String& title, const String& text, DialogButtonSets buttonSet) : View(title, nullptr)
{
    SetLayoutNode(ptr_new<VLayoutNode>());

    m_ButtonContainer = ptr_new<View>("DlgButtons");
    AddChild(m_ButtonContainer);
    m_ButtonContainer->SetLayoutNode(ptr_new<HLayoutNode>());
    m_ButtonContainer->SetHAlignment(Alignment::Right);

    switch (buttonSet)
    {
        case DialogButtonSets::None:
            break;
        case DialogButtonSets::Ok:
            AddButton("Ok", DialogButtonID::Ok);
            break;
        case DialogButtonSets::Cancel:
            AddButton("Cancel", DialogButtonID::Cancel);
            break;
        case DialogButtonSets::OkCancel:
            AddButton("Cancel", DialogButtonID::Cancel);
            AddButton("Ok", DialogButtonID::Ok);
            break;
        case DialogButtonSets::YesNo:
            AddButton("No", DialogButtonID::No);
            AddButton("Yes", DialogButtonID::Yes);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> DialogBoxView::SetContentView(Ptr<View> contentView)
{
    Ptr<View> prevContentView = m_ContentView;
    if (m_ContentView != nullptr) {
        m_ContentView->RemoveThis();
    }
    m_ContentView = contentView;
    if (m_ContentView != nullptr) {
        m_ContentView->SetBorders(10.0f, 10.0f, 10.0f, 0.0f);
        InsertChild(m_ContentView, 0);
    }
    return prevContentView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Button> DialogBoxView::AddButton(const String& label, DialogButtonID buttonID)
{
    Ptr<Button> button = ptr_new<Button>("DlgButton", label);
    button->SetBorders(m_Buttons.empty() ? 10.0f : 0.0f, 10.0f, 10.0f, 10.0f);
    m_ButtonContainer->AddChild(button);
    button->SetID(int32_t(buttonID));
    button->SignalActivated.Connect(this, &DialogBoxView::SlotButtonClicked);

    if (!m_Buttons.empty()) {
        button->AddToWidthRing(m_Buttons[0]);
    }

    m_Buttons.push_back(button);

    return button;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Button> DialogBoxView::FindButton(int32_t buttonID) const
{
    return m_ButtonContainer->FindChildIf<Button>([buttonID](Ptr<View> view) { Ptr<Button> button = ptr_dynamic_cast<Button>(view); return button != nullptr && button->GetID() == buttonID; }, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Button> DialogBoxView::FindButton(const String& buttonName) const
{
    return m_ButtonContainer->FindChild<Button>(buttonName, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DialogBoxView::SlotButtonClicked(MouseButton_e mouseButton, ButtonBase* button)
{
    SignalSelected(DialogButtonID(button->GetID()));
}

} //namespace os
