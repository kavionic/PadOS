// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 28.06.2020 12:56

#pragma once

#include <GUI/Widgets/Control.h>


class PButtonGroup;

class PButtonBase : public PControl
{
public:
    PButtonBase(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PButtonBase(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData, PAlignment defaultLabelAlignment);
    ~PButtonBase();

    static Ptr<PButtonGroup> FindButtonGroup(Ptr<PView> root, const PString& name);

    virtual void AllAttachedToScreen() override { Invalidate(); }

    virtual void OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerOver(PPointerID pointerID, const PPoint& position, const PPointerEvent& pointerEvent, PEventPhase phase) override;
    virtual void OnPointerOut(PPointerID pointerID, const PPoint& position, const PPointerEvent& pointerEvent, PEventPhase phase) override;
    virtual void OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason) override;

    void SetCheckable(bool value) { m_CanBeCheked = value; }
    bool IsCheckable() const { return m_CanBeCheked; }

    void SetChecked(bool isChecked);
    bool IsChecked() const { return m_IsChecked; }

    // From Control:
    virtual void OnEnableStatusChanged(bool isEnabled) override;

    // From ButtonBase:
    virtual void OnPressedStateChanged(bool isPressed) { Invalidate(); Flush(); }
    virtual void OnCheckedStateChanged(bool isChecked) { Invalidate(); Flush(); }

    Ptr<PButtonGroup> GetButtonGroup() const;

    Signal<void, PPointerID, PButtonBase*>    SignalActivated;
    Signal<void, bool, PButtonBase*>             SignalToggled;

protected:
    void SetPressedState(bool isPressed);
    bool GetPressedState() const { return m_IsPressed; }
    bool IsPointerOver() const { return m_IsPointerOver; }


private:
    friend class PButtonGroup;

    void ActivateCheckableButton();
    void SetButtonGroup(Ptr<PButtonGroup> group);
    PPointerID           m_HitPointerID = PInvalidPointerID;
    Ptr<PButtonGroup>        m_ButtonGroup;
    bool                    m_CanBeCheked = false;
    bool                    m_IsPressed = false;
    bool                    m_IsChecked = false;
    bool                    m_IsPointerOver = false;


    PButtonBase(const PButtonBase&) = delete;
    PButtonBase& operator=(const PButtonBase&) = delete;
};
