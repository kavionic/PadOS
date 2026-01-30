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

#pragma once

#include <GUI/Window.h>

class PButtonBase;
class PButton;
class PDialogBoxView;


enum class PDialogButtonID : int32_t
{
    None,
    Cancel,
    Ok,
    No = Cancel,
    Yes = Ok,
    FirstCustom = 100
};
enum class PDialogButtonSets : int
{
    None,
    Ok,
    Cancel,
    OkCancel,
    YesNo
};

class PDialogBase : public PWindow
{
public:
    PDialogBase(const PString& title, const PString& text, PDialogButtonSets buttonSet);
    virtual ~PDialogBase();

    Ptr<PView>   SetContentView(Ptr<PView> contentView);
    Ptr<PButton> AddButton(const PString& label, PDialogButtonID buttonID);
    Ptr<PButton> AddButton(const PString& label, int32_t buttonID);

    Ptr<PButton> FindButton(PDialogButtonID buttonID) const;
    Ptr<PButton> FindButton(int32_t buttonID) const;
    Ptr<PButton> FindButton(const PString& buttonName) const;

    PDialogButtonID Go(Ptr<PView> owner);
protected:
    virtual void OnActivated(PDialogButtonID buttonID) = 0;

private:
    void SlotSelected(PDialogButtonID buttonID);

    Ptr<PDialogBoxView> m_DialogView;
//    WeakPtr<View>      m_OwnerView;
    PDialogButtonID     m_ButtonClicked = PDialogButtonID::None;
};
