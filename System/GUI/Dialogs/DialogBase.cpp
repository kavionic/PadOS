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


class PDialogBoxView : public PView
{
public:
    PDialogBoxView(const PString& title, const PString& text, PDialogButtonSets buttonSet);

    Ptr<PView>   SetContentView(Ptr<PView> contentView);
    Ptr<PButton> AddButton(const PString& label, PDialogButtonID buttonID);

    Ptr<PButton> FindButton(int32_t buttonID) const;
    Ptr<PButton> FindButton(const PString& buttonName) const;

public:
    Signal<void, PDialogButtonID> SignalSelected;

private:
    void SlotButtonClicked(PMouseButton mouseButton, PButtonBase* button);

    Ptr<PView>                   m_ContentView;
    Ptr<PView>                   m_ButtonContainer;
    std::vector<Ptr<PButton>>    m_Buttons;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDialogBase::PDialogBase(const PString& title, const PString& text, PDialogButtonSets buttonSet) : PWindow(title)
{
    m_DialogView = ptr_new<PDialogBoxView>(title, text, buttonSet);

    m_DialogView->SignalSelected.Connect(this, &PDialogBase::SlotSelected);

    SetClient(m_DialogView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDialogBase::~PDialogBase()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PDialogBase::SetContentView(Ptr<PView> contentView)
{
    return m_DialogView->SetContentView(contentView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButton> PDialogBase::AddButton(const PString& label, PDialogButtonID buttonID)
{
    return m_DialogView->AddButton(label, buttonID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButton> PDialogBase::AddButton(const PString& label, int32_t buttonID)
{
    return AddButton(label, PDialogButtonID(buttonID));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDialogBase::SlotSelected(PDialogButtonID buttonID)
{
    m_ButtonClicked = buttonID;
    OnActivated(buttonID);
    Close();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButton> PDialogBase::FindButton(PDialogButtonID buttonID) const
{
    return FindButton(int32_t(buttonID));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButton> PDialogBase::FindButton(int32_t buttonID) const
{
    return m_DialogView->FindButton(buttonID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButton> PDialogBase::FindButton(const PString& buttonName) const
{
    return m_DialogView->FindButton(buttonName);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDialogButtonID PDialogBase::Go(Ptr<PView> owner)
{
    Ptr<PDialogBase> self = ptr_tmp_cast(this); // Make sure we are not deleted before returning.

    Open((owner != nullptr) ? owner->GetApplication() : nullptr);

    PLooper* looper = GetLooper();

    if (looper != nullptr)
    {
        while (HasFlags(PViewFlags::IsAttachedToScreen) && (owner == nullptr || owner->HasFlags(PViewFlags::IsAttachedToScreen)) && looper->Tick());
    }
    if (HasFlags(PViewFlags::IsAttachedToScreen)) {
        Close();
    }
    return m_ButtonClicked;
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDialogBoxView::PDialogBoxView(const PString& title, const PString& text, PDialogButtonSets buttonSet) : PView(title, nullptr)
{
    SetLayoutNode(ptr_new<PVLayoutNode>());

    m_ButtonContainer = ptr_new<PView>("DlgButtons");
    AddChild(m_ButtonContainer);
    m_ButtonContainer->SetLayoutNode(ptr_new<PHLayoutNode>());
    m_ButtonContainer->SetHAlignment(PAlignment::Right);

    switch (buttonSet)
    {
        case PDialogButtonSets::None:
            break;
        case PDialogButtonSets::Ok:
            AddButton("Ok", PDialogButtonID::Ok);
            break;
        case PDialogButtonSets::Cancel:
            AddButton("Cancel", PDialogButtonID::Cancel);
            break;
        case PDialogButtonSets::OkCancel:
            AddButton("Cancel", PDialogButtonID::Cancel);
            AddButton("Ok", PDialogButtonID::Ok);
            break;
        case PDialogButtonSets::YesNo:
            AddButton("No", PDialogButtonID::No);
            AddButton("Yes", PDialogButtonID::Yes);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PDialogBoxView::SetContentView(Ptr<PView> contentView)
{
    Ptr<PView> prevContentView = m_ContentView;
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

Ptr<PButton> PDialogBoxView::AddButton(const PString& label, PDialogButtonID buttonID)
{
    Ptr<PButton> button = ptr_new<PButton>("DlgButton", label);
    button->SetBorders(m_Buttons.empty() ? 10.0f : 0.0f, 10.0f, 10.0f, 10.0f);
    m_ButtonContainer->AddChild(button);
    button->SetID(int32_t(buttonID));
    button->SignalActivated.Connect(this, &PDialogBoxView::SlotButtonClicked);

    if (!m_Buttons.empty()) {
        button->AddToWidthRing(m_Buttons[0]);
    }

    m_Buttons.push_back(button);

    return button;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButton> PDialogBoxView::FindButton(int32_t buttonID) const
{
    return m_ButtonContainer->FindChildIf<PButton>([buttonID](Ptr<PView> view) { Ptr<PButton> button = ptr_dynamic_cast<PButton>(view); return button != nullptr && button->GetID() == buttonID; }, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButton> PDialogBoxView::FindButton(const PString& buttonName) const
{
    return m_ButtonContainer->FindChild<PButton>(buttonName, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDialogBoxView::SlotButtonClicked(PMouseButton mouseButton, PButtonBase* button)
{
    SignalSelected(PDialogButtonID(button->GetID()));
}
